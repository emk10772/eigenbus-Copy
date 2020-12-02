#ifndef EIGEN_TOPOLOGY_TRACKER_H
#define EIGEN_TOPOLOGY_TRACKER_H

#include "eigen_utils.h"
#include "eigen_responses/eigen_response.h"
#include "eigen_commands/eigen_command.h"
#include "eigen_update.h"
#include <vector>

class EigenTopologyTracker{
public:
    EigenTopologyTracker();
    ~EigenTopologyTracker();

    void process_packet(EigenResponse *packet);
    void process_command(EigenCommand *command);
    void process_update(EigenUpdate *update);

    eigen_addr_t max_depth();
    std::vector<eigen_addr_t> addr_by_depth(eigen_addr_t node_depth);
    std::vector<eigen_addr_t> addr_by_downstream(eigen_addr_t addr, uint8_t port);
private:
};

#endif
