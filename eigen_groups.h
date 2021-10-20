/* eigen_groups.h
 *
 * This file is the workhorse of module grouping. It contains tools for grouping
 * modules together and comparing their values. This interface vastly simplifies
 * group selection on the GUI side.
 *
 * Check https://github.com/biorobotics/titan-scope for examples on usage of this
 * file.
 */

#ifndef EIGENGROUP_H
#define EIGENGROUP_H

#include "eigen_commands/eigen_command.h"
#include "eigen_module.h"
#include "eigen_variable.h"
#include <type_traits>

/* EigenVariableGroup
    A strict observer of EigenVariables. Used to look for common values between variables.
    Must ensure that this class does not outlive its variables.
    Underlying data structure is not designed to be thread safe. Should not be modified while in use
*/
class EigenVariableGroup {
public:
    EigenVariableGroup(eigen_addr_t min_count = 0);

    //Utility functions for adding variables to this group
    void add_variable(const EigenVariable *variable);
    void add_variables(const std::vector<const EigenVariable *> variables);
    void add_variables(EigenVariableGroup *group);

    std::string print_variable(const std::string key) const;
    std::string print_variable_list(const std::string key) const;
    const std::vector<std::string> common_keys() const;
    const std::vector<const EigenVariable *> variables(const std::string key) const;
    int common_key_index(std::string) const;
    bool values_match(const std::string key) const;

    //T *value: A template for getting a value from the specified key
    template<typename T> const T* value(const std::string key) const{
        //Make sure that this is the correct type
        static_assert(std::is_base_of<EigenVariable, T>::value,
                            "Base type must be of EigenVariable");
        try {
            //Return a null reference if either is true:
            // 1. The values do not match in type or value
            // 2. The variable does not match the provided type
            if(!values_match(key)) return nullptr;
            const EigenVariable *var = variable_map_.find(key)->second;
            return dynamic_cast<const T*>(var);;
        } catch (std::exception e) {
            return nullptr;
        }
        return nullptr;
    }
private:
    std::unordered_multimap<std::string, const EigenVariable *> variable_map_;
    std::vector<std::string> common_keys_;
    eigen_addr_t min_count_;
};

class EigenModuleGroup {
public:
    EigenModuleGroup(std::vector<ModuleConst> modules);
    ~EigenModuleGroup();

    const EigenVariableGroup variable_group();
    const EigenVariableGroup parameter_group();
    const EigenVariableGroup mailbox_group();

    //Helper functions for checking the modules in this group
    bool contains_module(eigen_addr_t address);
    bool contains_module(ModuleConst module);
    size_t count();
    std::vector<ModuleConst> modules();

    //Group command utilities
    void group_command(EigenCommand *command);
    std::string group_poll(EigenCommand *command, uint64_t period_ms = UPDATE_PERIOD, bool enabled = true);
    void group_poll_set_enabled(std::string key, bool enabled = true);
    void group_poll_remove(std::string key);

    //Wrappers for variable_group value functions
    template<typename T> const T* key_to_value(const std::string key) const{
        return variable_group_.value<T>(key);
    }
    std::string key_to_string(const std::string key) const;
    std::string key_to_list_string(const std::string key) const;

    std::string print_list(const eigen_addr_t max_len) const;

private:
    std::vector<ModuleConst> modules_;      //List of modules that are in this group
    std::set<eigen_addr_t> module_addrs_;   //A set of the module addresses for fast inclusion checks
    std::unordered_multimap<std::string, std::string> poll_keys_; //List of keys used for group polling
    EigenVariableGroup variable_group_;     //All of the standard Eigenbus variables for each module
    EigenVariableGroup parameter_group_;    //Non-volatile parameter group
    EigenVariableGroup mailbox_group_;      //Volatile paramater group
};

#endif // EIGENGROUP_H
