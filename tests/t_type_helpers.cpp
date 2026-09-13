#include <gtest/gtest.h>
#include "type_helpers.hpp"

#include "TClass.h"

using namespace std;

// Make sure basic type comes back properly
TEST(t_type_helpers, type_int) {
    auto t = parse_typename("int");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "int");
    EXPECT_EQ(t.is_const, false);
}

TEST(t_type_helpers, type_int_spaces)
{
    auto t1 = parse_typename(" int");
    EXPECT_EQ(t1.type_name, "int");
    auto t2 = parse_typename("int ");
    EXPECT_EQ(t1.type_name, "int");
}

TEST(t_type_helpers, type_int_ptr)
{
    auto t = parse_typename("int*");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "int");
    EXPECT_EQ(t.cpp_name, "int *");
}

TEST(t_type_helpers, type_int_ref) {
    auto t = parse_typename("int&");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "int");
    // We do not care about reference modifiers here.
    EXPECT_EQ(t.cpp_name, "int");
}

TEST(t_type_helpers, type_int_ptr_space) {
    auto t = parse_typename("int *");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "int");
    EXPECT_EQ(t.cpp_name, "int *");
    EXPECT_EQ(t.is_const, false);
}

TEST(t_type_helpers, type_int_ptr_const)
{
    auto t = parse_typename("const int *");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "int");
    EXPECT_EQ(t.cpp_name, "const int *");
    EXPECT_EQ(t.is_const, true);
}

TEST(t_type_helpers, type_int_ptr_const_2)
{
    auto t = parse_typename("int const *");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "int");
    EXPECT_EQ(t.cpp_name, "const int *");
    EXPECT_EQ(t.is_const, true);
}

TEST(t_type_helpers, type_int_const_ptr_space)
{
    auto t = parse_typename("int * const ");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "int");
    EXPECT_EQ(t.cpp_name, "int * const");
    EXPECT_EQ(t.is_const, false);
}

TEST(t_type_helpers, type_int_const_ptr)
{
    auto t = parse_typename("int * const ");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "int");
    EXPECT_EQ(t.cpp_name, "int * const");
    EXPECT_EQ(t.is_const, false);
}

TEST(t_type_helpers, type_int_2ptr)
{
    auto t = parse_typename("int **");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "int");
    EXPECT_EQ(t.cpp_name, "int * *");
    EXPECT_EQ(t.is_const, false);
}

TEST(t_type_helpers, type_int_ptr_const_ptr)
{
    auto t = parse_typename("int * const *");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "int");
    EXPECT_EQ(t.cpp_name, "int * const *");
    EXPECT_EQ(t.is_const, false);
}

TEST(t_type_helpers, type_int_ptr_ptr_const)
{
    auto t = parse_typename("int * * const");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "int");
    EXPECT_EQ(t.cpp_name, "int * * const");
    EXPECT_EQ(t.is_const, false);
}

TEST(t_type_helpers, type_int_const)
{
    auto t = parse_typename("const int");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "int");
    EXPECT_EQ(t.cpp_name, "const int");
    EXPECT_EQ(t.is_const, true);
}

TEST(t_type_helpers, type_int_const_2)
{
    auto t = parse_typename("int const");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "int");
    EXPECT_EQ(t.cpp_name, "const int");
    EXPECT_EQ(t.is_const, true);
}

TEST(t_type_helpers, type_int_const_3)
{
    auto t = parse_typename("int const ");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "int");
    EXPECT_EQ(t.cpp_name, "const int");
    EXPECT_EQ(t.is_const, true);
}

TEST(t_type_helpers, type_int_const_spaces)
{
    auto t = parse_typename("const   int");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "int");
    EXPECT_EQ(t.cpp_name, "const int");
    EXPECT_EQ(t.is_const, true);
}

TEST(t_type_helpers, type_unsigned_int)
{
    auto t = parse_typename("unsigned int");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "unsigned int");
    EXPECT_EQ(t.cpp_name, "unsigned int");
    EXPECT_EQ(t.is_const, false);
}

