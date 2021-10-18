#include "eigen_groups.h"
#include "eigen_comms.h"


/* ==== EigenVariableGroup ==== */
EigenVariableGroup::EigenVariableGroup(eigen_addr_t min_count) {
    min_count_ = min_count;
}

void EigenVariableGroup::add_variable(const EigenVariable *variable) {
    if(variable == nullptr)
        return;
    variable_map_.emplace(variable->name(), variable);
    if(variable_map_.count(variable->name()) >= min_count_){
        common_keys_.emplace_back(variable->name());
    }
}

void EigenVariableGroup::add_variables(const std::vector<const EigenVariable *> variables) {
    for(auto variable : variables)
        add_variable(variable);
}

void EigenVariableGroup::add_variables(EigenVariableGroup *group) {
    for(auto &item : group->variable_map_)
        add_variable(item.second);
}

bool EigenVariableGroup::values_match(const std::string key) const{
    auto its = variable_map_.equal_range(key);
    //Check if the key exists
    if(its.first != its.second){
        //Get the value of the first variable under this key
        const EigenVariable *variable = its.first->second;
        if(variable == nullptr) //This should never happen as long as we do proper enforcement in add_variable
            return false;

        //Check if the other variables match. If they differ, return false
        auto it = its.first;
        it++;
        while(it != its.second){
            if(it->second != nullptr && !variable->strong_match(*it->second))
                return false;
            it++;
        }

        return true;
    } else {
        return false;
    }
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

const std::vector<const EigenVariable *> EigenVariableGroup::variables(const std::string key) const {
    auto vars = variable_map_.equal_range(key);
    std::vector<const EigenVariable *> retval = {};
    for(auto it = vars.first; it != vars.second; ++it)
        retval.emplace_back(it->second);
    return retval;
}

int EigenVariableGroup::common_key_index(std::string key) const {
    auto it = std::find(common_keys_.begin(), common_keys_.end(), key);
    if(it == common_keys_.end())
        return -1;
    return std::distance(common_keys_.begin(), it);
}


/* ==== EigenModuleGroup ==== */

EigenModuleGroup::EigenModuleGroup(std::vector<ModuleConst> modules) {
    modules_ = modules;
    variable_group_ = EigenVariableGroup((eigen_addr_t)modules.size());
    parameter_group_ = EigenVariableGroup((eigen_addr_t)modules.size());
    mailbox_group_ = EigenVariableGroup((eigen_addr_t)modules.size());

    for(ModuleConst &module : modules){
        variable_group_.add_variables(module->variable_list);
        parameter_group_.add_variables(module->parameters.list());
        mailbox_group_.add_variables(module->mailboxes.list());
        module_addrs_.emplace(module->address);
    }
}

EigenModuleGroup::~EigenModuleGroup() {
    modules_.clear();

    for(auto &key : poll_keys_){
        remove_poll_command(key.second);
    }
    poll_keys_.clear();
}

const EigenVariableGroup EigenModuleGroup::variable_group() {
    return variable_group_;
}

const EigenVariableGroup EigenModuleGroup::parameter_group() {
    return parameter_group_;
}

const EigenVariableGroup EigenModuleGroup::mailbox_group() {
    return mailbox_group_;
}

bool EigenModuleGroup::contains_module(eigen_addr_t address) {
    return module_addrs_.count(address) > 0;
}

bool EigenModuleGroup::contains_module(ModuleConst mod) {
    return module_addrs_.count(mod->address) > 0;
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

std::string EigenModuleGroup::group_poll(EigenCommand *command, uint64_t period_ms, bool enabled) {
    std::string main_key = command->packet();
    for(auto &module : modules_) {
        std::string key = add_poll_command(command->clone(module->address), period_ms, enabled);
        poll_keys_.emplace(main_key, key);
    }
    return main_key;
}

void EigenModuleGroup::group_poll_set_enabled(std::string poll_key, bool enabled) {
    auto keys = poll_keys_.equal_range(poll_key);
    auto it = keys.first;
    while(it != keys.second){
        set_poll_enable(it->second, enabled);
        it++;
    }
}

void EigenModuleGroup::group_poll_remove(std::string poll_key) {
    auto keys = poll_keys_.equal_range(poll_key);
    auto it = keys.first;
    while(it != keys.second){
        remove_poll_command(it->second);
        it++;
    }
    poll_keys_.erase(poll_key);
}

std::vector<ModuleConst> EigenModuleGroup::modules(){
    return modules_;
}

std::string EigenModuleGroup::key_to_string(const std::string key) const {
    return variable_group_.print_variable(key);
}

std::string EigenModuleGroup::key_to_list_string(const std::string key) const {
    return variable_group_.print_variable_list(key);
}

std::string EigenModuleGroup::print_list(const eigen_addr_t max_len) const {
    std::string retval = "";
    for(auto &mod : modules_){
        retval.append(strprintf("%02X, ", (uint8_t)mod->address));
    }

    //Remove the trailing comma and space
    if(retval.size() > 0)
        retval = retval.substr(0, retval.size() - 2);


    if(max_len > 0 && retval.size() > max_len)
        retval = retval.substr(0, max_len - 3).append("...");

    return retval;
}
