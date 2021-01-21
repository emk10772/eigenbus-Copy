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

    eigen_addr_t max_depth() const;

    class Node{
    public:
        Node(eigen_addr_t address, EigenTopologyTracker *tracker);
        Node(eigen_addr_t address, EigenTopologyTracker *tracker, Node *parent);
        ~Node();

        eigen_addr_t address() const;
        eigen_addr_t port_id() const;
        std::string port_name() const;
        std::string text() const;
        const Node *parent() const;
        int childNumber() const;
        Node *parent();
        const std::map<eigen_addr_t, Node *> children() const;
        const Node *child(eigen_addr_t index) const;

        void update_depth(Node *caller = nullptr);
        void set_parent(Node *parent);
        void set_port(eigen_addr_t id, std::string name);
        void add_child(Node *child);
        void remove_child(Node *child);
        void remove_child(eigen_addr_t child_addr);
        bool has_child(eigen_addr_t child) const;
        bool is_root() const;
        eigen_addr_t depth() const;

    private:
        eigen_addr_t addr_;
        std::map<eigen_addr_t, Node *> children_;
        Node *parent_;
        eigen_addr_t port_id_;
        std::string port_name_;
        eigen_addr_t depth_;
        EigenTopologyTracker *tracker_;
    };

    const Node *get_node(eigen_addr_t address) const;
    const Node *root_node() const;
    const std::vector<Node *> get_depth_list(eigen_addr_t depth) const;

private:
    Node *add_node(eigen_addr_t address, Node *parent = nullptr);
    void remove_node(eigen_addr_t address);
    void remove_depth(Node *node, eigen_addr_t depth);
    void add_depth(Node *node, eigen_addr_t depth);

    std::deque<EigenUpdate *> updates_;
    Node *root_node_;
    std::map<eigen_addr_t, Node *> node_map_;
    std::vector<std::vector<Node *>> depth_list_;
};

#endif
