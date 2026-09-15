#include "helper_files.hpp"
#include <string>
#include <fstream>
#include <vector>

using namespace std;


struct helper_file {
    // Name of the file
    string name;

    // Name it should be written to in the resulting output. If empty then the same thing.
    string layout_name;

    // Lines to add to the init
    vector<string> init_lines;

    // Specifies which releases a file can be used on.
    int max_release_series = 0;
    int min_release_series = 0;
};

// Files to add in!
vector<helper_file> _g_helper_files {
    { "trigger.py", "", {"from .trigger import tdt_chain_fired", "from .trigger import tmt_match_object"} },
    { "type_support.py", "", {"from .type_support import cpp_float, cpp_double, cpp_vfloat, cpp_vint, cpp_string, cpp_int", "from . import type_support"}},
    { "calibration_support.py", "", {"from .calibration_support import CalibrationEventConfig, calib_tools"}, 25, 0},
    { "calibration_support.py", "", {"from .calibration_support import calib_tools"}, 0, 25},
    { "metadata_for_collections.py", "", {}, 25, 0},
    { "default_calibration_config.py", "", {}, 25, 0},
    { "calibration_event_config.py", "", {}, 25, 0},
    { "sys_error_tool.py", "templates/sys_error_tool.py", {}, 25, 0},
    { "pileup_tool.py", "templates/pileup_tool.py", {}, 25, 0},
    { "corrections_jet.py", "templates/corrections_jet.py", {}, 25, 0},
    { "corrections_electron.py", "templates/corrections_electron.py", {}, 25, 0},
    { "corrections_photon.py", "templates/corrections_photon.py", {}, 25, 0},
    { "corrections_muon.py", "templates/corrections_muon.py", {}, 25, 0},
    { "corrections_tau.py", "templates/corrections_tau.py", {}, 25, 0},
    { "corrections_overlap.py", "templates/corrections_overlap.py", {}, 25, 0},
    { "corrections_met.py", "templates/corrections_met.py", {}, 25, 0},
    { "add_calibration_to_job.py", "templates/add_calibration_to_job.py", {}},
    { "config_phys.yaml", "", {}, 0, 25},
    { "config_physlite.yaml", "", {}, 0, 25},
};

const std::string WHITESPACE = " \n\r\t\f\v";
 
std::string rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

// Write out all helper information.
void emit_helper_files(YAML::Emitter &out, const metadata_file_finder &finder, int release_series)
{
    bool header_written = false;
    for (auto &&hf : _g_helper_files)
    {
        if (hf.max_release_series != 0 && release_series >= hf.max_release_series) {
            continue;
        }
        if (hf.min_release_series != 0 && release_series < hf.min_release_series) {
            continue;
        }

        if (!header_written) {
            header_written = true;
            out << YAML::Key << "files" << YAML::Value << YAML::BeginSeq;
        }

        out << YAML::BeginMap;

        // Name to write this as
        auto out_name = hf.name;
        if (hf.layout_name.size() > 0) {
            out_name = hf.layout_name;
        }

        // Header info
        out << YAML::Key << "name" << YAML::Value << out_name;
        out << YAML::Key << "init_lines" << YAML::Value << YAML::BeginSeq;
        for (auto &&line : hf.init_lines) {
            out << line;
        }
        out << YAML::EndSeq;

        // Now file contents
        ifstream file_contents(finder(hf.name));
        string line;
        out << YAML::Key << "contents" << YAML::Value << YAML::BeginSeq;
        if (!file_contents.is_open()) {
            throw runtime_error("could not find helper file metadata/" + hf.name);
        }
        out.SetStringFormat(YAML::DoubleQuoted);
        while (getline(file_contents, line)) {
            out << rtrim(line);
        }
        out << YAML::EndSeq;

        out << YAML::EndMap;

    }

    if (header_written) {
        out << YAML::EndSeq;
    }
}