#include "eigen_groups.h"
#include "eigen_comms.h"


/* ==== EigenVariableGroup ==== */
EigenVariableGroup::EigenVariableGroup() {

}

void EigenVariableGroup::add_variable(const EigenVariable *variable) {
    if(variable == nullptr)
        return;
    if(variable_map_.count(variable->name()) > 0){
        common_keys_.emplace_back(variable->name());
    }
    variable_map_.emplace(variable->name(), variable);
}

void EigenVariableGroup::add_variables(const std::vector<const EigenVariable *> variables){
    for(auto variable : variables)
        add_variable(variable);
}

std::string EigenVariableGroup::print_variable(const std::string key) const{
    auto its = variable_map_.equal_range(key);
    //Check if the key exists
    if(its.first != its.second){
        //Get the value of the first variable under this key
        const EigenVariable *variable = its.first->second;
        if(variable == nullptr) //This should never happen as long as we do proper enforcement in add_variable
            return "NULL";
        std::string result = variable->print();

        //Check if the other variables match. If they differ, return a placeholder
        auto it = its.first;
        it++;
        while(it != its.second){
            if(it->second != nullptr && !variable->strong_match(*it->second))
                return "*";
            it++;
        }

        return result;
    } else {
        return "N/A";
    }
}

std::string EigenVariableGroup::print_variable_list(const std::string key) const{
    auto its = variable_map_.equal_range(key);
    //Check if the key exists
    if(its.first != its.second){
        std::string result = "";

        auto it = its.first;
        while(it != its.second){
            if(it->second == nullptr)
                return "NULL";
            if(it != its.first)
                result.append(",");
            result.append(it->second->print());
            it++;
        }

        return result;
    } else {
        return "N/A";
    }
}

const std::vector<std::string> EigenVariableGroup::common_keys() const{
    return common_keys_;
}


/* ==== EigenModuleGroup ==== */

EigenModuleGroup::EigenModuleGroup(std::vector<ModuleConst> modules) {
    modules_ = modules;
    for(ModuleConst &module : modules){
        variable_group_.add_variables(module->variable_list);
        module_addrs_.emplace(module->address);
    }
}

EigenModuleGroup::~EigenModuleGroup() {
    modules_.clear();
}

const EigenVariableGroup EigenModuleGroup::variable_group() {
    return variable_group_;
}

bool EigenModuleGroup::contains_module(eigen_addr_t address) {
    return module_addrs_.count(address) > 0;
}

size_t EigenModuleGroup::count() {
    return module_addrs_.size();
}

void EigenModuleGroup::group_command(EigenCommand *command) {
    for(ModuleConst &module : modules_){
        EigenCommand *cloned_command = command->clone(module->address);
        add_command(cloned_command);
    }
    delete command;
}
