#include <gtest/gtest.h>
#include "class_info.hpp"
#include "type_helpers.hpp"

using namespace std;

// Make sure basic type comes back properly
TEST(t_class_info, referenced_class_inherited_class) {
    class_info ci;
    ci.name = "SubClass";
    ci.name_as_type = parse_typename(ci.name);

    ci.inherited_class_names.push_back("fork1");
    ci.inherited_class_names.push_back("fork2");

    auto ref_list = referenced_types(ci);
    EXPECT_EQ(set<string>(ref_list.begin(), ref_list.end()), set<string>({"SubClass", "fork1", "fork2"}));
}


// A simple top level template
TEST(t_class_info, referenced_class_template_argument) {
    class_info ci;
    ci.name = "vector<SubClass>";
    ci.name_as_type = parse_typename(ci.name);

    auto ref_list = referenced_types(ci);
    EXPECT_EQ(set<string>(ref_list.begin(), ref_list.end()), set<string>({"SubClass", "std::vector<SubClass>"}));
}

TEST(t_class_info, referenced_typeinfo_pointer) {
    auto tn = parse_typename("int*");

    auto ref_list = referenced_types(tn);
    EXPECT_EQ(set<string>(ref_list.begin(), ref_list.end()), set<string>({"int"}));
}

TEST(t_class_info, referenced_typeinfo_pointer_const) {
    auto tn = parse_typename("const int*");

    auto ref_list = referenced_types(tn);
    EXPECT_EQ(set<string>(ref_list.begin(), ref_list.end()), set<string>({"int"}));
}

// Template argument
TEST(t_class_info, referenced_typeinfo_template) {
    auto tn = parse_typename("vector<SubClass>");

    auto ref_list = referenced_types(tn);
    EXPECT_EQ(set<string>(ref_list.begin(), ref_list.end()), set<string>({"SubClass", "std::vector<SubClass>"}));
}

TEST(t_class_info, referenced_typeinfo_template_with_namespace) {
    auto tn = parse_typename("vector<std::SubClass>");

    auto ref_list = referenced_types(tn);
    EXPECT_EQ(set<string>(ref_list.begin(), ref_list.end()), set<string>({"std::SubClass", "std::vector<std::SubClass>"}));
}

TEST(t_class_info, referenced_typeinfo_qualified_name) {
    auto tn = parse_typename("vector<std::SubClass>::size_t");

    auto ref_list = referenced_types(tn);
    EXPECT_EQ(set<string>(ref_list.begin(), ref_list.end()), set<string>({"std::vector<std::SubClass>::size_t"}));
}

TEST(t_class_info, referenced_element_link) {
    auto tn = parse_typename("ElementLink<DataVector<xAOD::TruthVertex>>");

    auto ref_list = referenced_types(tn);
    EXPECT_EQ(set<string>(ref_list.begin(), ref_list.end()), set<string>({"ElementLink<DataVector<xAOD::TruthVertex>>", "xAOD::TruthVertex"}));
}

TEST(t_class_info, referenced_method_return_type) {
    method_info mi;
    mi.name = "do_it";
    mi.return_type = "vector<std::SubClass>";

    auto ref_list = referenced_types(mi);
    EXPECT_EQ(set<string>(ref_list.begin(), ref_list.end()), set<string>({"std::vector<std::SubClass>", "std::SubClass"}));
}

TEST(t_class_info, referenced_method_args) {
    method_info mi;
    mi.name = "do_it";
    mi.return_type = "vector<std::SubClass>";

    method_arg ma;
    ma.name = "a1";
    ma.raw_typename = "xAOD::Jet";
    ma.full_typename = "xAOD::Jet";
    mi.arguments.push_back(ma);

    auto ref_list = referenced_types(mi);
    EXPECT_EQ(set<string>(ref_list.begin(), ref_list.end()), set<string>({"std::vector<std::SubClass>", "std::SubClass", "xAOD::Jet"}));
}

TEST(t_class_info, referenced_typename_ignore_integers) {
    auto tn = parse_typename("Eigen::DenseBase<Eigen::Matrix<double,6,1,0,6,-1> >");

    auto ref_list = referenced_types(tn);
    EXPECT_EQ(set<string>(ref_list.begin(), ref_list.end()), set<string>({
        "Eigen::DenseBase<Eigen::Matrix<double, 6, 1, 0, 6, -1>>",
        "Eigen::Matrix<double, 6, 1, 0, 6, -1>",
        "double"}));
}

TEST(t_class_info, referenced_typename_deep_inside) {
    auto tn = parse_typename("unique_ptr<DataVector<xAOD::Jet_v1,DataVector<xAOD::IParticle,DataModel_detail::NoBase> >::base_value_type>");

    auto ref_list = referenced_types(tn);
    EXPECT_EQ(set<string>(ref_list.begin(), ref_list.end()), set<string>({
        "unique_ptr<DataVector<xAOD::Jet_v1, DataVector<xAOD::IParticle, DataModel_detail::NoBase>>::base_value_type>",
        "DataVector<xAOD::Jet_v1, DataVector<xAOD::IParticle, DataModel_detail::NoBase>>::base_value_type",
        }));
}

TEST(t_class_info, convert_double_to_float) {
    auto tn = parse_typename("vector<double>");
    ostringstream out;
    out << tn;
    EXPECT_EQ(out.str(), "std.vector[float]");
}

TEST(t_class_info, convert_unsigned_char)
{
    auto tn = parse_typename("vector<unsigned char>");
    ostringstream out;
    out << tn;
    EXPECT_EQ(out.str(), "std.vector[int]");
}

TEST(t_class_info, has_methods_no)
{
    method_info mi;
    mi.name = "begin";

    class_info ci;
    ci.methods.push_back(mi);

    EXPECT_EQ(has_methods(ci, {"end"}), false);
}

TEST(t_class_info, has_methods_yes) {
    method_info mi;
    mi.name = "end";

    class_info ci;
    ci.methods.push_back(mi);

    EXPECT_EQ(has_methods(ci, {"end"}), true);
}

TEST(t_class_info, has_methods_multiple) {
    method_info mi_1;
    mi_1.name = "begin";
    method_info mi_2;
    mi_2.name = "end";

    class_info ci;
    ci.methods.push_back(mi_1);
    ci.methods.push_back(mi_2);

    EXPECT_EQ(has_methods(ci, {"end", "begin"}), true);
}

TEST(t_class_info, has_methods_multiple_no) {
    method_info mi_1;
    mi_1.name = "begin";
    method_info mi_2;
    mi_2.name = "end1";

    class_info ci;
    ci.methods.push_back(mi_1);
    ci.methods.push_back(mi_2);

    EXPECT_EQ(has_methods(ci, {"end", "begin"}), false);
}

TEST(t_class_info, get_methods_yes) {
    method_info mi;
    mi.name = "end";

    class_info ci;
    ci.methods.push_back(mi);

    auto a = get_method(ci, "end");

    EXPECT_EQ(a.size(), 1);
    EXPECT_EQ(a[0].name, "end");
}
