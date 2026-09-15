/// generate_types
///
/// Command line interface to generate a yaml type specification file.
/// Other tools can be used to generate interface files from the yaml file.
///
/// This must run in an environment where everything ROOT and the
/// atlas software is available.
///
#include "translate.hpp"
#include "type_helpers.hpp"
#include "utils.hpp"
#include "xaod_helpers.hpp"
#include "collections_info.hpp"
#include "helper_files.hpp"
#include "metadata_file_finder.hpp"

#include "TSystem.h"
#include "TROOT.h"
#include "TClassTable.h"

#include "yaml-cpp/yaml.h"
#include <argparse/argparse.hpp>

#include <iostream>
#include <queue>
#include <set>
#include <algorithm>
#include <iterator>
#include <fstream>

using namespace std;

// Return true if it is ok to emit this particular class.
// Check for specific bad types (e.g. string, types of vector, etc.).
// No checking w.r.t. other lists is done.
bool can_emit_class(const class_info &c_info) {
    if (c_info.name_as_type.type_name == "string") {
        return false;
    }
    if ((c_info.name_as_type.type_name != "vector") && (c_info.name_as_type.type_name != "ElementLink")) {
        if (c_info.name_as_type.template_arguments.size() > 0) {
            return false;
        }
    }
    if (c_info.methods.size() == 0 && c_info.enums.size() == 0) {
        return false;
    }
    return true;
}

// Return true if any method can be emitted
bool can_emit_any_methods(const vector<method_info> &methods, const set<string> &classes_to_emit) {
    for (auto &&m : methods)
    {
        if (is_understood_method(m, classes_to_emit)) {
            return true;
        }
    }
    return false;
}

// If this has template arguments, see if they are in the list of classes to emit
bool check_template_arguments(const typename_info &info, const set<string> &classes_to_emit) {
    // If this is an element link, we skip a level.
    if (info.type_name == "ElementLink" && info.template_arguments[0].type_name == "DataVector") {
        return check_template_arguments(info.template_arguments[0].template_arguments[0], classes_to_emit);
    }

    // Otherwise, we should look at everything.
    for(auto && ta: info.template_arguments) {
        if (ta.template_arguments.size() > 0) {
            if (!check_template_arguments(ta, classes_to_emit)) {
                return false;
            }
        } else {
            if (classes_to_emit.find(ta.cpp_name) == classes_to_emit.end()) {
                return false;
            }
        }
    }

    // Everything checks out!
    return true;
}

set<string> _g_bad_root_libraries({
    "RIO.so",
    "Tree.so",
    "Graf.so",
    "Gui.so",
    "Hist.so",
    "TreePlayer.so",
    "Gpad.so",
});

set<string> _g_bad_root_classes({
    "TClonesArray",
    "TArray",
    "TArrayC",
    "TArrayD",
    "TArrayF",
    "TArrayI",
    "TAttAxis",
    "TBrowser",
    "TBroserImp",
    "TBuffer",
    "TBuffer3D",
    "TClass",
    "type_info"
});

// If this is a ROOT class that we do not want to have any part of
// our results...
bool is_root_only_class(const class_info &info) {
    if (_g_bad_root_libraries.find(info.library_name) != _g_bad_root_libraries.end()) {
        return false;
    }
    if (_g_bad_root_classes.find(info.name) != _g_bad_root_classes.end()) {
        return false;
    }
    if (info.library_name.size() == 0 && info.name[0] == 'T') {
        return false;
    }
    return true;
}

// Collections can be actual collections or just single items. Get
// the type correctly in both those cases.
string extract_container_iterator_type(const collection_info &c)
{
    switch (c.iterator_type_info.template_arguments.size()) {
        case 0:
            return c.iterator_type_info.cpp_name;
            break;
        
        case 1:
            return c.iterator_type_info.template_arguments[0].cpp_name;
            break;
        
        default:
            throw runtime_error("Do not know how to deal with the collection of iterator type " + c.iterator_type_info.cpp_name);
    }
}

// These classes are known to be problematic
set<string> _g_bad_classes{
    "ROOT",
    "SG",
    "TObject",
    "SG::auxid_set_t",
    "DataModel_detail",
};

// Inspect the class name. There are just some classes we
// should not be writing out under any circumstances.
bool class_name_is_good(const string &c_name) {
    return (_g_bad_classes.find(c_name) == _g_bad_classes.end()
        && (c_name.find("Eigen") == c_name.npos)
        && (c_name.find("SG::") == c_name.npos)
        && (c_name.find("ROOT::Experimental") != 0)
       );
}

