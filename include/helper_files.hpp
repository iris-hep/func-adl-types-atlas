#ifndef __helper_files__
#define __helper_files__

#include "yaml-cpp/yaml.h"
#include "metadata_file_finder.hpp"

// Emit the files
void emit_helper_files (YAML::Emitter &out, const metadata_file_finder &finder, int release_series);

#endif
