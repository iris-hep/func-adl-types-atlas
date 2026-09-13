#include "type_helpers.hpp"
#include "class_info.hpp"
#include "util_string.hpp"

#include "TROOT.h"
#include "TClassTable.h"

#include <algorithm>
#include <regex>
#include <iterator>
#include <sstream>
#include <boost/algorithm/string.hpp>

using namespace std;

///
// Return a type name without any qualifiers
//
string type_name(const string &qualified_type_name)
{
    string result(qualified_type_name);
    auto i = result.rfind(':');
    if (i != std::string::npos) {
        result = result.substr(i+1);
    }
    return result;
}

///
// Take list of types names and clean them up
//
vector<string> type_name(const vector<std::string> &qualified_type_names)
{
    vector<string> result;
    transform(qualified_type_names.begin(), qualified_type_names.end(),
                back_insert_iterator(result),
                [](const string &a) {return type_name(a);});
    return result;
}

set<string> type_name(const set<string> &qualified_type_names)
{
    auto v = type_name(vector<string>(qualified_type_names.begin(), qualified_type_names.end()));
    return set<string>(v.begin(), v.end());
}

// Strip out leading "const" and post modifiers (like ptr, etc.)
string unqualified_type_name(const string &full_type_name)
{
    auto t = parse_typename(full_type_name);
    return unqualified_typename(t);
}

// Find all classes that inherit from a given class name
vector<string> all_that_inherit_from(const string &c_name)
{
    TClassTable::Init();
    while (auto m_info = TClassTable::Next()) {
        cout << "** " << m_info << endl;
    }

    TIter next(gROOT->GetListOfClasses());
    set<string> results;
    while (auto c_info = static_cast<TClass *>(next()))
    {
        cout << " -> " << c_info->GetName() << endl;
        if (c_info->InheritsFrom(c_name.c_str()))
        {
            results.insert(c_info->GetName());
            cout << "    *** inherits!" << endl;
        }
    }
    return vector<string>(results.begin(), results.end());
}

map<string, string> g_typedef_map;

void build_typedef_map() {
    if (g_typedef_map.size() > 0)
        return;

    // Build the forward map just once
	TIter i_typedef (gROOT->GetListOfTypes(true));
	int junk = gROOT->GetListOfTypes()->GetEntries();
	TDataType *typedef_spec;
	while ((typedef_spec = static_cast<TDataType*>(i_typedef.Next())) != 0)
	{
		string typedef_name = typedef_spec->GetName();
		string base_name = typedef_spec->GetFullTypeName();

        if (typedef_name != base_name) {
            g_typedef_map[typedef_name] = base_name;
        }
    }

    // Add a few special ones to keep the system working
    g_typedef_map["ULong64_t"] = "unsigned long long";
    g_typedef_map["uint32_t"] = "unsigned int";
    g_typedef_map["Double_t"] = "double";
    g_typedef_map["Float_t"] = "float";
    g_typedef_map["Int_t"] = "int";
    g_typedef_map["Long64_t"] = "long long";
    g_typedef_map["Long_t"] = "long";
    g_typedef_map["Bool_t"] = "bool";
    g_typedef_map["UInt_t"] = "unsigned int";
    g_typedef_map["ULong_t"] = "unsigned long";
    g_typedef_map["ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>::Scalar"] = "double";

    // Some class typedef's that ROOT RTTI can't seem to "get".
    g_typedef_map["xAOD::CaloCluster_v1::CaloSample"] = "CaloSampling::CaloSample";
    g_typedef_map["xAOD::CaloCluster_v1::flt_t"] = "float";
}

// Build a list of type mapping
map<string, vector<string>> root_typedef_map()
{
    // Build the forward map
    build_typedef_map();

    // Use that to build the backwards map.
    map<string, vector<string>> typedef_back_map;
    for (auto &item : g_typedef_map)
    {
        typedef_back_map[item.second].push_back(item.first);
    }

    return typedef_back_map;
}

