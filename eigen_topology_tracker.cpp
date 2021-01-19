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
    auto it = updates_.front();
    while(it != updates_.back()){
        auto update = (*it);

        if(update.type() == EigenUpdate::MODULE_DOWNSTREAM){
            //TODO: this
        } else if(update.type() == EigenUpdate::MODULE_ADDED){

        } else if(update.type() == EigenUpdate::MODULE_REMOVED){

        }

        it++;
        updates_.pop_front();
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
