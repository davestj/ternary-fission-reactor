#include "daemon.ternary.fission.server.h"

namespace TernaryFission {

DaemonTernaryFissionServer::DaemonTernaryFissionServer()
    : configuration_(std::make_shared<Configuration>()) {}

DaemonTernaryFissionServer::DaemonTernaryFissionServer(std::shared_ptr<Configuration> config)
    : configuration_(std::move(config)) {
    if (!configuration_) {
        configuration_ = std::make_shared<Configuration>();
    }
}

std::shared_ptr<Configuration> DaemonTernaryFissionServer::getConfiguration() const {
    return configuration_;
}

} // namespace TernaryFission