// From typedefs, return resolved typedefs.
// Do not call until all libraries have been loaded!
string resolve_typedef(const string &c_name) {
    // Check the typedef name
    build_typedef_map();
    auto t = parse_typename(c_name);
    if (t.type_name == "") {
        return "";
    }

    // If it is size_t then we are going to take a shortcut
    // For whatever reason, ROOT never seems to know about this,
    // and this is a strictly C++ type.
    if (t.type_name == "size_t") {
        return "unsigned int";
    }

    string result = unqualified_typename(t);
    bool done = false;
    while (!done) {
        auto td = g_typedef_map.find(result);
        if (td == g_typedef_map.end()) {
            done = true;
        } else {
            result = td->second;
        }
    }

    // Last is to normalize, if possible, with a class name
    auto c = get_tclass(result);
    if (c != nullptr) {
        result = c->GetName();
    }

    // Reapply the various modifiers
    if (t.is_const) {
        result = "const " + result;
    }
    for (auto &p : t.p_info) {
        result = result + "*";
        if (p.is_const) {
            result = result + " const";
        }
    }

    return result;
}


///
// Look through the list of typedefs, and add aliases for any classes
// we've already seen.
//
void fixup_type_aliases(vector<class_info> &classes)
{    
    // Build a typedef backwards mapping
    map<string, vector<string>> typedef_back_map = root_typedef_map();

    // Loop through all the classes we are looking at to see if there is an alias.
    for (auto &&c : classes)
    {
        if (typedef_back_map.find(c.name) != typedef_back_map.end()) {
            c.aliases = typedef_back_map[c.name];
        }
    }
}

// Find referenced arguments in methods and resolve any typedefs in there
void fixup_type_defs(vector<class_info> &classes)
{    
    // Loop through all the classes we are looking at to see if there is an alias.
    for (auto &&c : classes)
    {
        for (auto &&m : c.methods)
        {
            m.return_type = resolve_typedef(m.return_type);
            for (auto &&a : m.arguments)
            {
                a.full_typename = resolve_typedef(a.full_typename);
            }
        }
    }
}

std::regex _multi_space_regex("\\s+");

// Parse a horrendous C++ typename into its various pieces.
//
// "int"
// class_name::size_type
// class_name<t1,t2>::class_name2<t3, t4>::size_type
typename_info parse_typename(const string &type_name)
{
    typename_info result;
    result.is_const = false;
    result.cpp_name = "";
    bool top_level_is_const = false;

    // Simple bail if this is a blank.
    if (trim(type_name).size() == 0) {
        return result;
    }

    // Walk through, parsing.
    int ns_depth = 0;
    size_t t_index = 0;
    string name;
    while (t_index < type_name.size()) {
        switch (type_name[t_index]) {
            case '<':
                if (ns_depth == 0) {
                    result.type_name = boost::trim_copy(name);
                    name = "";
                } else {
                    name += type_name[t_index];
                }
                ns_depth++;
                break;
            
            case '>':
                ns_depth--;
                if (ns_depth == 0) {
                    result.template_arguments.push_back(parse_typename(name));
                    name = "";
                } else {
                    name += type_name[t_index];
                }
                break;
            
            case ',':
                if (ns_depth == 1) {
                    result.template_arguments.push_back(parse_typename(name));
                    name = "";
                } else {
                    name += type_name[t_index];
                }
                break;

            case ':':
                if (ns_depth == 0) {
                    if (name.size() > 0) {
                        result.namespace_list.push_back(parse_typename(name));
                        name = "";
                    }
                    else
                    {
                        if (is_collection(result) && result.namespace_list.empty()) {
                            typename_info std_ns;
                            std_ns.type_name = "std";
                            std_ns.cpp_name = "std";
                            result.namespace_list.insert(result.namespace_list.begin(), std_ns);
                        }
                        result.cpp_name = typename_cpp_string(result);
                        typename_info nested_ns = result;
                        result = typename_info();

                        result.cpp_name = nested_ns.cpp_name;
                        // nested_ns.cpp_name = nested_ns.cpp_name.substr(0, t_index);

                        result.namespace_list.push_back(nested_ns);
                    }
                    t_index++; // Get past the double colon
                } else {
                    name += type_name[t_index];
                }
                break;

            case '*':
                if (ns_depth == 0) {
                    pointer_info p;
                    p.is_const = false;
                    result.p_info.push_back(p);
                } else {
                    name += type_name[t_index];
                }
                break;
            
            case ' ':
                if (ns_depth == 0) {
                    auto n1 = boost::trim_copy(name);
                    if (boost::ends_with(n1, "const")) {
                        if (result.p_info.size() > 0) {
                            result.p_info.back().is_const = true;
                        } else {
                            top_level_is_const = true;
                        }
                        name = boost::trim_copy(name.substr(0, name.size() - 5));
                        break;
                    }
                }
                if (!name.empty() && name.back() != ' ') {
                    name += ' ';
                }
                break;

            case '&':
                // We don't care about reference modifiers for this work.
                if (ns_depth != 0) {
                    name += type_name[t_index];
                }
                break;

            default:
                name += type_name[t_index];
                break;
        }
        t_index++;
    }
    if (boost::ends_with(name, "const")) {
        if (result.p_info.size() > 0)
        {
            result.p_info.back().is_const = true;
        }
        else
        {
            top_level_is_const = true;
        }
        name = name.substr(0, name.size() - 5);
    }
    if (name.size() > 0 && result.type_name.size() == 0) {
        boost::trim(name);
        result.type_name = name;
        name = "";
    }
    result.is_const = top_level_is_const;

    if (is_collection(result) && result.namespace_list.empty()) {
        typename_info std_ns;
        std_ns.type_name = "std";
        std_ns.cpp_name = "std";
        result.namespace_list.insert(result.namespace_list.begin(), std_ns);
    }

    // Get the full type name right, and properly parsed.
    result.cpp_name = typename_cpp_string(result);

    return result;
}

