#include "bridge/Bridge.hpp"

#include <iostream>

int main(int argc, char** argv) {
    const std::string config_path = argc > 1 ? argv[1] : "configs/bridge_config.cfg";
    try {
        auto config = bridge::BridgeConfig::from_file(config_path);
        bridge::Bridge bridge(config);
        return bridge.run_until_shutdown();
    } catch (const std::exception& ex) {
        std::cerr << "basic_bridge failed: " << ex.what() << '\n';
        return 1;
    }
}
