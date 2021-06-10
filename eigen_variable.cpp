#include "eigen_variable.h"
#include "eigen_utils.h"

EigenVariable::EigenVariable(std::string name, EigenVarType type) : name_(name), type_(type){

}

std::string EigenVariable::name() const {
    return name_;
}

EigenVarType EigenVariable::type() const {
    return type_;
}

bool EigenVariable::weak_match(const EigenVariable &variable) const {
    return (variable.name_ == name_ && variable.type_ == type_);
}


/* ==== EigenUint8 ==== */
EigenUint8::EigenUint8(std::string name, uint8_t value) : EigenVariable(name, EIGEN_UINT8){
    this->value_ = value;
}

bool EigenUint8::strong_match(const EigenVariable &variable) const {
    if(!weak_match(variable))
        return false;

    try {
        auto var_casted = dynamic_cast<const EigenUint8 &>(variable);
        return var_casted.value_ == value_;
    } catch(std::exception e){
        return false;
    }
}

std::string EigenUint8::print() const {
    return strprintf("%02X", value_);
}


/* ==== EigenUint16 ==== */
EigenUint16::EigenUint16(std::string name, uint16_t value) : EigenVariable(name, EIGEN_UINT16){
    this->value_ = value;
}

bool EigenUint16::strong_match(const EigenVariable &variable) const {
    if(!weak_match(variable))
        return false;

    try {
        auto var_casted = dynamic_cast<const EigenUint16 &>(variable);
        return var_casted.value_ == value_;
    } catch(std::exception e){
        return false;
    }
}

std::string EigenUint16::print() const {
    return strprintf("%04X", value_);
}


/* ==== EigenUint32 ==== */
EigenUint32::EigenUint32(std::string name, uint32_t value) : EigenVariable(name, EIGEN_UINT32){
    this->value_ = value;
}

bool EigenUint32::strong_match(const EigenVariable &variable) const {
    if(!weak_match(variable))

    try {
        auto var_casted = dynamic_cast<const EigenUint32 &>(variable);
        return var_casted.value_ == value_;
    } catch(std::exception e){
        return false;
    }

    return false;
}

std::string EigenUint32::print() const {
    return strprintf("%08lX", value_);
}


/* ==== EigenUint64 ==== */
EigenUint64::EigenUint64(std::string name, uint64_t value) : EigenVariable(name, EIGEN_UINT64){
    this->value_ = value;
}

bool EigenUint64::strong_match(const EigenVariable &variable) const {
    if(!weak_match(variable))
        return false;

    try {
        auto var_casted = dynamic_cast<const EigenUint64 &>(variable);
        return var_casted.value_ == value_;
    } catch(std::exception e){
        return false;
    }
}

std::string EigenUint64::print() const {
    return strprintf("%016llX", value_);
}


/* ==== EigenDouble ==== */
EigenDouble::EigenDouble(std::string name, double value) : EigenVariable(name, EIGEN_DOUBLE){
    this->value_ = value;
}

bool EigenDouble::strong_match(const EigenVariable &variable) const {
    if(!weak_match(variable))
        return false;

    try {
        auto var_casted = dynamic_cast<const EigenDouble &>(variable);
        return var_casted.value_ == value_;
    } catch(std::exception e){
        return false;
    }
}

std::string EigenDouble::print() const {
    return strprintf("%08.4f", value_);
}


/* ==== EigenString ==== */
EigenString::EigenString(std::string name, std::string value) : EigenVariable(name, EIGEN_STRING){
    this->value_ = value;
}

bool EigenString::strong_match(const EigenVariable &variable) const {
    if(!weak_match(variable))
        return false;

    try {
        auto var_casted = dynamic_cast<const EigenString &>(variable);
        return var_casted.value_ == value_;
    } catch(std::exception e){
        return false;
    }
}

std::string EigenString::print() const {
    return value_;
}



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