// Returns the type info for the first class or inherited class that
// has name as the name as a class. Can't climb the inheritance tree far,
// but it will try.
typename_info get_first_class(const class_info &c, const string &name) {
    // Check the class itself
    if (c.name_as_type.type_name == name) {
        return c.name_as_type.template_arguments[0];
    }

    // Go after the inherited item
    for (auto &&i_name : c.inherited_class_names)
    {
        auto t_info = parse_typename(i_name);
        if (t_info.type_name ==  name) {
            return t_info.template_arguments[0];
        }
    }

    return typename_info();
}

// Dump out the typename as fully qualified, but normalized
// (rather than just C++).
string normalized_type_name(const typename_info &ti)
{
    ostringstream full_name;
    full_name << ti;
    string result(full_name.str());

    // Iterable and cpp_type should not be touched as it is understood as a python template
    // (the only template that is understood).
    if (ti.type_name == "Iterable" || ti.type_name == "cpp_type") {
        return result;
    }

    // // If we are looking at an Element link, we also must do some fancy footwork
    // if (ti.type_name == "ElementLink" && ti.template_arguments[0].type_name == "DataVector") {
    //     return normalized_type_name(ti.template_arguments[0].template_arguments[0]);
    // }

    // Walk through the rest sensibly converting the typename.
    int bracket_depth = 0;
    for(int i = 0; i < result.size(); i++) {
        switch (result[i])
        {
        case '[':
            result[i] = '_';
            bracket_depth++;
            break;
        case ']':
            result[i] = '_';
            bracket_depth--;
            break;
        
        case '.':
            if (bracket_depth > 0) {
                result[i] = '_';
            }
            break;

        case ' ':
        case ',':
            result[i] = '_';
            break;

        default:
            break;
        }
    }
    replace(result.begin(), result.end(), '[', '_');
    replace(result.begin(), result.end(), ']', '_');
    return result;
}

// Return a C++ type that has been normalized.
string normalized_type_name(const string &ti)
{
    return normalized_type_name(parse_typename(ti));
}

// Figure out if this type is a container that can be
// iterated over using standard for like C++ syntax
// (vector, etc.)
bool is_collection(const typename_info &ti) {
    if (ti.type_name == "vector" & ti.template_arguments.size() > 0) {
        return true;
    }
    return false;
}

