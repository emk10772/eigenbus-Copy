#include "eigen_command_float.h"

EigenCommandFloat::EigenCommandFloat(eigen_addr_t address, std::string command, std::string response, double value, command_t command_type) 
    : EigenCommand(address, EIGEN_PACKET_DEFAULT, command_type), command_(command), response_(response), value_(value){

}

std::string EigenCommandFloat::packet(){
    return strprintf("%02x%s%08.4f", address_, command_.c_str(), value_);
}

std::string EigenCommandFloat::expected_response(){
    return response_;
}


EigenCommandPosition::EigenCommandPosition(eigen_addr_t address, double value)
    : EigenCommandFloat(address, "P", "", value, EIGEN_CMD_POSITION){
}

EigenCommandVelocity::EigenCommandVelocity(eigen_addr_t address, double value)
    : EigenCommandFloat(address, "S", "", value, EIGEN_CMD_VELOCITY){
}

EigenCommandEffort::EigenCommandEffort(eigen_addr_t address, double value)
    : EigenCommandFloat(address, "T", "", value, EIGEN_CMD_EFFORT){
}