TEST(t_type_helpers, type_unsigned_int_spaces)
{
    auto t = parse_typename("unsigned   int");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "unsigned int");
    EXPECT_EQ(t.cpp_name, "unsigned int");
    EXPECT_EQ(t.is_const, false);
}

TEST(t_type_helpers, type_const_unsigned_int)
{
    auto t = parse_typename("const unsigned int");

    EXPECT_EQ(t.namespace_list.size(), 0);
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.type_name, "unsigned int");
    EXPECT_EQ(t.cpp_name, "const unsigned int");
    EXPECT_EQ(t.is_const, true);
}

// Simple template
TEST(t_type_helpers, type_vector_int) {
    auto t = parse_typename("vector<int>");

    EXPECT_EQ(t.namespace_list.size(), 1);
    EXPECT_EQ(t.namespace_list[0].type_name, "std");
    EXPECT_EQ(t.template_arguments.size(), 1);
    auto sub_t = t.template_arguments[0];
    EXPECT_EQ(sub_t.type_name, "int");
    EXPECT_EQ(sub_t.template_arguments.size(), 0);
    EXPECT_EQ(sub_t.namespace_list.size(), 0);
    EXPECT_EQ(t.type_name, "vector");
}

TEST(t_type_helpers, type_vector_int_spaces)
{
    auto t = parse_typename("vector< int >");

    EXPECT_EQ(t.template_arguments.size(), 1);
    auto sub_t = t.template_arguments[0];
    EXPECT_EQ(sub_t.type_name, "int");
    EXPECT_EQ(t.type_name, "vector");
}

// Simple typename qualified by namespace
TEST(t_type_helpers, type_namespaced_type) {
    auto t = parse_typename("std::size_t");

    EXPECT_EQ(t.type_name, "size_t");

    EXPECT_EQ(t.namespace_list.size(), 1);
    auto t_ns = t.namespace_list[0];
    EXPECT_EQ(t_ns.type_name, "std");
    EXPECT_EQ(t_ns.template_arguments.size(), 0);
    EXPECT_EQ(t_ns.namespace_list.size(), 0);

    EXPECT_EQ(t.template_arguments.size(), 0);
}


// Simple typename qualified by double namespace
TEST(t_type_helpers, type_namespaced_type_twice) {
    auto t = parse_typename("std::org::size_t");

    EXPECT_EQ(t.type_name, "size_t");
    EXPECT_EQ(t.cpp_name, "std::org::size_t");

    EXPECT_EQ(t.namespace_list.size(), 2);
    auto t_ns = t.namespace_list[0];
    EXPECT_EQ(t_ns.type_name, "std");
    EXPECT_EQ(t_ns.template_arguments.size(), 0);
    EXPECT_EQ(t_ns.namespace_list.size(), 0);

    auto t_ns2 = t.namespace_list[1];
    EXPECT_EQ(t_ns2.type_name, "org");
    EXPECT_EQ(t_ns2.template_arguments.size(), 0);
    EXPECT_EQ(t_ns2.namespace_list.size(), 0);

    EXPECT_EQ(t.template_arguments.size(), 0);
}


// Simple typename qualified by namespace
TEST(t_type_helpers, type_namespaced_buried) {
    auto t = parse_typename("vector<std::size_t>");

    EXPECT_EQ(t.type_name, "vector");

    EXPECT_EQ(t.namespace_list.size(), 1);
    EXPECT_EQ(t.namespace_list[0].type_name, "std");

    EXPECT_EQ(t.template_arguments.size(), 1);
    auto t_arg1 = t.template_arguments[0];
    EXPECT_EQ(t_arg1.type_name, "size_t");
    EXPECT_EQ(t_arg1.namespace_list.size(), 1);

    auto t_ns1 = t_arg1.namespace_list[0];
    EXPECT_EQ(t_ns1.type_name, "std");
    EXPECT_EQ(t_ns1.namespace_list.size(), 0);
    EXPECT_EQ(t_ns1.template_arguments.size(), 0);

    EXPECT_EQ(t_arg1.template_arguments.size(), 0);
}


