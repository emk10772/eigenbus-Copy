#include "eigen_topology_tracker.h"
#include <iterator>

EigenTopologyTracker::EigenTopologyTracker(){
    root_node_ = new Node(0xFF, this); //The root node has an address of 0xFF, or invalid
    add_depth(root_node_, 0);
}

EigenTopologyTracker::~EigenTopologyTracker(){
    delete root_node_;

    //Delete all the nodes
    for(auto node : node_map_){
        //Clear all parent references to stop use-after-free in Node destructor
        node.second->clear_children();
        node.second->set_parent(nullptr);
        delete node.second;
    }

    //Clear the node references
    node_map_.clear();
    depth_list_.clear();
}

void EigenTopologyTracker::add_update(EigenUpdate *update){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    updates_.push_back(update);
}

void EigenTopologyTracker::add_update(std::vector<EigenUpdate *> updates){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    for(auto update : updates){
        updates_.push_back(update);
    }
}

/* void EigenTopologyTracker::process_updates()
 *
 * This function is responsible for maintaining the topology graph based on
 * module updates. It focuses on three types of updates:
 *  1. MODULE_DOWNSTREAM: Downstream ports have changes
 *  2. MODULE_ADDED: A new module has been added
 *  3. MODULE_REMOVED: A module has been removed
 *
 * Thread safe, as long as the updates are processed before being sent to the
 * application. In normal implementations this should not be an issue
 */
void EigenTopologyTracker::process_updates(){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    auto it = updates_.begin();
    while(it != updates_.end()){
        EigenUpdate *update = *it;

        if(update == nullptr) return; //If the update is invalid, we are done here

        //Module ports have been updated
        if(update->type() == EigenUpdate::MODULE_DOWNSTREAM && update->module() != nullptr){
            //Check that there is a node with this address
            if(node_map_.count(update->address()) != 0){
                Node *node = node_map_[update->address()];

                //Check if the node already has an entry for the specific port
                eigen_addr_t ind = 0;
                for(auto &port : update->module()->downstream()){
                    //If there is no node at the port representing this module, add one
                    if(port.addr_current != 0xFF && !node->has_child(port.addr_current)){
                        Node *child = add_node(port.addr_current, node);
                        node->add_child(child);
                        child->set_port(ind, port.name); //Update the child with its index and port name
                    }
                    ind++;
                }
            }
        //If there is a module added, add a new node if there is not yet one in the map
        } else if (update->type() == EigenUpdate::MODULE_ADDED && update->module() != nullptr){
            if(node_map_.count(update->address()) == 0)
                add_node(update->address());
        //Delete the node that has been removed
        } else if (update->type() == EigenUpdate::MODULE_REMOVED){
            remove_node(update->address());
        }

        //Get the next update
        updates_.pop_front();
        it = updates_.begin();
    }
}

/* EigenTopologyTracker::Node *EigenTopologyTracker::add_node(eigen_addr_t address, Node *parent)
 *
 * Adds or fetches a node corresponding to the specified address
 *
 * Thread safe
 */
EigenTopologyTracker::Node *EigenTopologyTracker::add_node(eigen_addr_t address, Node *parent){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    //If there is no node, make a new one
    if(node_map_.count(address) == 0){
        Node *node = new Node(address, this, parent);
        node_map_[address] = node;

        if(parent == nullptr)
            root_node_->add_child(node);

        return node;
    //If there is a node, reset its parenthood to the new parent
    } else {
        Node *node = node_map_[address];

        //If the node already has a parent, detatch it
        if(node->parent() != parent){
            if(node->parent() != nullptr)
                node->parent()->remove_child(node);
            node->set_parent(parent);
        }

        //If this is a top level node, add it to the root node
        if(parent == nullptr)
            root_node_->add_child(node);

        return node;
    }
}

/* void EigenTopologyTracker::remove_node(eigen_addr_t address)
 *
 * Deletes the node corresponding to the specified address
 *
 * Thread safe
 */
void EigenTopologyTracker::remove_node(eigen_addr_t address){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    //Make sure that the node exists first
    if(node_map_.count(address) != 0){
        Node *node = node_map_[address];

        //Remove it from its parent and the depth list
        node->parent()->remove_child(node);
        remove_depth(node, node->depth());

        //Remove it from the node map, and add any orphaned children to the root node
        node_map_.erase(address);
        for(auto child : node->children()){
            root_node_->add_child(child.second);
        }

        delete node;
    }
}

/* const EigenTopologyTracker::Node *EigenTopologyTracker::get_node(eigen_addr_t address) const
 *
 * Get the node with the corresponding address.
 * If the address is 0xFF, then it is the root node.
 * Returns null if the address does not correspond to an existing node.
 *
 * Thread safe
 */
const EigenTopologyTracker::Node *EigenTopologyTracker::get_node(eigen_addr_t address) const{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if(node_map_.count(address) > 0){
        Node *node = node_map_.at(address);
        return node;
    } else if(address == root_node_->address()){
        return root_node_;
    }
    return nullptr;
}

/* const EigenTopologyTracker::Node *EigenTopologyTracker::root_node() const
 *
 * Returns a pointer to the root node of the graph
 *
 * Thread safe
 */
const EigenTopologyTracker::Node *EigenTopologyTracker::root_node() const{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    return root_node_;
}

/* const std::vector<EigenTopologyTracker::Node *>
 *  EigenTopologyTracker::get_depth_list(eigen_addr_t depth) const
 *
 *  Returns the list for the specified depth. If the depth is not valid it will
 *  return an empty list.
 *
 *  Thread safe
 */
