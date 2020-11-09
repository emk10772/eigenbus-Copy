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
    return strprintf("%02X)%02X,%s,8675309", address_, id_, param_.print().c_str());
}

std::string EigenCommandParamWrite::expected_response(){
    //Check if the param type is valid
    if(param_.type() == 0 || param_.type() > PARAM_TYPE_MAX) return "";

    //Return expected response if param type is correct
    return strprintf("|)%02X", id_);
}

EigenCommandParamWrite::EigenCommandParamWrite(eigen_addr_t address, uint8_t id, EigenParameter param)
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_PARAM_WRITE), param_(param){
    id_ = id;
}