// Qualified name by template
TEST(t_type_helpers, type_qualified_typename) {
    auto t = parse_typename("vector<float>::size_t");

    EXPECT_EQ(t.cpp_name, "std::vector<float>::size_t");
    EXPECT_EQ(t.type_name, "size_t");
    EXPECT_EQ(t.template_arguments.size(), 0);
    EXPECT_EQ(t.namespace_list.size(), 1);
    EXPECT_EQ(t.namespace_list[0].cpp_name, "std::vector<float>");
}


// Whitespace check
TEST(t_type_helpers, type_whitespace) {
    auto t = parse_typename("vector<float>::size_t ");

    EXPECT_EQ(t.cpp_name, "std::vector<float>::size_t");
}


// Simple typename qualified by namespace
TEST(t_type_helpers, type_multiple_template_args) {
    auto t = parse_typename("vector<std::size_t, std::allocate<std::size_t>>");

    EXPECT_EQ(t.type_name, "vector");

    EXPECT_EQ(t.namespace_list.size(), 1);
    EXPECT_EQ(t.namespace_list[0].type_name, "std");

    EXPECT_EQ(t.template_arguments.size(), 2);
    EXPECT_EQ(t.template_arguments[0].type_name, "size_t");
    EXPECT_EQ(t.template_arguments[1].type_name, "allocate");
}

TEST(t_type_helpers, type_multiple_template_args_with_spaces)
{
    auto t = parse_typename("vector<std::size_t,       std::allocate<std::size_t>>");

    EXPECT_EQ(t.type_name, "vector");

    EXPECT_EQ(t.namespace_list.size(), 1);
    EXPECT_EQ(t.namespace_list[0].type_name, "std");

    EXPECT_EQ(t.template_arguments.size(), 2);
    EXPECT_EQ(t.template_arguments[0].type_name, "size_t");
    EXPECT_EQ(t.template_arguments[1].type_name, "allocate");
    EXPECT_EQ(t.template_arguments[0].namespace_list.size(), 1);
    EXPECT_EQ(t.template_arguments[0].namespace_list[0].type_name, "std");
}

TEST(t_type_helpers, type_multiple_template_args_with_spaces_no_ns)
{
    auto t = parse_typename("vector<std::size_t,       allocate<std::size_t>>");

    EXPECT_EQ(t.type_name, "vector");

    EXPECT_EQ(t.namespace_list.size(), 1);
    EXPECT_EQ(t.namespace_list[0].type_name, "std");

    EXPECT_EQ(t.template_arguments.size(), 2);
    EXPECT_EQ(t.template_arguments[0].type_name, "size_t");
    EXPECT_EQ(t.template_arguments[1].type_name, "allocate");
    EXPECT_EQ(t.template_arguments[1].namespace_list.size(), 0);
}

TEST(t_type_helpers, type_const_on_top)
{
    auto t = parse_typename("const DataVector<xAOD::SlowMuon_v1, DataModel_detail::NoBase>::PtrVector");

    EXPECT_EQ(t.type_name, "PtrVector");
    EXPECT_EQ(t.is_const, true);
}

TEST(t_type_helpers, type_blank) {
    auto t = parse_typename("");

    EXPECT_EQ(t.type_name, "");
}

TEST(t_type_helpers, type_spaces) {
    auto t = parse_typename("  ");

    EXPECT_EQ(t.type_name, "");
}

// Simple typedef checks
TEST(t_type_helpers, typedef_resolve_simple) {
    TClass::GetClass("xAOD::EventInfo"); // Make sure the typedefs are loaded into root!
    EXPECT_EQ(resolve_typedef("xAOD::EventInfo"), "xAOD::EventInfo_v1");
}

