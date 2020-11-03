#include "eigen_command_simple.h"

EigenCommandSimple::EigenCommandSimple(eigen_addr_t address, std::string command, std::string response) 
    : EigenCommand(address), command_(command), response_(response){

}

std::string EigenCommandSimple::packet(){
    return strprintf("%02x%s", address_, command_);
}

std::string EigenCommandSimple::expected_response(){
    if(response_ != "")
        return strprintf("%02x%s", address_, response_);
    return "";
}


EigenCommandRun::EigenCommandRun(eigen_addr_t address)
    : EigenCommandSimple(address, "R", ""){
}

EigenCommandZero::EigenCommandZero(eigen_addr_t address)
    : EigenCommandSimple(address, "Z", ""){
}

EigenCommandTopology::EigenCommandTopology(eigen_addr_t address)
    : EigenCommandSimple(address, "O", "S"){
}
