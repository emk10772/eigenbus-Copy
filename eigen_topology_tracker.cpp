#include "eigen_topology_tracker.h"
#include <iterator>

EigenTopologyTracker::EigenTopologyTracker(){
    root_node_ = new Node(0xFF, this);
}

EigenTopologyTracker::~EigenTopologyTracker(){

}

void EigenTopologyTracker::add_update(EigenUpdate *update){
    updates_.push_back(update);
}

void EigenTopologyTracker::add_update(std::vector<EigenUpdate *> updates){
    for(auto update : updates){
        updates_.push_back(update);
    }
}

void EigenTopologyTracker::process_updates(){
    auto it = updates_.begin();
    while(it != updates_.end()){
        EigenUpdate *update = *it;

        if(update == nullptr) return;

        if(update->type() == EigenUpdate::MODULE_DOWNSTREAM && update->module() != nullptr){
            if(node_map_.count(update->address()) != 0){
                Node *node = node_map_[update->address()];
                //Check if the node already has an entry for the specific port
                eigen_addr_t ind = 0;
                for(auto port : update->module()->downstream()){
                    if(port.addr_current != 0xFF && !node->has_child(port.addr_current)){
                        Node *child = add_node(port.addr_current, node);
                        node->add_child(child);
                        child->set_port(ind, port.name);
                    }
                    ind++;
                }
            }
        } else if (update->type() == EigenUpdate::MODULE_ADDED && update->module() != nullptr){
            add_node(update->address());
        } else if (update->type() == EigenUpdate::MODULE_REMOVED){
            remove_node(update->address());
        }

        updates_.pop_front();
        it = updates_.begin();
    }
}

EigenTopologyTracker::Node *EigenTopologyTracker::add_node(eigen_addr_t address, Node *parent){
    //If there is no node, make a new one
    if(node_map_.count(address) == 0){
        Node *node = new Node(address, this, parent);
        node_map_[address] = node;

        if(parent == nullptr)
            root_node_->add_child(node);

        return node;
    //If there is a node, detach it from its current parent
    } else {
        Node *node = node_map_[address];

        if(node->parent() != parent){
            if(node->parent() != nullptr)
                node->parent()->remove_child(node);
            node->set_parent(parent);
        }

        if(parent == nullptr)
            root_node_->add_child(node);

        return node;
    }
}

void EigenTopologyTracker::remove_node(eigen_addr_t address){
    if(node_map_.count(address) != 0){
        Node *node = node_map_[address];

        node->parent()->remove_child(node);

        node_map_.erase(address);
        for(auto child : node->children()){
            root_node_->add_child(child.second);
        }

        delete node;
    }
}

const EigenTopologyTracker::Node *EigenTopologyTracker::get_node(eigen_addr_t address) const{
    if(node_map_.count(address) > 0){
        Node *node = node_map_.at(address);
        return node;
    } else if(address == root_node_->address()){
        return root_node_;
    }
    return nullptr;
}

const EigenTopologyTracker::Node *EigenTopologyTracker::root_node() const{
    return root_node_;
}

const std::vector<EigenTopologyTracker::Node *> EigenTopologyTracker::get_depth_list(eigen_addr_t depth) const{
    if(depth < depth_list_.size())
        return depth_list_[depth];
    else
        return std::vector<EigenTopologyTracker::Node *>();
}

eigen_addr_t EigenTopologyTracker::max_depth() const{
    return depth_list_.size();
}

void EigenTopologyTracker::remove_depth(Node *node, eigen_addr_t depth){
    if(depth >= depth_list_.size()) return;

    auto it = depth_list_[depth].begin();
    while(it != depth_list_[depth].end()){
        if((*it)->address() == node->address())
            it = depth_list_[depth].erase(it);
        else
            it++;
    }

    if(depth == (depth_list_.size() - 1) && depth_list_[depth].empty())
        depth_list_.pop_back();
}