TEST(t_type_helpers, typedef_resolve_size_t) {
    TClass::GetClass("xAOD::EventInfo"); // Make sure the typedefs are loaded into root!
    EXPECT_EQ(resolve_typedef("size_t"), "unsigned int");
}

TEST(t_type_helpers, typedef_resolve_blank) {
    EXPECT_EQ(resolve_typedef(""), "");
}

TEST(t_type_helpers, typedef_resolve_simple_ptr) {
    TClass::GetClass("xAOD::EventInfo"); // Make sure the typedefs are loaded into root!
    EXPECT_EQ(resolve_typedef("xAOD::EventInfo*"), "xAOD::EventInfo_v1*");
}

// Simple typedef checks - no change
TEST(t_type_helpers, typedef_resolve_none) {
    TClass::GetClass("xAOD::EventInfo"); // Make sure the typedefs are loaded into root!
    EXPECT_EQ(resolve_typedef("xAOD::EventInfo_v1"), "xAOD::EventInfo_v1");
}

TEST(t_type_helpers, typedef_resolve_utin32) {
    EXPECT_EQ(resolve_typedef("uint32_t"), "unsigned int");
}

TEST(t_type_helpers, typedef_resolve_ulong64)
{
    EXPECT_EQ(resolve_typedef("ULong64_t"), "unsigned long long");
}

TEST(t_type_helpers, typedef_resolve_no_loops) {
    EXPECT_EQ(resolve_typedef("int"), "int");
}

TEST(t_type_helpers, typedef_fixup_methods) {
    method_info mi;
    mi.name = "test_method";
    mi.return_type = "ULong64_t";

    method_arg a1;
    a1.name = "arg1";
    a1.full_typename = "ULong64_t";
    mi.arguments.push_back(a1);

    class_info ci;
    ci.name = "class_1";
    ci.methods.push_back(mi);

    vector<class_info> all_classes;
    all_classes.push_back(ci);

    fixup_type_defs(all_classes);

    EXPECT_EQ(all_classes.size(), 1);
    auto new_ci = all_classes[0];
    EXPECT_EQ(new_ci.methods.size(), 1);
    auto new_mi = new_ci.methods[0];
    EXPECT_EQ(new_mi.return_type, "unsigned long long");
    EXPECT_EQ(new_mi.arguments.size(), 1);
    auto new_a1 = new_mi.arguments[0];
    EXPECT_EQ(new_a1.full_typename, "unsigned long long");
}

TEST(t_type_helpers, is_collection_int) {
    typename_info ti;
    ti.type_name = "int";
    ti.cpp_name = "int";
    EXPECT_EQ(is_collection(ti), false);
}

TEST(t_type_helpers, is_collection_vector) {
    typename_info ti_int;
    ti_int.type_name = "int";
    ti_int.cpp_name = "int";

    typename_info ti;
    ti.type_name = "vector";
    ti.cpp_name = "vector<int>";
    ti.template_arguments.push_back(ti_int);
    EXPECT_EQ(is_collection(ti), true);
}

TEST(t_type_helpers, is_collection_simple_class) {
    typename_info ti;
    ti.cpp_name = "dude";
    ti.cpp_name = "dude";

    class_info ci;
    ci.name = "dude";
    ci.name_as_type = ti;

    EXPECT_EQ(is_collection(ci), false);
}

TEST(t_type_helpers, container_of_begin_end_TIter) {
    typename_info ti;
    ti.cpp_name = "dude";
    ti.cpp_name = "dude";

    class_info ci;
    ci.name = "MyVector";
    ci.name_as_type.type_name = "MyVector";
    ci.name_as_type.cpp_name = "MyVector";

    // Add begin/end vector here
    method_info m_begin;
    m_begin.name = "begin";
    m_begin.return_type = "TIter";
    method_info m_end;
    m_end.name = "end";
    m_end.return_type = "TIter";
    ci.methods.push_back(m_begin);
    ci.methods.push_back(m_end);

    auto t = container_of(ci);

    EXPECT_EQ(t.cpp_name, "TObject");
}

