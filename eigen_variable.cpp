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

bool EigenVariable::weak_match(EigenVariable &variable) const {
    return (variable.name_ == name_ && variable.type_ == type_);
}


/* ==== EigenUint8 ==== */
EigenUint8::EigenUint8(std::string name, uint8_t value) : EigenVariable(name, UINT8){
    this->value_ = value;
}

bool EigenUint8::strong_match(EigenVariable &variable) const {
    if(!weak_match(variable))
        return false;

    try {
        auto var_casted = dynamic_cast<EigenUint8 &>(variable);
        return var_casted.value_ == value_;
    } catch(std::exception e){
        return false;
    }
}

std::string EigenUint8::print() const {
    return strprintf("%02X", value_);
}


/* ==== EigenUint16 ==== */
EigenUint16::EigenUint16(std::string name, uint16_t value) : EigenVariable(name, UINT16){
    this->value_ = value;
}

bool EigenUint16::strong_match(EigenVariable &variable) const {
    if(!weak_match(variable))
        return false;

    try {
        auto var_casted = dynamic_cast<EigenUint16 &>(variable);
        return var_casted.value_ == value_;
    } catch(std::exception e){
        return false;
    }
}

std::string EigenUint16::print() const {
    return strprintf("%04X", value_);
}


/* ==== EigenUint32 ==== */
EigenUint32::EigenUint32(std::string name, uint32_t value) : EigenVariable(name, UINT32){
    this->value_ = value;
}

bool EigenUint32::strong_match(EigenVariable &variable) const {
    if(!weak_match(variable))

    try {
        auto var_casted = dynamic_cast<EigenUint32 &>(variable);
        return var_casted.value_ == value_;
    } catch(std::exception e){
        return false;
    }
}

std::string EigenUint32::print() const {
    return strprintf("%08lX", value_);
}


/* ==== EigenUint64 ==== */
EigenUint64::EigenUint64(std::string name, uint64_t value) : EigenVariable(name, UINT64){
    this->value_ = value;
}

bool EigenUint64::strong_match(EigenVariable &variable) const {
    if(!weak_match(variable))
        return false;

    try {
        auto var_casted = dynamic_cast<EigenUint64 &>(variable);
        return var_casted.value_ == value_;
    } catch(std::exception e){
        return false;
    }
}

std::string EigenUint64::print() const {
    return strprintf("%016llX", value_);
}


/* ==== EigenDouble ==== */
EigenDouble::EigenDouble(std::string name, double value) : EigenVariable(name, DOUBLE){
    this->value_ = value;
}

bool EigenDouble::strong_match(EigenVariable &variable) const {
    if(!weak_match(variable))
        return false;

    try {
        auto var_casted = dynamic_cast<EigenDouble &>(variable);
        return var_casted.value_ == value_;
    } catch(std::exception e){
        return false;
    }
}

std::string EigenDouble::print() const {
    return strprintf("%08.4f", value_);
}


/* ==== EigenString ==== */
EigenString::EigenString(std::string name, std::string value) : EigenVariable(name, STRING){
    this->value_ = value;
}

bool EigenString::strong_match(EigenVariable &variable) const {
    if(!weak_match(variable))
        return false;

    try {
        auto var_casted = dynamic_cast<EigenString &>(variable);
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

void EigenVariableGroup::add_variable(EigenVariable &variable) {
    if(variable_map_.count(variable.name()) > 0){
        common_keys_.emplace_back(variable.name());
    }
    variable_map_.emplace(variable.name(), variable);
}

void EigenVariableGroup::add_variables(std::vector<EigenVariable &> variables) {
    for(auto variable : variables){
        add_variable(variable);
    }
}

std::string EigenVariableGroup::print_variable(std::string key) {
    auto it = variable_map_.find(key);
    if(it != variable_map_.end()){

    }
    //if(variable_map_.count(key) > 0)
}

std::vector<std::string> EigenVariableGroup::common_keys() {

}
