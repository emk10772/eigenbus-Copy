#include "eigen_command_float.h"

EigenCommandFloat::EigenCommandFloat(eigen_addr_t address, std::string command, std::string response, double value) 
    : EigenCommand(address), command_(command), response_(response), value_(value){

}

std::string EigenCommandFloat::packet(){
    return strprintf("%02x%s%08.4f", address_, command_, value_);
}

std::string EigenCommandFloat::expected_response(){
    if(response_ != "")
        return strprintf("%02x%s", address_, response_);
    return "";
}


EigenCommandPosition::EigenCommandPosition(eigen_addr_t address, double value)
    : EigenCommandFloat(address, "P", "", value){
}

EigenCommandVelocity::EigenCommandVelocity(eigen_addr_t address, double value)
    : EigenCommandFloat(address, "S", "", value){
}

EigenCommandEffort::EigenCommandEffort(eigen_addr_t address, double value)
    : EigenCommandFloat(address, "T", "", value){
}

