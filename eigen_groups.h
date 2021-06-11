#ifndef EIGENGROUP_H
#define EIGENGROUP_H

#include "eigen_commands/eigen_command.h"
#include "eigen_module.h"
#include "eigen_variable.h"
#include <type_traits>

/* EigenVariableGroup
    A strict observer of EigenVariables. Used to look for common values between variables.
    Must ensure that this class does not outlive its variables
*/
class EigenVariableGroup {
public:
    EigenVariableGroup();

    void add_variable(const EigenVariable *variable);
    void add_variables(const std::vector<const EigenVariable *> variables);

    std::string print_variable(const std::string key) const;
    std::string print_variable_list(const std::string key) const;
    const std::vector<std::string> common_keys() const;
    bool values_match(const std::string key) const;
    template<typename T> const T* value(const std::string key) const{
        //using type = std::remove_pointer_t<T>;
        static_assert(std::is_base_of<EigenVariable, T>::value,
                            "Base type must be of EigenVariable");
        try {
            if(!values_match(key)) return nullptr;
            const EigenVariable *var = variable_map_.find(key)->second;
            //const T retval = dynamic_cast<const T>(var);
            return dynamic_cast<const T*>(var);;
        } catch (std::exception e) {
            return nullptr;
        }
        return nullptr;
    }
private:
    std::unordered_multimap<std::string, const EigenVariable *> variable_map_;
    std::vector<std::string> common_keys_;
};

class EigenModuleGroup {
public:
    EigenModuleGroup(std::vector<ModuleConst> modules);
    ~EigenModuleGroup();

    const EigenVariableGroup variable_group();
    bool contains_module(eigen_addr_t address);
    bool contains_module(ModuleConst module);
    size_t count();
    void group_command(EigenCommand *command);

    std::string group_poll(EigenCommand *command, uint64_t period_ms = UPDATE_PERIOD, bool enabled = true);
    void group_poll_set_enabled(std::string key, bool enabled = true);
    void group_poll_remove(std::string key);
    std::vector<ModuleConst> modules();

    //Wrappers for variable_group value functions
    template<typename T> const T* key_to_value(const std::string key) const{
        return variable_group_.value<T>(key);
    };
    std::string key_to_string(const std::string key) const;
    std::string key_to_list_string(const std::string key) const;
private:
    std::vector<ModuleConst> modules_;
    std::set<eigen_addr_t> module_addrs_;
    EigenVariableGroup variable_group_;
    std::unordered_multimap<std::string, std::string> poll_keys_;
};

#endif // EIGENGROUP_H