void EigenTopologyTracker::add_depth(Node *node, eigen_addr_t depth){
    while(depth >= depth_list_.size()){
        depth_list_.push_back(std::vector<Node *>());
    }

    depth_list_[depth].push_back(node);
}


EigenTopologyTracker::Node::Node(eigen_addr_t address, EigenTopologyTracker *tracker){
    parent_ = nullptr;
    addr_ = address;
    tracker_ = tracker;
    depth_ = 0;
}

EigenTopologyTracker::Node::Node(eigen_addr_t address, EigenTopologyTracker *tracker, EigenTopologyTracker::Node *parent){
    parent_ = parent;
    addr_ = address;
    tracker_ = tracker;
    depth_ = 0;
}

EigenTopologyTracker::Node::~Node(){
    if(parent_ != nullptr)
        parent_->remove_child(this);
}

eigen_addr_t EigenTopologyTracker::Node::address() const{
    return addr_;
}

eigen_addr_t EigenTopologyTracker::Node::port_id() const{
    return port_id_;
}

std::string EigenTopologyTracker::Node::port_name() const{
    return port_name_;
}

std::string EigenTopologyTracker::Node::text() const{
    if(is_root())
        return strprintf("Module %02X", addr_);
    else
        return strprintf("%01X: Module %02X", port_id_, addr_);
}

void EigenTopologyTracker::Node::set_port(eigen_addr_t id, std::string name){
    port_id_ = id;
    port_name_ = name;
}

const EigenTopologyTracker::Node *EigenTopologyTracker::Node::parent() const{
    return parent_;
}

EigenTopologyTracker::Node *EigenTopologyTracker::Node::parent() {
    return parent_;
}

int EigenTopologyTracker::Node::childNumber() const{
    if(parent_)
        return std::distance(parent_->children_.begin(), parent_->children_.find(addr_));
    return 0;
}

const std::map<eigen_addr_t, EigenTopologyTracker::Node *> EigenTopologyTracker::Node::children() const{
    return children_;
}

const EigenTopologyTracker::Node *EigenTopologyTracker::Node::child(eigen_addr_t index) const{
    if(index >= children_.size()) return nullptr;

    auto it = children_.begin();
    for(eigen_addr_t ind = 0; ind < index; ind++)
        it++;

    return it->second;
}

void EigenTopologyTracker::Node::update_depth(EigenTopologyTracker::Node *caller){
    Node *caller_ = caller;
    if(caller == nullptr) //First call
        caller_ = this;
    else if(caller == this) //Guard against loops in the graph
        return;

    tracker_->remove_depth(this, depth_);
    //If we don't have a parent, our depth is 0
    depth_ = (parent_) ? parent_->depth() + 1 : 0;
    tracker_->add_depth(this, depth_);

    //Update our children
    for(auto child : children_){
        child.second->update_depth(caller);
    }
}

void EigenTopologyTracker::Node::set_parent(EigenTopologyTracker::Node *parent){
    parent_ = parent;

    //Reset upstream info
    port_id_ = 0xFF;
    port_name_ = "";

    //Recalculate the depth and the depth of the node's children
    update_depth();
}

void EigenTopologyTracker::Node::add_child(EigenTopologyTracker::Node *child){
    //Make sure we don't have the child already
    if(children_.count(child->address()) == 0){
        children_[child->address()] = child;
        child->set_parent(this);
    }
}

void EigenTopologyTracker::Node::remove_child(EigenTopologyTracker::Node *child){
    remove_child(child->address());
}

void EigenTopologyTracker::Node::remove_child(eigen_addr_t child_addr){
    children_.erase(child_addr);
}

bool EigenTopologyTracker::Node::has_child(eigen_addr_t child) const{
    return children_.count(child) > 0;
}

bool EigenTopologyTracker::Node::is_root() const{
    if(parent_ != nullptr)
        return parent_->address() == 0xFF;

    return parent_ == nullptr;
}

eigen_addr_t EigenTopologyTracker::Node::depth() const{
    return depth_;
}

