#include "bridge/Bridge.hpp"
#include "bridge/BridgeConfig.hpp"
#include "bridge/Logger.hpp"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    const std::string config_path = argc > 1 ? argv[1] : "configs/bridge_config.cfg";
    try {
        auto config = bridge::BridgeConfig::from_file(config_path);
        bridge::Bridge app(config);
        return app.run_until_shutdown();
    } catch (const std::exception& ex) {
        bridge::Logger::instance().log(bridge::LogLevel::Error, "main", ex.what());
        std::cerr << "ipc-socket-bridge failed: " << ex.what() << '\n';
        return 1;
    }
}
