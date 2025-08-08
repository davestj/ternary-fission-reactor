#ifndef DAEMON_TERNARY_FISSION_SERVER_H
#define DAEMON_TERNARY_FISSION_SERVER_H

#include <memory>

#include "config.ternary.fission.server.h"

namespace TernaryFission {

class DaemonTernaryFissionServer {
public:
    DaemonTernaryFissionServer();
    explicit DaemonTernaryFissionServer(std::shared_ptr<Configuration> config);

    std::shared_ptr<Configuration> getConfiguration() const;

private:
    std::shared_ptr<Configuration> configuration_;
};

} // namespace TernaryFission

#endif // DAEMON_TERNARY_FISSION_SERVER_H