// Look at the class see if this is a vector of some sort that
// can be iterated over. We also only return true if we are sure we can get
// an actual type out of (in short - container_of will return successfully).
bool is_collection(const class_info &ci) {
    // Look to see if there is a begin/end method. If so, then we will
    // assume that is good to go!

    try {
        container_of(ci);
        return true;
    } catch (std::runtime_error &e) {
        return false;
    }
}

map<string, string> _g_container_iterator_specials = {
    {"TIter", "TObject"},
    {"xAOD::JetConstituentVector::iterator", "xAOD::JetConstituent*"},
};

typename_info container_of(const typename_info &ti) {
    throw runtime_error("ops");
}

// Given this is a container, as above, figure out
// what it is containing.
typename_info container_of(const class_info &ci) {
    // If this is a vector object, then we can grab from the argument
    if (ci.name_as_type.type_name == "vector") {
        return ci.name_as_type.template_arguments[0];
    }
    if (ci.name_as_type.type_name == "DataVector") {
        return ci.name_as_type.template_arguments[0];
    }

    // If there is a begin/end object, lets lift that out.
    if (has_methods(ci, {"begin", "end"})) {
        auto rtn_type_name = get_method(ci, "begin")[0].return_type;


        auto rtn_type = _g_container_iterator_specials.find(rtn_type_name);
        if (rtn_type != _g_container_iterator_specials.end()) {
            return parse_typename(rtn_type->second);
        }

        auto &&c = get_tclass(rtn_type_name);
        if (c != nullptr) {
            return parse_typename(c->GetName());
        } else {
            throw runtime_error("Unable to find container type for iterator type " + rtn_type_name + " for container " + ci.name);
        }
    }

    throw runtime_error("Do not know how to find container type for class " + ci.name);
}

// Return a list of the C++ types that this type refers to in its
// type name. This will unwind all the template arguments and return them as a set.
// Does not return the typename that is itself.
set<string> type_referenced_types(const typename_info &t) {
    set<string> result;
    for (auto &&t_arg : t.template_arguments) {
        result.insert(unqualified_typename(t_arg));
        auto t_args = type_referenced_types(t_arg);
        result.insert(t_args.begin(), t_args.end());
    }
    return result;
}

// Return the C++ type as unqualified.
std::string unqualified_typename(const typename_info &ti)
{
    typename_info n_ti = ti;
    n_ti.is_const = false;
    n_ti.p_info.clear();
    return typename_cpp_string(n_ti);
}

// Return the C++ type in a standard format
std::string typename_cpp_string(const typename_info &ti)
{
    bool first = true;
    ostringstream stream;

    if (ti.is_const) {
        stream << "const ";
    }

    for (auto &&ns : ti.namespace_list)
    {
        if (!first)
            stream << "::";
        first = false;
        stream << ns.cpp_name;
    }
    if (!first)
        stream << "::";

    stream << ti.type_name;


    // And any template arguments
    first = true;
    for (auto &&t_arg : ti.template_arguments)
    {
        if (first) {
            stream << "<";
            first = false;
        } else {
            stream << ", ";
        }
        stream << t_arg.cpp_name;
    }
    if (!first) {
        stream << ">";
    }

    for (auto &p : ti.p_info) {
        stream << " *";
        if (p.is_const)
        {
            stream << " const";
        }
    }

    return stream.str();
}

// See if we can handle this type:
// Raw types in the known list are ok
// Vectors of known types are ok
// Element links of known types are ok
// DataVectors of known types are ok.
bool is_understood_type(const string &t_name, const set<std::string> &known_types)
{
    // First, check to see if the type is in the list of known types
    auto t = parse_typename(t_name);
    return is_understood_type(t, known_types);
}

bool is_understood_type(const typename_info &t, const set<std::string> &known_types)
{
    // First, check to see if the type is in the list of known types
    auto uq_name = unqualified_typename(t);
    if (known_types.find(uq_name) != known_types.end()) {
        return true;
    }

    return false;
}