TEST(t_type_helpers, container_of_begin_end_JetVertexConst) {
    typename_info ti;
    ti.cpp_name = "dude";
    ti.cpp_name = "dude";

    class_info ci;
    ci.name = "MyVector";
    ci.name_as_type.type_name = "MyVector";
    ci.name_as_type.cpp_name = "MyVector";

    // Add begin/end vector here
    method_info m_begin;
    m_begin.name = "begin";
    m_begin.return_type = "xAOD::JetConstituentVector::iterator";
    method_info m_end;
    m_end.name = "end";
    m_end.return_type = "xAOD::JetConstituentVector::iterator";
    ci.methods.push_back(m_begin);
    ci.methods.push_back(m_end);

    auto t = container_of(ci);

    EXPECT_EQ(t.cpp_name, "xAOD::JetConstituent *");
}

TEST(t_type_helpers, container_of_vector) {
    class_info ci;
    ci.name = "vector<int>";
    ci.name_as_type = parse_typename("vector<int>");
    
    method_info m_begin;
    m_begin.name = "begin";
    m_begin.return_type = "vector<int>::iterator";
    method_info m_end;
    m_end.name = "end";
    m_end.return_type = "vector<int>::iterator";
    ci.methods.push_back(m_begin);
    ci.methods.push_back(m_end);

    auto t = container_of(ci);

    EXPECT_EQ(t.cpp_name, "int");
}

TEST(t_type_helpers, cpp_string_simple) {
    EXPECT_EQ(typename_cpp_string(parse_typename("int")), "int");
}

TEST(t_type_helpers, cpp_string_simple_pointer) {
    EXPECT_EQ(typename_cpp_string(parse_typename("int*")), "int *");
}

TEST(t_type_helpers, cpp_string_simple_const)
{
    EXPECT_EQ(typename_cpp_string(parse_typename("const int*")), "const int *");
}

TEST(t_type_helpers, cpp_string_simple_ns) {
    EXPECT_EQ(typename_cpp_string(parse_typename("int*")), "int *");
}

TEST(t_type_helpers, cpp_string_simple_template) {
    EXPECT_EQ(typename_cpp_string(parse_typename("vector<int>")), "std::vector<int>");
}


TEST(t_type_helpers, cpp_string_simple_multi_ns) {
    EXPECT_EQ(typename_cpp_string(parse_typename("fork::spoon::int")), "fork::spoon::int");
}

TEST(t_type_helpers, cpp_string_simple_template_args) {
    EXPECT_EQ(typename_cpp_string(parse_typename("vector<int, float>")), "std::vector<int, float>");
}

TEST(t_type_helpers, unqualified_pointer) {
    EXPECT_EQ(unqualified_typename(parse_typename("int*")), "int");
}

TEST(t_type_helpers, unqualified_const) {
    EXPECT_EQ(unqualified_typename(parse_typename("const int")), "int");
}

TEST(t_type_helpers, unqualified_template_args_untouched) {
    EXPECT_EQ(unqualified_typename(parse_typename("const vector<int*>")), "std::vector<int *>");
}

TEST(t_type_helpers, unqualified_template_args_untouched_with_const) {
    EXPECT_EQ(unqualified_typename(parse_typename("vector<const int*>")), "std::vector<const int *>");
}

TEST(t_type_helpers, unqualified_const_DataVector) {
    EXPECT_EQ(unqualified_typename(parse_typename("const DataVector<xAOD::SlowMuon_v1, DataModel_detail::NoBase>::PtrVector")), "DataVector<xAOD::SlowMuon_v1, DataModel_detail::NoBase>::PtrVector");
}

TEST(t_type_helpers, understood_simple_yes) {
    EXPECT_EQ(is_understood_type("hi", set<string>({"hi"})), true);
}

TEST(t_type_helpers, understood_simple_no) {
    EXPECT_EQ(is_understood_type("hi", set<string>()), false);
}