// Given a list of arguments, dump them out.
void dump_arguments(const string &arg_list_name, const vector<method_arg> &arguments, YAML::Emitter &out) {
    bool first_argument = true;
    for (auto &&arg : arguments)
    {
        if (first_argument) {
            first_argument = false;
            out << YAML::Key << arg_list_name
                << YAML::Value
                << YAML::BeginSeq;
        }
        out << YAML::BeginMap
            << YAML::Key << "name" << YAML::Value << arg.name
            << YAML::Key << "type" << YAML::Value << normalized_type_name(arg.full_typename)
            << YAML::EndMap;
    }
    if (!first_argument) {
        out << YAML::EndSeq;
    }
}

// Function to get the list of all types we are able to consider
set<string> get_known_types(const set<string>& classes_to_emit, const map<string, class_info>& class_map)
{
    set<string> known_types(classes_to_emit.begin(), classes_to_emit.end());
    for (auto &&c_name : classes_to_emit)
    {
        auto class_info_ptr = class_map.find(c_name);
        if (class_info_ptr != class_map.end()) {
            auto &&class_info = class_info_ptr->second;
            auto defined_enums = class_enums(class_info);
            known_types.insert(defined_enums.begin(), defined_enums.end());
        }
    }
    return known_types;
}

// Find c_name as a class in the map, or if not, look one level up
// to see if the class has an enum named.
map<string, class_info>::const_iterator find_class_or_enum(const string &c_name, const map<string, class_info> &class_map)
{
    auto c_info = class_map.find(c_name);
    if (c_info != class_map.end()) {
        return c_info;
    }

    auto t = parse_typename(c_name);
    if (t.namespace_list.size() > 0) {
        auto parent_class_name = unqualified_typename(parent_class(t));
        auto parent_class_itr = class_map.find(parent_class_name);
        if (parent_class_itr == class_map.end()) {
            return class_map.end();
        }

        // Check to see if c_name is an enum in this class
        auto all_enums = class_enums(parent_class_itr->second);
        if (find(all_enums.begin(), all_enums.end(), c_name) != all_enums.end()) {
            return parent_class_itr;
        }
    }

    return class_map.end();
}

