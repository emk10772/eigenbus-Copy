#include "eigen_command_param.h"

EigenCommandParamRead::EigenCommandParamRead(eigen_addr_t address, uint8_t id) 
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_PARAM_READ){
    id_ = id;
}

std::string EigenCommandParamRead::packet(){
    return strprintf("%02x(%02x", address_, id_);
}

std::string EigenCommandParamRead::expected_response(){
    return strprintf("|(%02X", id_);
}

std::string EigenCommandParamWrite::packet(){
    switch(param_type_){
    case _UINT8:
        return strprintf("%02X)%02X,%02X,8675309", address_, id_, value_.uint8_);
    case _UINT16:
        return strprintf("%02X)%02X,%04X,8675309", address_, id_, value_.uint16_);
    case _UINT32:
        return strprintf("%02X)%02X,%08X,8675309", address_, id_, value_.uint32_);
    case _UINT64:
        return strprintf("%02X)%02X,%016llX,8675309", address_, id_, value_.uint64_);
    case _FLOAT:
        return strprintf("%02X)%02X,%08.4f,8675309", address_, id_, value_.float_);
    case _DOUBLE:
        return strprintf("%02X)%02X,%08.4f,8675309", address_, id_, value_.double_); 
    default: 
        return "ERR";
    }
}

std::string EigenCommandParamWrite::expected_response(){
    //Check if the param type is valid
    if(param_type_ == 0 || param_type_ > PARAM_TYPE_MAX) return "";

    //Return expected response if param type is correct
    return strprintf("|)%02X", id_);
}

EigenCommandParamWrite::EigenCommandParamWrite(eigen_addr_t address, uint8_t id, uint8_t value)
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_PARAM_WRITE){
    value_.uint8_ = value;
    param_type_ = _UINT8;
    id_ = id;
}

EigenCommandParamWrite::EigenCommandParamWrite(eigen_addr_t address, uint8_t id, uint16_t value)
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_PARAM_WRITE){
    value_.uint16_ = value;
    param_type_ = _UINT16;
    id_ = id;
}

EigenCommandParamWrite::EigenCommandParamWrite(eigen_addr_t address, uint8_t id, uint32_t value)
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_PARAM_WRITE){
    value_.uint32_ = value;
    param_type_ = _UINT32;
    id_ = id;
}

EigenCommandParamWrite::EigenCommandParamWrite(eigen_addr_t address, uint8_t id, uint64_t value)
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_PARAM_WRITE){
    value_.uint64_ = value;
    param_type_ = _UINT64;
    id_ = id;
}

EigenCommandParamWrite::EigenCommandParamWrite(eigen_addr_t address, uint8_t id, float value)
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_PARAM_WRITE){
    value_.float_ = value;
    param_type_ = _FLOAT;
    id_ = id;
}

EigenCommandParamWrite::EigenCommandParamWrite(eigen_addr_t address, uint8_t id, double value)
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_PARAM_WRITE){
    value_.double_ = value;
    param_type_ = _DOUBLE;
    id_ = id;
}

EigenCommandParamWrite::EigenCommandParamWrite(eigen_addr_t address, uint8_t id, eigen_param_t value, uint8_t param_type)
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_PARAM_WRITE){
    value_ = value;
    param_type_ = param_type_;
    id_ = id;
}