// Can we emit this class?
//  1. The method must return something (e.g. it can't be void)
//  2. All types used in the method must be known.
bool is_understood_method(const method_info &meth, const set<string> &classes_to_emit) {
    // Make sure returns something.
    if (meth.return_type.size() == 0) {
        return false;
    }

    // Get all referenced types, and make sure we know about those types.
    auto method_types = referenced_types(meth);
    set<string> method_types_set(method_types.begin(), method_types.end());

    // Next, look at the template arguments and see if they define any special
    // types that we should "allow" for just this method.
    set<string> known_classes(classes_to_emit);
    for (auto &&t_arg : meth.parameter_arguments)
    {
        auto t_parsed = parse_typename(t_arg.full_typename);
        if (t_parsed.type_name == "cpp_type") {
            if (t_parsed.template_arguments.size() != 1) {
                throw runtime_error("Method " + meth.name + " uses a template argument of cpp_type and doesn't have exactly one template argument");
            }
            
            known_classes.insert(t_parsed.template_arguments[0].cpp_name);
        }
    }
    

    for (auto &&m_type: method_types_set) {
        if (!is_understood_type(m_type, known_classes)) {
            return false;
        }
    }
    return true;
}

// Convert C++ types into python types for return
typename_info py_typename(const string &t_name){
    return py_typename(parse_typename(t_name));
}

// Convert C++ types into python types, with some special
// handling.
typename_info py_typename(const typename_info &t)
{
    // If the type is either vector or DataVector, we
    // just convert it to Iterable.
    if (t.type_name == "vector" || t.type_name == "DataVector") {
        typename_info result(t);
        result.type_name = "Iterable";
        result.namespace_list.clear();
        result.template_arguments[0] = py_typename(t.template_arguments[0]);
        result.cpp_name = typename_cpp_string(result);
        return result;
    }

    // If this is an element link, then we need to convert this to a pointer.
    // Element links can only be taken of data vectors, and we are interested in their
    // inside type.
    if (t.type_name == "ElementLink") {
        if (t.template_arguments[0].type_name != "DataVector") {
            throw runtime_error("Found element link type, but not of a DataVector: '" + t.cpp_name + "'.");
        }
        // Lift from twice deep - this is a link to a DataVector.
        typename_info result = py_typename(t.template_arguments[0].template_arguments[0]);
        pointer_info p;
        p.is_const = false;
        result.p_info.push_back(p);
        result.cpp_name = typename_cpp_string(result);
        return result;
    }
    return t;
}

// Look for all defined enums in the class and return
// their fully qualified names
vector<string> class_enums(const class_info &c)
{
    vector<string> result;

    // Iterate over the enums in the class
    for (const auto &enum_info : c.enums) {
        // Add the fully qualified name of the enum to the result vector
        result.push_back(unqualified_typename(c.name_as_type) + "::" + enum_info.name);
    }

    return result;
}

// Determine the parent class our surrounding
// namespace.
typename_info parent_class(const typename_info &ti)
{
    // Make sure this is going to work!
    if (ti.namespace_list.size() == 0) {
        throw invalid_argument("No parent class for " + ti.cpp_name);
    }

    // Pop ourselves one up!
    typename_info result = ti.namespace_list.back();

    // Re-insert all the extra namespace info.
    result.namespace_list.insert(result.namespace_list.begin(), ti.namespace_list.begin(), ti.namespace_list.end() - 1);

    // Reset pointer and const-ness
    result.is_const = false;

    // And update the nickname
    result.cpp_name = typename_cpp_string(result);

    return result;
}

// Get a TClass pointer, but protect against fetching
// internal classes.
TClass *get_tclass(const string &name)
{
    // Any type that looks like it has something internal,
    // like __gnu_cxx::xxxx. ROOT has some issue with these,
    // printing out error messages to cout (rather than cerr),
    // which pollutes out output, of course. We don't need them,
    // so avoid them.
    if (name.find("__") != string::npos) {
        return nullptr;
    }

    return TClass::GetClass(name.c_str());
}