TEST(t_type_helpers, understood_simple_vector_yes) {
    EXPECT_EQ(is_understood_type("vector<hi>", set<string>({"std::vector<hi>"})), true);
}

TEST(t_type_helpers, understood_simple_vector_no) {
    EXPECT_EQ(is_understood_type("vector<hi>", set<string>({"hit"})), false);
}

// TEST(t_type_helpers, understood_simple_DataVector_yes) {
//     EXPECT_EQ(is_understood_type("DataVector<hi>", set<string>({"hi"})), true);
// }

// TEST(t_type_helpers, understood_simple_DataVector_no) {
//     EXPECT_EQ(is_understood_type("DataVector<hi>", set<string>({"hit"})), false);
// }

// TEST(t_type_helpers, understood_simple_ElementLink_yes) {
//     EXPECT_EQ(is_understood_type("ElementLink<hi>", set<string>({"hi"})), true);
// }

// TEST(t_type_helpers, understood_simple_ElementLink_no) {
//     EXPECT_EQ(is_understood_type("ElementLink<hi>", set<string>({"hit"})), false);
// }

TEST(t_type_helpers, understood_method_simple) {
    method_info m;
    m.name = "fork";
    m.return_type = "int";

    EXPECT_EQ(is_understood_method(m, set<string>({"int"})), true);
}

TEST(t_type_helpers, understood_method_unkown) {
    method_info m;
    m.name = "fork";
    m.return_type = "U";

    EXPECT_EQ(is_understood_method(m, set<string>({"int"})), false);
}

TEST(t_type_helpers, understood_method_template) {
    method_info m;
    m.name = "fork";
    m.return_type = "U";

    method_arg ma;
    ma.name = "return_type";
    ma.raw_typename = "cpp_type<U>";
    ma.full_typename = "cpp_type<U>";
    m.parameter_arguments.push_back(ma);

    EXPECT_EQ(is_understood_method(m, set<string>({"int"})), true);
}

TEST(t_type_helpers, py_type_simple_type) {
    auto t = py_typename("int");
    EXPECT_EQ(t.type_name, "int");
}

TEST(t_type_helpers, py_type_vector) {
    auto t = py_typename("vector<int>");
    EXPECT_EQ(t.cpp_name, "Iterable<int>");
}

TEST(t_type_helpers, py_type_element_link) {
    auto t = py_typename("ElementLink<DataVector<int>>");
    EXPECT_EQ(t.cpp_name, "int *");
}

TEST(t_type_helpers, py_type_dv) {
    auto t = py_typename("DataVector<int>");
    EXPECT_EQ(t.cpp_name, "Iterable<int>");
}

TEST(t_type_helpers, py_type_dv_el) {
    auto t = py_typename("vector<ElementLink<DataVector<int>>>");
    EXPECT_EQ(t.cpp_name, "Iterable<int *>");
}
// TODO: Remove py_typename, not used.

TEST(t_type_helpers, normalized_int) {
    EXPECT_EQ(normalized_type_name("int"), "int");
}

TEST(t_type_helpers, normalized_vector) {
    EXPECT_EQ(normalized_type_name("vector<float>"), "std.vector_float_");
}

TEST(t_type_helpers, normalized_iterable) {
    EXPECT_EQ(normalized_type_name("Iterable<float>"), "Iterable[float]");
}

TEST(t_type_helpers, normalized_vector_ns) {
    EXPECT_EQ(normalized_type_name("vector<ROOT::Fit::ParameterSettings>"), "std.vector_ROOT_Fit_ParameterSettings_");
}

TEST(t_type_helpers, normalized_vector_front_ns) {
    EXPECT_EQ(normalized_type_name("std::vector<double>"), "std.vector_float_");
}

TEST(t_type_helpers, normalized_vector_space)
{
    EXPECT_EQ(normalized_type_name("vector<unsigned char>"), "std.vector_int_");
}

