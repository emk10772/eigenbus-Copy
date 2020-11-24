#ifndef EIGEN_VECTOR_MAP_H
#define EIGEN_VECTOR_MAP_H

#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>
#include <set>
#include "eigen_module.h"
#include "eigen_responses/eigen_response.h"

class EigenVectorMap{
public:
    EigenVectorMap();
    ~EigenVectorMap();

    ModuleShared get_shared(eigen_addr_t key);
    ModuleShared get_shared_by_ind(eigen_addr_t ind);

    ModuleConst get_const(eigen_addr_t key);
    ModuleConst get_const_by_ind(eigen_addr_t ind);

    void insert(ModuleShared module);
    void remove(eigen_addr_t key);
    void clear();
    std::vector<EigenUpdate *> clear_old(uint64_t t_now, uint64_t t_stale);
    std::set<eigen_addr_t> keys();
    eigen_addr_t size();

private:
    void remove_no_lock(eigen_addr_t key);
    bool remove_ind_no_lock(eigen_addr_t ind);

    std::recursive_mutex mutex_;
    std::unordered_map<eigen_addr_t, ModuleShared> map_;
    std::vector<ModuleShared> vector_;
};

#endif
