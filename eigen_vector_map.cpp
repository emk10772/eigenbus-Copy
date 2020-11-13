#include "eigen_vector_map.h"


EigenVectorMap::EigenVectorMap(){

}

EigenVectorMap::~EigenVectorMap(){
    clear();
}

ModuleShared EigenVectorMap::get_shared(eigen_addr_t key){
    std::lock_guard<std::mutex> lock(mutex_);

    if(map_.count(key) == 0) return nullptr;
    return map_[key];
}

ModuleShared EigenVectorMap::get_shared_by_ind(eigen_addr_t ind){
    std::lock_guard<std::mutex> lock(mutex_);

    if(ind > vector_.size()) return nullptr;
    return vector_[ind];
}

ModuleConst EigenVectorMap::get_const(eigen_addr_t key){
    std::lock_guard<std::mutex> lock(mutex_);

    if(map_.count(key) == 0) return nullptr;
    return std::const_pointer_cast<const EigenModule>(map_[key]);
}

ModuleConst EigenVectorMap::get_const_by_ind(eigen_addr_t ind){
    std::lock_guard<std::mutex> lock(mutex_);

    if(ind > vector_.size()) return nullptr;
    return std::const_pointer_cast<const EigenModule>(vector_[ind]);
}

bool cmp(ModuleShared module, eigen_addr_t address){
    return module->get_address() < address;
}

void EigenVectorMap::remove(eigen_addr_t key){
    std::lock_guard<std::mutex> lock(mutex_);
    remove_no_lock(key);
}

void EigenVectorMap::remove_no_lock(eigen_addr_t key){
    if(map_.count(key) == 0) return;

    map_.erase(key);

    auto where = std::lower_bound(vector_.begin(), vector_.end(), key, &cmp);

    if(*where == nullptr || where->get()->get_address() != key)
        throw std::out_of_range("Vector and Map mismatch");
    else
        vector_.erase(where);
}

void EigenVectorMap::insert(ModuleShared module){
    std::lock_guard<std::mutex> lock(mutex_);

    if(module == NULL || map_.count(module->get_address()) > 0) return;

    map_[module->get_address()] = module;

    auto where = std::lower_bound(vector_.begin(), vector_.end(), module->get_address(), &cmp);
    vector_.insert(where, module);
}

void EigenVectorMap::clear(){
    map_.clear();
    vector_.clear();
}

std::set<eigen_addr_t> EigenVectorMap::keys(){
    std::lock_guard<std::mutex> lock(mutex_);
    std::set<eigen_addr_t> retval;

    for(auto mod : vector_){
        retval.insert(mod->get_address());
    }

    return retval;
}

void EigenVectorMap::clear_old(uint64_t t_now, uint64_t t_stale){
    std::lock_guard<std::mutex> lock(mutex_);
    eigen_addr_t ind = 0;

    while(ind < vector_.size()){
        if((t_now - vector_[ind]->t_last_update) > t_stale){
            remove_no_lock(ind);
        } else {
            ind++;
        }
    }
}

eigen_addr_t EigenVectorMap::size(){
    std::lock_guard<std::mutex> lock(mutex_);

    return vector_.size();
}
