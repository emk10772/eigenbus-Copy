#ifndef EIGEN_TOPOLOGY_TRACKER_H
#define EIGEN_TOPOLOGY_TRACKER_H

#include "eigen_utils.h"
#include "eigen_responses/eigen_response.h"
#include "eigen_commands/eigen_command.h"
#include "eigen_update.h"
#include <vector>
#include <deque>
#include <map>

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
        bool has_child(eigen_addr_t child);
        bool is_root();

    private:
        eigen_addr_t addr_;
        std::vector<Node *> children_;
        Node *parent_;
    };

    Node *add_node(eigen_addr_t address, Node *parent = nullptr);
    void remove_node(eigen_addr_t address);

    std::deque<EigenUpdate *> updates_;
    std::map<eigen_addr_t, Node *> root_nodes_;
    std::map<eigen_addr_t, Node *> node_map_;
};

#endif
