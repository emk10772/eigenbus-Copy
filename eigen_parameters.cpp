#include "eigen_parameters.h"
#include "eigen_comms.h"
#include <stdexcept>


/* EigenParameter definitions */
EigenParameter::EigenParameter(){
    type_ = 0;
    value_.uint64_ = 0;
}

EigenParameter::EigenParameter(uint8_t type){
    if(type > 0 && type <= PARAM_TYPE_MAX)
        type_ = type;
    else
        type_ = _UINT64;

    value_.uint64_ = 0;
}

EigenParameter::~EigenParameter(){

}

EigenParameter::eigen_param_t EigenParameter::value() const{
    return value_;
}

uint8_t EigenParameter::type() const{
    return type_;
}


std::string EigenParameter::print() const{
    return print_as(type_);
}

std::string EigenParameter::print_as(uint8_t type) const{
    switch(type){
    case _UINT8:
        return strprintf("%02X", value_.uint8_);
    case _UINT16:
        return strprintf("%04X", value_.uint16_);
    case _UINT32:
        return strprintf("%08X", value_.uint32_);
    case _UINT64:
        return strprintf("%016llX", value_.uint64_);
    case _FLOAT:
        return strprintf("%08.4f", value_.float_);
    case _DOUBLE:
        return strprintf("%08.4f", value_.double_);
    default:
        return "ERR";
    }
}

bool EigenParameter::update_value(uint8_t val){
    if(type_ != _UINT8) return false;

    value_.uint8_ = val;
    return true;
}

bool EigenParameter::update_value(uint16_t val){
    if(type_ != _UINT16) return false;

    value_.uint16_ = val;
    return true;
}

bool EigenParameter::update_value(uint32_t val){
    if(type_ != _UINT32) return false;

    value_.uint32_ = val;
    return true;
}

bool EigenParameter::update_value(uint64_t val){
    if(type_ != _UINT64) return false;

    value_.uint64_ = val;
    return true;
}

bool EigenParameter::update_value(float val){
    if(type_ != _FLOAT) return false;

    value_.float_ = val;
    return true;
}

bool EigenParameter::update_value(double val){
    if(type_ != _DOUBLE) return false;

    value_.double_ = val;
    return true;
}

bool EigenParameter::update_value(std::string val){
    try{
        switch(type_){
            case _UINT8: {
                uint64_t parsed = std::stoull(val, nullptr, EIGENBUS_BASE);
                if(parsed < std::numeric_limits<uint8_t>::max()){
                    value_.uint8_ = (uint8_t) parsed;
                    return true;
                }
                break;
            }
            case _UINT16: {
                uint64_t parsed = std::stoull(val, nullptr, EIGENBUS_BASE);
                if(parsed < std::numeric_limits<uint16_t>::max()){
                    value_.uint8_ = (uint16_t) parsed;
                    return true;
                }
                break;
            }
            case _UINT32: {
                uint64_t parsed = std::stoull(val, nullptr, EIGENBUS_BASE);
                if(parsed < std::numeric_limits<uint32_t>::max()){
                    value_.uint8_ = (uint32_t) parsed;
                    return true;
                }
                break;
            }
            case _UINT64: {
                uint64_t parsed = std::stoull(val, nullptr, EIGENBUS_BASE);
                value_.uint64_ = (uint64_t) parsed;
                return true;
            }
            case _FLOAT: {
                float parsed = stof(val);
                if(isfinite(parsed)){
                    value_.float_ = parsed;
                    return true;
                }
                break;
            }
            case _DOUBLE: {
                double parsed = stod(val);
                if(isfinite(parsed)){
                    value_.double_ = parsed;
                    return true;
                }
                break;
            }
            default:
                return false;
        }
        return false;

    } catch (std::exception e){
        //Do not update the value if the parse did not work
        return false;
    }

    return false;
}



/* EigenMailbox Definition */
EigenMailbox::EigenMailbox(uint8_t type){
    type_hint_ = type & MAILBOX_TYPE_MASK;
    access_ = type & MAILBOX_RW_MASK;

    raw_value_ = "";
    parsed_int_ = 0;
    parsed_double_ = 0.0;
    parse_valid = false;
}

EigenMailbox::EigenMailbox(){
    type_hint_ = MAILBOX_STRING;
    access_ = MAILBOX_READ_ONLY;

    raw_value_ = "";
    parsed_int_ = 0;
    parsed_double_ = 0.0;
    parse_valid = false;
}


EigenMailbox::~EigenMailbox(){

}

uint8_t EigenMailbox::type() const{
    return type_hint_ | access_;
}

bool EigenMailbox::plottable() const{
    return (type_hint_ == MAILBOX_INT) || (type_hint_ == MAILBOX_DOUBLE);
}

bool EigenMailbox::update_value(std::string val){
    raw_value_ = val;

    try{
        if(type_hint_ == MAILBOX_INT){
            parsed_int_ = std::stoul(raw_value_, nullptr, EIGENBUS_BASE);
            parse_valid = true;
        } else if(type_hint_ == MAILBOX_DOUBLE) {
            parsed_double_ = std::stod(raw_value_);
            parse_valid = isfinite(parsed_double_);
        }
    } catch (std::exception e){
        parse_valid = false;
    }

    return true;
}

bool EigenMailbox::valid() const{
    if(type_hint_ == MAILBOX_INT || type_hint_ == MAILBOX_DOUBLE){
        return parse_valid;
    } else if(type_hint_ == MAILBOX_STRING) {
        return true;
    }
    return false;
}

std::string EigenMailbox::print() const {
    return raw_value_;
}

double EigenMailbox::as_float() const {
    if(type_hint_ == MAILBOX_INT && parse_valid){
        return parsed_int_;
    } else if(type_hint_ == MAILBOX_DOUBLE && parse_valid) {
        return parsed_double_;
    } else {
        return nan("");
    }
}