TEST(t_type_helpers, normalized_elPtr)
{
    EXPECT_EQ(normalized_type_name("ElementLink<DataVector<xAOD::Truth>>"), "ElementLink_DataVector_xAOD_Truth__");
}

TEST(t_type_helpers, normalized_cpp_type) {
    EXPECT_EQ(normalized_type_name("cpp_type<U>"), "cpp_type[U]");
}

TEST(t_type_helpers, class_enums_one) {
    enum_info e1;
    e1.name = "fork";

    class_info ci;
    ci.name = "class_1";
    ci.name_as_type = parse_typename("xAOD::class_1");
    ci.enums.push_back(e1);

    auto r = class_enums(ci);

    EXPECT_EQ(r.size(), 1);

    EXPECT_EQ(r[0], "xAOD::class_1::fork");
}

TEST(t_type_helpers, class_enums_zero) {

    class_info ci;
    ci.name = "class_1";
    auto r = class_enums(ci);

    EXPECT_EQ(r.size(), 0);
}


TEST(t_type_helpers, parent_class_three) {
    auto t = parse_typename("std::org::size_t");
    auto p = parent_class(t);

    EXPECT_EQ(p.type_name, "org");
    EXPECT_EQ(p.namespace_list.size(), 1);
    EXPECT_EQ(p.namespace_list[0].type_name, "std");
    EXPECT_EQ(p.template_arguments.size(), 0);
    EXPECT_EQ(p.is_const, false);
    EXPECT_EQ(p.cpp_name, "std::org");
}

TEST(t_type_helpers, parent_class_const_ptr) {
    auto t = parse_typename("const std::org::size_t *");
    auto p = parent_class(t);

    EXPECT_EQ(p.type_name, "org");
    EXPECT_EQ(p.namespace_list.size(), 1);
    EXPECT_EQ(p.namespace_list[0].type_name, "std");
    EXPECT_EQ(p.cpp_name, "std::org");
    EXPECT_EQ(p.template_arguments.size(), 0);
    EXPECT_EQ(p.is_const, false);
}

TEST(t_type_helpers, parent_class_none) {
    auto t = parse_typename("size_t");
    try {
        parent_class(t);
        FAIL() << "Expected std::invalid_argument";
    } catch (std::invalid_argument const & err) {
        EXPECT_EQ(err.what(), std::string("No parent class for size_t"));
    } catch (...) {
        FAIL() << "Expected std::invalid_argument";
    }   
}

TEST(t_type_helpers, parent_class_vector) {
    auto t = parse_typename("std::vector<int>::size_t");
    auto p = parent_class(t);
    auto expected = parse_typename("std::vector<int>");

    EXPECT_EQ(p.type_name, expected.type_name);
    EXPECT_EQ(p.namespace_list.size(), expected.namespace_list.size());
    EXPECT_EQ(p.cpp_name, expected.cpp_name);
    EXPECT_EQ(p.template_arguments.size(), expected.template_arguments.size());
}

TEST(t_type_helpers, referenced_types_simple)
{
    auto t = parse_typename("int");
    auto r = type_referenced_types(t);
    EXPECT_EQ(r.size(), 0);
}

TEST(t_type_helpers, referenced_types_blank)
{
    auto t = parse_typename("");
    auto r = type_referenced_types(t);
    EXPECT_EQ(r.size(), 0);
}

TEST(t_type_helpers, referenced_types_vector)
{
    auto t = parse_typename("std::vector<int>");
    auto r = type_referenced_types(t);
    EXPECT_EQ(r.size(), 1);
    EXPECT_EQ(r.find("int") != r.end(), true);
}

TEST(t_type_helpers, referenced_types_nested_vector)
{
    auto t = parse_typename("std::vector<ElementLink<int>>");
    auto r = type_referenced_types(t);
    EXPECT_EQ(r.size(), 2);
    EXPECT_EQ(r.find("int") != r.end(), true);
    EXPECT_EQ(r.find("ElementLink<int>") != r.end(), true);
}
