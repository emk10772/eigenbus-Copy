#include "eigen_variable.h"
#include "eigen_utils.h"

EigenVariable::EigenVariable(std::string name, EigenVarType type)
    : name_(name), type_(type){
    id_ = 0;
    mod_address_ = 0xFF;
    addr_set_ = false;
    id_set_ = false;
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

eigen_addr_t EigenVariable::id() const {
    return id_;
}

eigen_addr_t EigenVariable::module_address() const {
    return mod_address_;
}

bool EigenVariable::parent_info_valid() const {
    return id_set_ && addr_set_ && mod_address_ != 0xFF;
}

void EigenVariable::set_id(eigen_addr_t id) {
    id_set_ = true;
    id_ = id;
}

void EigenVariable::set_address(eigen_addr_t addr) {
    addr_set_ = true;
    mod_address_ = addr;
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

bool EigenUint8::parse_value(std::string encoded) {
    try{
        uint64_t parsed = std::stoull(encoded, nullptr, EIGENBUS_BASE);
        if(parsed < std::numeric_limits<uint8_t>::max()){
            value_ = (uint8_t) parsed;
            return true;
        }
    } catch(std::exception e) {
        return false;
    }
    return false;
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

bool EigenUint16::parse_value(std::string encoded) {
    try{
        uint64_t parsed = std::stoull(encoded, nullptr, EIGENBUS_BASE);
        if(parsed < std::numeric_limits<uint16_t>::max()){
            value_ = (uint16_t) parsed;
            return true;
        }
    } catch(std::exception e) {
        return false;
    }
    return false;
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

bool EigenUint32::parse_value(std::string encoded) {
    try{
        uint64_t parsed = std::stoull(encoded, nullptr, EIGENBUS_BASE);
        if(parsed < std::numeric_limits<uint32_t>::max()){
            value_ = (uint32_t) parsed;
            return true;
        }
    } catch(std::exception e) {
        return false;
    }
    return false;
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

bool EigenUint64::parse_value(std::string encoded) {
    try{
        uint64_t parsed = std::stoull(encoded, nullptr, EIGENBUS_BASE);
        value_ = (uint64_t) parsed;
        return true;
    } catch(std::exception e) {
        return false;
    }
    return false;
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

bool EigenDouble::parse_value(std::string encoded) {
    try{
        double parsed = stod(encoded);
        if(std::isfinite(parsed)){
            value_ = parsed;
            return true;
        }
    } catch(std::exception e) {
        return false;
    }
    return false;
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

bool EigenString::parse_value(std::string encoded) {
    value_ = encoded;
    return true;
}
