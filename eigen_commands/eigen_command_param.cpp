#include "eigen_command_param.h"

EigenCommandParamRead::EigenCommandParamRead(eigen_addr_t address, uint8_t id) 
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_PARAM_READ){
    id_ = id;
}

std::string EigenCommandParamRead::packet() const{
    return strprintf("%02x(%02x", address_, id_);
}

std::string EigenCommandParamRead::expected_response() const{
    return strprintf("|(%02X", id_);
}

EigenCommand *EigenCommandParamRead::clone() const{
    return new EigenCommandParamRead(*this);
}

EigenCommand *EigenCommandParamRead::clone(eigen_addr_t addr) const{
    return new EigenCommandParamRead(addr, id_);
}



std::string EigenCommandParamWrite::packet() const{
    return strprintf("%02X)%02X,%s,8675309", address_, id_, param_.print().c_str());
}

std::string EigenCommandParamWrite::expected_response() const{
    //Check if the param type is valid
    if(param_.type() == 0 || param_.type() > PARAM_TYPE_MAX) return "";

    //Return expected response if param type is correct
    return strprintf("|)%02X", id_);
}

EigenCommandParamWrite::EigenCommandParamWrite(eigen_addr_t address, uint8_t id, EigenParameter param)
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_PARAM_WRITE), param_(param){
    id_ = id;
}

EigenCommand *EigenCommandParamWrite::clone() const{
    return new EigenCommandParamWrite(*this);
}

EigenCommand *EigenCommandParamWrite::clone(eigen_addr_t addr) const{
    return new EigenCommandParamWrite(addr, id_, param_);
}
