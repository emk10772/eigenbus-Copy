#ifndef EIGEN_TOPOLOGY_TRACKER_H
#define EIGEN_TOPOLOGY_TRACKER_H

#include "eigen_utils.h"
#include "eigen_responses/eigen_response.h"
#include "eigen_commands/eigen_command.h"
#include "eigen_update.h"
#include <vector>
#include <deque>

class EigenTopologyTracker{
public:
    EigenTopologyTracker();
    ~EigenTopologyTracker();

    void add_update(EigenUpdate *update);
    void add_update(std::vector<EigenUpdate *>);
    void process_updates();

    eigen_addr_t max_depth();
    std::vector<eigen_addr_t> addr_by_depth(eigen_addr_t node_depth);
    std::vector<eigen_addr_t> addr_by_downstream(eigen_addr_t addr, uint8_t port);
private:
    class Node{
    public:
        Node(eigen_addr_t address);
        Node(eigen_addr_t address, Node *parent);
        ~Node();

        eigen_addr_t address();
        Node *parent();
        std::vector<Node *> children();

        void set_parent(Node *parent);
        void add_child(Node *child);
        void remove_child(Node *child);
        void remove_child(eigen_addr_t child_addr);

    private:
        eigen_addr_t addr_;
        std::vector<Node *> children_;
        Node *parent_;
    };

    std::deque<EigenUpdate *> updates_;
    std::vector<Node *>root_nodes_;
};

#endif