int main(int argc, char**argv) {
    auto app_reference = create_root_app();

    // Parse the command line arguments
    argparse::ArgumentParser program("generate_types");

    program.add_argument("-l", "--library")
        .help("Load shared library")
        .append()
        .default_value(vector<string>{});

    program.add_argument("-c", "--class")
        .help("Translate class")
        .append()
        .required();

    program.add_argument("-h", "--help")
        .default_value(false)
        .implicit_value(true)
        .nargs(0)
        .help("This message");

    try {
        program.parse_args(argc, argv);
    } catch (const runtime_error& err) {
        cerr << err.what() << endl;
        cerr << program;
        return 1;
    }

    if (program.get<bool>("--help")) {
        cout << program;
        return 1;
    }

    auto cmd_classes = program.get<vector<string>>("--class");
    queue<string> classes_to_do;
    set<string> classes_original_set;

    for (auto &&c_name : cmd_classes)
    {
        if (class_name_is_good(c_name)) {
            classes_to_do.push(c_name);
            classes_original_set.insert(c_name);
        }
    }

    auto libraries = program.get<vector<string>>("--library");
    for (auto &&l_name : libraries)
    {
        auto status = gSystem->Load(l_name.c_str());
        if (status < 0) {
            cerr << "ERROR: Can't load library " << l_name << " - status: " << status << endl;
        }
    }

    // Translate the classes from the ROOT system to our internal system, starting from
    // a given top level. Add all connected classes below that.
    set<string> classes_done;
    set<string> classes_original_set_done;
    vector<class_info> done_classes;
    set<string> seen_namespace_additions;

    while (classes_to_do.size() > 0) {
        // Grab a class and mark it on the list
        // so we don't try to re-run it.
        auto raw_class_name(classes_to_do.front());
        classes_to_do.pop();
        if (classes_done.find(raw_class_name) != classes_done.end())
            continue;
        classes_done.insert(raw_class_name);

        auto class_name = unqualified_type_name(raw_class_name);
        if ((class_name != raw_class_name)
            && (classes_done.find(class_name) != classes_done.end())) {
                continue;
            }
        classes_done.insert(class_name);

        // Translate the class
        auto c = translate_class(class_name);

        // Make sure to add all namespace qualifications in. This is
        // because ROOT will store "global" enums in those namespaces,
        // which is crazy but true.
        const auto tn = c.name.size() == 0 ? parse_typename(class_name) : c.name_as_type;
        string namespace_stem = "";
        for (auto &&ns : tn.namespace_list)
        {
            if (namespace_stem.size() > 0) {
                namespace_stem += "::";
            }
            namespace_stem += ns.cpp_name;
            if (class_name_is_good(namespace_stem))
            {
                // Make sure it isn't on the classes_to_do list or the classes_done list first
                if (classes_done.find(namespace_stem) == classes_done.end()
                    && seen_namespace_additions.find(namespace_stem) == seen_namespace_additions.end()) {
                    classes_to_do.push(namespace_stem);
                    seen_namespace_additions.insert(namespace_stem);
                }
            }
        }

        // If we translated it, look at all classes it referenced and
        // add them to the list to translate.
        if (c.name.size() > 0) {
            // Mark this class done
            done_classes.push_back(c);

            // And if this is one of the original classes, mark it as done too
            // with the full name we can do the lookup for.
            if (classes_original_set.find(raw_class_name) != classes_original_set.end()) {
                classes_original_set_done.insert(c.name);
            }

            // Add enum's to the `classes_done` list so we don't try to translate them again.
            auto defined_enums = class_enums(c);
            classes_done.insert(defined_enums.begin(), defined_enums.end());

            // Add any referenced classes to our class list!
            for (auto &&c_name : referenced_types(c))
            {
                if (class_name_is_good(c_name)) {
                    classes_to_do.push(c_name);
                }
            }

            // And add any template references in this class name
            auto t_info = parse_typename(class_name);
            for (auto &&t_name : type_referenced_types(t_info))
            {
                auto c_name = unqualified_type_name(t_name);
                if (class_name_is_good(c_name))
                {
                    classes_to_do.push(c_name);
                }
            }

        } else {
            // The class might actually be an enum, and the parent might
            // be something we can translate. So - parse off one level down in the
            // type name, and add it to the list, and see what happens.
            auto t = parse_typename(class_name);
            if (t.namespace_list.size() > 0) {
                auto parent_class_name = unqualified_typename(parent_class(t));
                if (class_name_is_good(parent_class_name)) {
                    classes_to_do.push(parent_class_name);
                }
            }
        }
    }

    // Look at the loaded type defs, and add aliases.
    fixup_type_aliases(done_classes);

    // Fix up type defs. We have to wait to do this b.c. otherwise
    // ROOT won't load the typedefs
    fixup_type_defs(done_classes);

    // Build a class map
    map<string, class_info> class_map;
    for (auto &&c : done_classes)
    {
        class_map[c.name] = c;
    }

    // Get the list of containers from the classes. These will be top level collections
    // stored in the data.
    auto all_collections = find_collections(done_classes);
    auto single_collections = get_single_object_collections(done_classes);
    copy(single_collections.begin(), single_collections.end(),
        back_inserter(all_collections));

    // Start by looking at the classes that are connected to our
    // containers.
    classes_done.clear();
    set<string> classes_to_emit;
    for (auto &&c : all_collections)
    {
        auto c_name (extract_container_iterator_type(c));
        if (class_name_is_good(c_name)) {
            classes_to_do.push(c_name);
        }
    }

    // Next, look at any of the classes that were on the original list
    for (auto &&c_name : classes_original_set_done)
    {
        classes_to_do.push(c_name);
    }

    // With that list of classes, lets find everything connected.
    while (!classes_to_do.empty()) {
        string c_name(unqualified_type_name(classes_to_do.front()));
        classes_to_do.pop();
        if (classes_done.find(c_name) != classes_done.end()) {
            continue;
        }
        classes_done.insert(c_name);

        // Find the class or enum
        auto c_info = find_class_or_enum(c_name, class_map);
        // auto c_info = class_map.find(c_name);
        if (c_info == class_map.end()) {
            continue;
        }

        // If we can dump the class, then we should!
        if (can_emit_class(c_info->second)) {
            classes_to_emit.insert(c_info->first);
        } else {
            cerr << "ERROR: Class " << c_name << " fails `can_emit_class`: not emitted." << endl;
        }

        // Now, add referenced classes to the queue
        auto reffed_classes = referenced_types(c_info->second);
        for (auto &&c_ref : reffed_classes)
        {
            if (class_name_is_good(c_ref)) {
                classes_to_do.push(c_ref);
            }
        }
    }

    // Add some of the default types that need no introduction
    classes_to_emit.insert("bool");
    classes_to_emit.insert("double");
    classes_to_emit.insert("float");
    classes_to_emit.insert("short");
    classes_to_emit.insert("unsigned short");
    classes_to_emit.insert("int");
    classes_to_emit.insert("int8_t");
    classes_to_emit.insert("int16_t");
    classes_to_emit.insert("int32_t");
    classes_to_emit.insert("int64_t");
    classes_to_emit.insert("uint8_t");
    classes_to_emit.insert("uint16_t");
    classes_to_emit.insert("uint64_t");
    classes_to_emit.insert("uint");
    classes_to_emit.insert("unsigned int");
    classes_to_emit.insert("char");
    classes_to_emit.insert("unsigned char");
    classes_to_emit.insert("size_t");
    classes_to_emit.insert("long");
    classes_to_emit.insert("unsigned long");
    classes_to_emit.insert("long long");
    classes_to_emit.insert("unsigned long long");
    classes_to_emit.insert("string");

    // Now, we need to loop through all of these things until we get a stable set of classes that we can emit.
    // This is painful, because there could be classes that look good, but contain no valid methods - so no need
    // to emit them. Cross them off the list, and another class' method is no longer interesting, in which case, that
    // has to be crossed. So we keep looping until we reach a stable set of classes.
    bool modified = true;
    while (modified)
    {
        modified = false;
        set<string> bad_classes;

        // Get a list of all types we are able to consider by merging the classes_to_emit and the enums that
        // those classes define together.
        auto known_types = get_known_types(classes_to_emit, class_map);
        
        // Now, what classes/methods can't be emitted due to lacking definitions?
        for (auto &&c_name : classes_to_emit)
        {
            auto class_info_ptr = class_map.find(c_name);
            if (class_info_ptr != class_map.end()) {
                auto &&class_info = class_info_ptr->second;
                if (!check_template_arguments(class_info.name_as_type, known_types)) {
                    bad_classes.insert(c_name);
                    cerr << "ERROR: Class " << c_name << " not translated: template arguments were bad." << endl;
                }
                if (!is_root_only_class(class_info)) {
                    cerr << "INFO: Class " << c_name << " not translated: ROOT only class." << endl;
                    bad_classes.insert(c_name);
                }
            }
        }
        if (bad_classes.size() > 0) {
            modified = true;
            for (auto &&b_c : bad_classes)
            {
                classes_to_emit.erase(b_c);
            }
        }
    }

    // Get the final list of known types we can work with.
    auto known_types = get_known_types(classes_to_emit, class_map);

    // Finally, go through the collections and keep only the ones where we are
    // dumping out the classes they contain.
    vector<collection_info> collections;
    copy_if(all_collections.begin(), all_collections.end(), back_insert_iterator(collections),
        [&classes_to_emit](const collection_info &c_info) {
            string collection_iterator_typename(extract_container_iterator_type(c_info));
            return find_if(classes_to_emit.begin(), classes_to_emit.end(), [&collection_iterator_typename](const string &cl_name){
                return cl_name == collection_iterator_typename;
            }) != classes_to_emit.end();
        });

    // Dump them all out
    YAML::Emitter out;
    out << YAML::BeginMap
        << YAML::Key << "collections"
        << YAML::Value
        << YAML::BeginSeq;
    for (auto &&c : collections)
    {
        out << YAML::BeginMap
            << YAML::Key << "collection_name" << YAML::Value << c.name
            << YAML::Key << "cpp_item_type" << YAML::Value << extract_container_iterator_type(c)
            << YAML::Key << "python_item_type" << YAML::Value << normalized_type_name(extract_container_iterator_type(c))
            << YAML::Key << "cpp_container_type" << YAML::Value << c.type_info.cpp_name
            << YAML::Key << "python_container_type" << YAML::Value << normalized_type_name(c.iterator_type_info)
            << YAML::Key << "include_file" << YAML::Value << c.include_file
            << YAML::Key << "link_libraries" << YAML::Value << YAML::BeginSeq;

        for (auto &&lib : c.link_libraries)
        {
            out << lib;
        }
        out << YAML::EndSeq;

        auto meta_data_itr = _g_collection_config.find(c.name);
        if (meta_data_itr != _g_collection_config.end()) {
            auto &&meta_data = meta_data_itr->second;

            if (meta_data.method_callback.size() > 0) {
                out << YAML::Key << "method_callback" << meta_data.method_callback;
            }

            if (meta_data.parameters.size() > 0) {
                out << YAML::Key << "parameters" << YAML::Value;
                out << YAML::BeginSeq;
                for (auto &&p : meta_data.parameters)
                {
                    out << YAML::BeginMap;
                    out << YAML::Key << "name" << YAML::Value << p.name;
                    out << YAML::Key << "type" << YAML::Value << p.p_type;
                    out << YAML::Key << "default_value" << YAML::Value << p.p_default;
                    out << YAML::EndMap;
                }                
                out << YAML::EndSeq;
            }

            if (meta_data.extra_parameters.size() > 0) {
                out << YAML::Key << "extra_parameters" << YAML::Value;
                out << YAML::BeginSeq;
                for (auto &&p : meta_data.extra_parameters)
                {
                    out << YAML::BeginMap;
                    out << YAML::Key << "name" << YAML::Value << p.name;
                    out << YAML::Key << "type" << YAML::Value << p.p_type;
                    out << YAML::Key << "default_value" << YAML::Value << p.p_default;
                    out << YAML::Key << "actions" << YAML::BeginSeq;

                    for (auto && a: p.variable_actions) {
                        out << YAML::BeginMap;
                        out << YAML::Key << "value" << YAML::Value << a.value;
                        out << YAML::Key << "metadata_names" << YAML::Value << YAML::BeginSeq;
                        for (auto &&md : a.metadata_names) {
                            out << md;
                        }
                        out << YAML::EndSeq;
                        out << YAML::Key << "bank_rename" << YAML::Value << a.bank_rename;
                        out << YAML::EndMap;
                    }

                    out << YAML::EndSeq;
                    out << YAML::EndMap;

                }
                
                out << YAML::EndSeq;
            }
        } else {
            // Write out the default name parameter for collections that have no actions associated
            // with them.
            out << YAML::Key << "parameters" << YAML::Value;
            out << YAML::BeginSeq;

            out << YAML::BeginMap;
            out << YAML::Key << "name" << YAML::Value << "name";
            out << YAML::Key << "type" << YAML::Value << "str";
            out << YAML::EndMap;

            out << YAML::EndSeq;
        }

        out << YAML::EndMap;
    }
    out << YAML::EndSeq;

    // Finally, we actually emit these.
    out << YAML::Key << "classes"
        << YAML::Value
        << YAML::BeginSeq;

    // Get a list of a list of all classes 
    map<string, vector<string>> failed_types;
    for (auto &&c : classes_to_emit)
    {
        string c_name(unqualified_type_name(c));

        // Find the class
        auto c_info = class_map.find(c_name);
        if (c_info == class_map.end()) {
            cerr << "ERROR: Ready to emit class " << c_name << " but it is not in the class map." << endl;
            continue;
        }

        if (!can_emit_class(c_info->second)) {
            cerr << "ERROR: Ready to emit class " << c_name << " but it is on our list of classes to block." << endl;
            continue;
        }

        // If we can dump the class, then we should!
        out << YAML::BeginMap
            << YAML::Key << "python_name" << YAML::Value << normalized_type_name(c_info->second.name_as_type)
            << YAML::Key << "cpp_name" << YAML::Value << c_info->second.name_as_type.cpp_name;
        
        if (c_info->second.library_name.size() > 0 && c_info->second.library_name.find(".so") == string::npos) {
            out << YAML::Key << "library" << YAML::Value << c_info->second.library_name;
        }

        if (is_collection(c_info->second)) {
            auto container_typename = container_of(c_info->second);
            out << YAML::Key << "is_container_of_cpp" << YAML::Value << container_typename.cpp_name;
            out << YAML::Key << "is_container_of_python" << YAML::Value << normalized_type_name(container_typename);
        }
        
        if (c_info->second.include_file.size() > 0) {
            out << YAML::Key << "include_file" << YAML::Value << c_info->second.include_file;
        }

        if (c_info->second.class_behaviors.size() > 0) {
            out << YAML::Key << "also_behaves_like" << YAML::Value << YAML::BeginSeq;
            for(auto &&c : c_info->second.class_behaviors) {
                out << c;
            }
            out << YAML::EndSeq;
        }

        // Now we need to emit the enums.
        if (c_info->second.enums.size() > 0)
        {
            out << YAML::Key << "enums"
                << YAML::Value
                << YAML::BeginSeq;
            for (auto &&e : c_info->second.enums)
            {
                out << YAML::BeginMap
                    << YAML::Key << "name" << YAML::Value << e.name
                    << YAML::Key << "values" << YAML::Value
                    << YAML::BeginSeq;
                for (auto &&v : e.values)
                {
                    out << YAML::BeginMap
                        << YAML::Key << "name" << YAML::Value << v.first
                        << YAML::Key << "value" << YAML::Value << v.second
                        << YAML::EndMap;
                }
                out << YAML::EndSeq
                    << YAML::EndMap;
            }
            out << YAML::EndSeq;
        }

        // Now we need to emit the methods.
        bool first_method = true;
        for (auto &&meth : c_info->second.methods)
        {
            if (is_understood_method(meth, known_types)) {
                if (first_method) {
                    out << YAML::Key << "methods"
                        << YAML::Value
                        << YAML::BeginSeq;
                    first_method = false;
                }

                auto rtn_type = parse_typename(meth.return_type);
                out << YAML::BeginMap
                    << YAML::Key << "name" << YAML::Value << meth.name
                    << YAML::Key << "return_type" << YAML::Value << rtn_type.cpp_name;

                dump_arguments("arguments", meth.arguments, out);
                dump_arguments("parameter_arguments", meth.parameter_arguments, out);

                if (meth.parameter_type_helper.size() > 0) {
                    out << YAML::Key << "param_helper" << YAML::Value << meth.parameter_type_helper;
                }

                if (meth.param_method_callback.size() > 0) {
                    out << YAML::Key << "param_type_callback" << YAML::Value << meth.param_method_callback;
                }

                out << YAML::EndMap;
            } else {
                auto method_args(referenced_types(meth));
                // Do not warn when return type is void - this is just how we work
                // in a functional world for now (e.g. by design).
                if (meth.return_type.size() != 0) {
                    bool first = true;
                    cerr << "ERROR: Cannot emit method " << c_info->first << "::" << meth.name << " - some types not known: ";
                    for (const auto& arg : method_args) {
                        if (known_types.find(arg) == known_types.end()) {
                            if (!first) {
                                cerr << ", ";
                            }
                            first = false;
                            cerr << arg;
                            failed_types[arg].push_back(c_info->first + "::" + meth.name);
                        }
                    }
                    cerr << endl;
                }
            }
        }

        if (!first_method) {
            out << YAML::EndSeq;
        }

        out << YAML::EndMap;
    }
    out << YAML::EndSeq;

    // Do the helper files
    string atlas_release (getenv("AtlasVersion"));
    metadata_file_finder m_finder (atlas_release);
    emit_helper_files(out, m_finder, parse_release(atlas_release)[0]);

    // Dump some parameters about the running.
    out << YAML::Key << "config";
    out << YAML::BeginMap;

    out << YAML::Key << "atlas_release" << YAML::Value << atlas_release;
    out << YAML::Key << "dataset_types" << YAML::Value;
    out << YAML::BeginSeq;
    out << "PHYS";
    if (atlas_release.find("21") == string::npos) {
        out << "PHYSLITE";
    }

    out << YAML::EndSeq;

    out << YAML::EndMap;

    out << YAML::EndMap;

    // Dump to the output
    cout << out.c_str() << endl;

    // Next, append the metadata file onto the end of this
    fstream metadata_in(m_finder("extra_metadata.yaml"));
    const int buf_size = 4096;
    char buf[buf_size];
    do {
        metadata_in.read(&buf[0], buf_size);
        cout.write(&buf[0], metadata_in.gcount());
    } while (metadata_in.gcount() > 0);     

    // Dump the failed types and their associated methods
    cerr << "Summary of failed types and their methods:" << endl;
    vector<pair<string, vector<string>>> sorted_failed_types(failed_types.begin(), failed_types.end());
    sort(sorted_failed_types.begin(), sorted_failed_types.end(), [](const auto& a, const auto& b) {
        return a.second.size() > b.second.size();
    });
    for (const auto& [type, methods] : sorted_failed_types) {
        cerr << "Type: " << type << ", Number of methods: " << methods.size() << endl;
        for (const auto& method : methods) {
            cerr << "  " << method << endl;
        }
    }
}
