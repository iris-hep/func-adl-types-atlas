#include <gtest/gtest.h>

#include "translate.hpp"

#include <algorithm>

using namespace std;

TEST(t_translate, include_files) {
    auto info = translate_class("xAOD::Jet_v1");

    EXPECT_EQ(info.include_file, "xAODJet/versions/Jet_v1.h");
    EXPECT_EQ(info.library_name, "xAODJet");
}

TEST(t_translate, include_files_no_shared_libs) {
    auto info = translate_class("DataModel_detail::NoBase");

    EXPECT_EQ(info.include_file, "");
}

TEST(t_translate, include_files_neutral_particle) {
    auto info = translate_class("xAOD::NeutralParticleContainer");

    EXPECT_EQ(info.include_file, "xAODTracking/NeutralParticleContainer.h");
    EXPECT_EQ(info.library_name, "xAODTracking");
}

TEST(t_translate, include_files_jet_container) {
    auto info = translate_class("xAOD::JetContainer");

    EXPECT_EQ(info.include_file, "xAODJet/JetContainer.h");
}

TEST(t_translate, include_files_jet_container_v1) {
    auto info = translate_class("xAOD::JetContainer_v1");

    EXPECT_EQ(info.include_file, "xAODJet/JetContainer.h");
}

TEST(t_translate, include_files_DataVector) {
    auto info = translate_class("DataVector<xAOD::Jet_v1>");

    EXPECT_EQ(info.name, "DataVector<xAOD::Jet_v1>");
    EXPECT_EQ(info.include_file, "xAODJet/JetContainer.h");
}

TEST(t_translate, include_files_met) {
    auto info = translate_class("xAOD::MissingETContainer");

    EXPECT_EQ(info.include_file, "xAODMissingET/MissingETContainer.h");
}

TEST(t_translate, include_files_met_v1) {
    auto info = translate_class("xAOD::MissingETContainer_v1");

    EXPECT_EQ(info.include_file, "xAODMissingET/MissingETContainer.h");
}

TEST(t_translate, method_only_once) {
    auto info = translate_class("xAOD::Jet_v1");

    EXPECT_EQ(count_if(info.methods.begin(), info.methods.end(), [](const method_info &m) { return m.name == "pt";}), 1);
}

TEST(t_translate, no_ctor) {
    auto info = translate_class("xAOD::Jet_v1");

    EXPECT_EQ(count_if(info.methods.begin(), info.methods.end(), [](const method_info &m) { return m.name == "Jet_v1";}), 0);
}

TEST(t_translate, truth_particle_methods) {
    auto info = translate_class("xAOD::TruthParticle_v1");
    EXPECT_EQ(count_if(info.methods.begin(), info.methods.end(), [](const method_info &m) { return m.name == "prodVtx";}), 1);
    EXPECT_EQ(count_if(info.methods.begin(), info.methods.end(), [](const method_info &m) { return m.name == "hasProdVtx";}), 1);
}

TEST(t_translate, vector_float) {
    auto info = translate_class("vector<float>");

    EXPECT_EQ(info.name, "std::vector<float>");
    EXPECT_EQ(info.methods.size(), 1);
    EXPECT_EQ(info.methods[0].name, "size");
    EXPECT_EQ(info.inherited_class_names.size(), 0);
    EXPECT_EQ(info.library_name, "");
}

TEST(t_translate, element_link) {
    auto info = translate_class("ElementLink<DataVector<xAOD::Jet_v1> >");

    EXPECT_EQ(info.name, "ElementLink<DataVector<xAOD::Jet_v1>>");
    EXPECT_EQ(info.methods.size(), 1);
    EXPECT_EQ(info.methods[0].name, "isValid");
    EXPECT_EQ(info.inherited_class_names.size(), 0);
    EXPECT_EQ(info.class_behaviors.size(), 1);
    EXPECT_EQ(info.class_behaviors[0], "xAOD::Jet_v1**");
}

TEST(t_translate, enum_calo) {
    auto info = translate_class("xAOD::CaloCluster_v1");

    EXPECT_EQ(info.name, "xAOD::CaloCluster_v1");
    EXPECT_EQ(info.enums.size(), 3);

    // Find one we can look up.
    auto it = find_if(info.enums.begin(), info.enums.end(), [](const enum_info &e) { return e.name == "ClusterSize"; });
    EXPECT_EQ(it == info.enums.end(), false);

    // Check the values of the enum
    EXPECT_EQ(it->values.size(), 18);

    // Check for the value of one, SW_37ele, is 3.
    auto it2 = find_if(it->values.begin(), it->values.end(), [](const pair<string, int> &p) { return p.first == "SW_37ele"; });
    EXPECT_EQ(it2 == it->values.end(), false);
    EXPECT_EQ(it2->second, 3);
}

TEST(t_translate, enum_TLZ) {
    auto info = translate_class("TLorentzVector");

    EXPECT_EQ(info.name, "TLorentzVector");
    EXPECT_EQ(info.enums.size(), 1);

    EXPECT_EQ(info.enums[0].name, "Global");
}

TEST(t_translate, muon_container) {
    // Make sure we can translates something like ElementLink<xAOD::MuonContainer>
    // when we've not already loaded muon container.
    // WARNING: the test can't load muon container anywhere else!

    auto info = translate_class("ElementLink<xAOD::MuonContainer>");

    EXPECT_EQ(info.name, "ElementLink<DataVector<xAOD::Muon_v1>>");
}