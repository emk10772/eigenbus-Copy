#include "eigen_topology_tracker.h"

EigenTopologyTracker::EigenTopologyTracker(){

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
                for(auto port : update->module()->downstream()){
                    if(!node->has_child(port.addr_current)){
                        Node *child = add_node(update->address(), node);
                        node->add_child(child);
                    }
                }
            }
        } else if (update->type() == EigenUpdate::MODULE_ADDED && update->module() != nullptr){
            add_node(update->address());
        } else if (update->type() == EigenUpdate::MODULE_REMOVED){
            remove_node(update->address());
        }

        it++;
        updates_.pop_front();
    }
}

EigenTopologyTracker::Node *EigenTopologyTracker::add_node(eigen_addr_t address, Node *parent){
    //If there is no node, make a new one
    if(node_map_.count(address) == 0){
        Node *node = new Node(address, parent);
        node_map_[address] = node;

        if(parent == nullptr)
            root_nodes_[address] = node;

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
            root_nodes_[address] = node;

        return node;
    }
}

void EigenTopologyTracker::remove_node(eigen_addr_t address){
    if(node_map_.count(address) != 0){
        Node *node = node_map_[address];

        if(node->parent() == nullptr)
            root_nodes_.erase(address);

        node_map_.erase(address);
        for(auto child : node->children()){
            child->set_parent(nullptr);
            root_nodes_[address] = child;
        }

        delete node;
    }
}

eigen_addr_t EigenTopologyTracker::max_depth(){
    return 0;
}

std::vector<eigen_addr_t> EigenTopologyTracker::addr_by_depth(eigen_addr_t node_depth){
    return std::vector<eigen_addr_t>();
}

std::vector<eigen_addr_t> EigenTopologyTracker::addr_by_downstream(eigen_addr_t addr, uint8_t port){
    return std::vector<eigen_addr_t>();
}



EigenTopologyTracker::Node::Node(eigen_addr_t address){
    parent_ = nullptr;
    addr_ = address;
}

EigenTopologyTracker::Node::Node(eigen_addr_t address, EigenTopologyTracker::Node *parent){
    parent_ = parent;
    addr_ = address;
}

EigenTopologyTracker::Node::~Node(){
    if(parent_ != nullptr)
        parent_->remove_child(this);
}

eigen_addr_t EigenTopologyTracker::Node::address(){
    return addr_;
}

EigenTopologyTracker::Node *EigenTopologyTracker::Node::parent(){
    return parent_;
}

std::vector<EigenTopologyTracker::Node *> EigenTopologyTracker::Node::children(){
    return children_;
}

void EigenTopologyTracker::Node::set_parent(EigenTopologyTracker::Node *parent){
    parent_ = parent;
}

void EigenTopologyTracker::Node::add_child(EigenTopologyTracker::Node *child){
    //Make sure we don't have the child already
    for(auto child_ : children_){
        if(child_->address() == child->address())
            return;
    }

    children_.push_back(child);
}

void EigenTopologyTracker::Node::remove_child(EigenTopologyTracker::Node *child){
    remove_child(child->address());
}

void EigenTopologyTracker::Node::remove_child(eigen_addr_t child_addr){
    auto it = children_.begin();
    while(it != children_.end()){
        if(child_addr == (*it)->address())
            it = children_.erase(it);
        else
            it++;
    }
}

bool EigenTopologyTracker::Node::has_child(eigen_addr_t child){
    auto it = children_.begin();
    while(it != children_.end()){
        if(child == (*it)->address())
            return true;
        else
            it++;
    }
    return false;
}

bool EigenTopologyTracker::Node::is_root(){
    return parent_ == nullptr;
}
