/* eigen_vector_map.h
 *
 * This class is a combination of a vector and an unordered map.
 *
 * This unholy data structure exists because we often need both a sorted list
 * and fast access by module address at the same time. The combination of a
 * vector and an unordered map allow for constant time access by both index
 * and by module address.
 *
 * Adding to / removing from this data structure is relatively slow, but this
 * is not an issue because we only add and remove from this structure on
 * library initialization / deinitialization (AKA not very often).
 *
 * This class is designed to be thread safe.
 *
 * Created by Nick Paiva - 2020
 */

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

    //Get the mutable reference to a module
    ModuleShared get_shared(eigen_addr_t key);          //By module address
    ModuleShared get_shared_by_ind(eigen_addr_t ind);   //By position in sorted list

    //Get the non-mutable reference to a module
    ModuleConst get_const(eigen_addr_t key);            //By module address
    ModuleConst get_const_by_ind(eigen_addr_t ind);     //By position in sorted list

    //Add/remove modules
    void insert(ModuleShared module);
    void remove(eigen_addr_t key);                      //By module address
    void clear();

    //This function is used to get rid of stale modules
    std::vector<EigenUpdate *> clear_old(uint64_t t_now, uint64_t t_stale);

    //Inclusion / size info functions
    std::set<eigen_addr_t> keys();
    eigen_addr_t size();

private:
    //Lock free removes for internal use
    void remove_no_lock(eigen_addr_t key);
    bool remove_ind_no_lock(eigen_addr_t ind);

    std::recursive_mutex mutex_;                            //Mutex. For thread safety
    std::unordered_map<eigen_addr_t, ModuleShared> map_;    //Map from address to module
    std::vector<ModuleShared> vector_;                      //Sorted list (by address)
};

#endif
