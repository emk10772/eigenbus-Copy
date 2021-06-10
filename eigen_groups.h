#ifndef EIGENGROUP_H
#define EIGENGROUP_H

#include "eigen_commands/eigen_command.h"
#include "eigen_module.h"
#include "eigen_variable.h"

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
    size_t count();
    void group_command(EigenCommand *command);
private:
    std::vector<ModuleConst> modules_;
    std::set<eigen_addr_t> module_addrs_;
    EigenVariableGroup variable_group_;
};

#endif // EIGENGROUP_H
