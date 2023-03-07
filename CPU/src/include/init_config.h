#include <iostream>
#include <fstream>
#include "json.hh"
using json = nlohmann::json;

json global_cfg;

// Load global config from file.
extern "C" void init_config(const char* config_path) {
    std::ifstream file(config_path);
    if (file.fail()) {
        fprintf(stderr, "failed to open %s\n", config_path);
        exit(-1);
    }
    global_cfg = json::parse(file, nullptr, true, true);
}