const std::vector<EigenTopologyTracker::Node *>
    EigenTopologyTracker::get_depth_list(eigen_addr_t depth) const{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if(depth < depth_list_.size())
        return depth_list_[depth];
    else
        return std::vector<EigenTopologyTracker::Node *>();
}

eigen_addr_t EigenTopologyTracker::max_depth() const{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    return (eigen_addr_t) depth_list_.size();
}

/* void EigenTopologyTracker::remove_depth(Node *node, eigen_addr_t depth)
 *
 * Remove a node from the depth list
 *
 * The list for each individual level is not sorted, so this function performs
 * a simple linear search through the list until it finds the matching address.
 *
 * Thread safe
 */
void EigenTopologyTracker::remove_depth(Node *node, eigen_addr_t depth){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if(depth >= depth_list_.size()) return;

    //Find the node and erase it from the list
    auto it = depth_list_[depth].begin();
    while(it != depth_list_[depth].end()){
        if((*it)->address() == node->address())
            it = depth_list_[depth].erase(it);
        else
            it++;
    }

    //If this was the last node in the last list then delete the list
    if(depth == (depth_list_.size() - 1) && depth_list_[depth].empty())
        depth_list_.pop_back();
}

/* void EigenTopologyTracker::add_depth(Node *node, eigen_addr_t depth)
 *
 * Add a node to the depth list. Generates additional lists up to the specified
 * depth if they do not exist yet.
 *
 * Thread safe
 */
void EigenTopologyTracker::add_depth(Node *node, eigen_addr_t depth){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    //If there aren't any nodes at the depths before this yet, expand the list
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
    //If this node has a parent, make sure to remove it from their child list
    if(parent_ != nullptr)
        parent_->remove_child(this);
}

eigen_addr_t EigenTopologyTracker::Node::address() const{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    return addr_;
}

eigen_addr_t EigenTopologyTracker::Node::port_id() const{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    return port_id_;
}

std::string EigenTopologyTracker::Node::port_name() const{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    return port_name_;
}

/* std::string EigenTopologyTracker::Node::text() const
 *
 * This function generates a string representation of this node. This function
 * is useful for GUI representations of topology. There are two modes:
 *  1. If it is a root node it simply prints out the address of the corresponding module
 *  2. If it is a child it prints out the corresponding port ID before the module ID
 *
 * Thread safe
 */
std::string EigenTopologyTracker::Node::text() const{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if(is_root())
        return strprintf("Module %02X", addr_);
    else
        return strprintf("%01X: Module %02X", port_id_, addr_);
}

void EigenTopologyTracker::Node::set_port(eigen_addr_t id, std::string name){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    port_id_ = id;
    port_name_ = name;
}

const EigenTopologyTracker::Node *EigenTopologyTracker::Node::parent() const{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    return parent_;
}

EigenTopologyTracker::Node *EigenTopologyTracker::Node::parent() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    return parent_;
}

int EigenTopologyTracker::Node::childNumber() const{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if(parent_)
        return (int) std::distance(parent_->children_.begin(), parent_->children_.find(addr_));
    return 0;
}

const std::map<eigen_addr_t, EigenTopologyTracker::Node *> EigenTopologyTracker::Node::children() const{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    return children_;
}

const EigenTopologyTracker::Node *EigenTopologyTracker::Node::child(eigen_addr_t index) const{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if(index >= children_.size()) return nullptr;

    auto it = children_.begin();
    for(eigen_addr_t ind = 0; ind < index; ind++)
        it++;

    return it->second;
}

/* void EigenTopologyTracker::Node::update_depth(EigenTopologyTracker::Node *caller)
 *
 * This function recursively updates the depth measurement to this node and all
 * of its children. The <caller> parameter is used as a guard against infinite
 * loops. When the function is first called, <caller> is initially null. For
 * subsequent calls it is set to the first node that it was called by. If a node
 * recognizes the <caller> parameter as itself then it has found a loop.
 *
 * This function will recursively trigger the update_depth function in its
 * children. If the node has no parents it will have a depth of 0.
 *
 * Thread safe
 */
void EigenTopologyTracker::Node::update_depth(EigenTopologyTracker::Node *caller){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

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
        child.second->update_depth(caller_);
    }
}

/* void EigenTopologyTracker::Node::set_parent(EigenTopologyTracker::Node *parent)
 *
 * Set the parent of this node and recalculate the depth measurement.
 *
 * Thread safe
 */
void EigenTopologyTracker::Node::set_parent(EigenTopologyTracker::Node *parent){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    parent_ = parent;

    //Reset upstream info
    port_id_ = 0xFF;
    port_name_ = "";

    //Recalculate the depth and the depth of the node's children
    update_depth();
}

/* void EigenTopologyTracker::Node::add_child(EigenTopologyTracker::Node *child)
 *
 * Add a child to this node.
 *
 * Thread safe
 */
void EigenTopologyTracker::Node::add_child(EigenTopologyTracker::Node *child){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    //Make sure we don't have the child already
    if(children_.count(child->address()) == 0){
        children_[child->address()] = child;
        child->set_parent(this);
    }
}

void EigenTopologyTracker::Node::remove_child(EigenTopologyTracker::Node *child){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    remove_child(child->address());
}

void EigenTopologyTracker::Node::remove_child(eigen_addr_t child_addr){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    children_.erase(child_addr);
}

void EigenTopologyTracker::Node::clear_children() {
    children_.clear();
}

bool EigenTopologyTracker::Node::has_child(eigen_addr_t child) const{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    return children_.count(child) > 0;
}

bool EigenTopologyTracker::Node::is_root() const{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if(parent_ != nullptr)
        return parent_->address() == 0xFF;

    return parent_ == nullptr;
}

eigen_addr_t EigenTopologyTracker::Node::depth() const{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    return depth_;
}

