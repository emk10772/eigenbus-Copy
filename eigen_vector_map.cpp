#include "eigen_vector_map.h"


EigenVectorMap::EigenVectorMap(){

}

EigenVectorMap::~EigenVectorMap(){
    clear();
}

ModuleShared EigenVectorMap::get_shared(eigen_addr_t key){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if(map_.count(key) == 0) return nullptr;
    return map_[key];
}

ModuleShared EigenVectorMap::get_shared_by_ind(eigen_addr_t ind){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if(ind >= vector_.size()) return nullptr;
    return vector_[ind];
}

ModuleConst EigenVectorMap::get_const(eigen_addr_t key){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if(map_.count(key) == 0) return nullptr;
    return make_const(map_[key]);
}

ModuleConst EigenVectorMap::get_const_by_ind(eigen_addr_t ind){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if(ind >= vector_.size()) return nullptr;
    return make_const(vector_[ind]);
}

bool cmp(ModuleShared module, eigen_addr_t address){
    return module->address() < address;
}

void EigenVectorMap::remove(eigen_addr_t key){
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    remove_no_lock(key);
}

void EigenVectorMap::remove_no_lock(eigen_addr_t key){
    if(map_.count(key) == 0) return;

    map_.erase(key);

    auto where = std::lower_bound(vector_.begin(), vector_.end(), key, &cmp);

    if(*where == nullptr || where->get()->address() != key)
        throw std::out_of_range("Vector and Map mismatch");
    else
        vector_.erase(where);
}

bool EigenVectorMap::remove_ind_no_lock(eigen_addr_t ind){
    if(ind >= vector_.size()) return false;
    eigen_addr_t key = vector_[ind]->address();

    if(map_.count(key) == 0) {
        throw std::out_of_range("Vector and Map mismatch");
    } else {
        map_.erase(key);
        vector_.erase(vector_.begin() + ind);
        return true;
    }
}

void EigenVectorMap::insert(ModuleShared module){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if(module == NULL || map_.count(module->address()) > 0) return;

    map_[module->address()] = module;

    auto where = std::lower_bound(vector_.begin(), vector_.end(), module->address(), &cmp);
    vector_.insert(where, module);
}

void EigenVectorMap::clear(){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    map_.clear();
    vector_.clear();
}

std::set<eigen_addr_t> EigenVectorMap::keys(){
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::set<eigen_addr_t> retval;

    for(auto mod : vector_){
        retval.insert(mod->address());
    }

    return retval;
}

std::vector<EigenUpdate *> EigenVectorMap::clear_old(uint64_t t_now, uint64_t t_stale){
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    eigen_addr_t ind = 0;
    bool valid = true;
    std::vector<EigenUpdate *> retval;

    while(valid && ind < vector_.size()){
        if((t_now - vector_[ind]->t_last_update) > t_stale){
            eigen_addr_t addr = vector_[ind]->address();
            valid = remove_ind_no_lock(ind);
            if(valid){
                retval.push_back(new EigenUpdate(addr, EigenUpdate::MODULE_REMOVED, nullptr));
            }
        } else {
            ind++;
        }
    }

    return retval;
}

eigen_addr_t EigenVectorMap::size(){
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    return vector_.size();
}
