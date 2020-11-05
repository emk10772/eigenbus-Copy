#include "eigen_command_simple.h"

EigenCommandSimple::EigenCommandSimple(eigen_addr_t address, std::string command, std::string response, packet_type type, command_t command_type) 
    : EigenCommand(address, type, command_type), command_(command), response_(response) {

}

std::string EigenCommandSimple::packet(){
    return strprintf("%02x%s", address_, command_.c_str());
}

std::string EigenCommandSimple::expected_response(){
    return response_;
}


EigenCommandRun::EigenCommandRun(eigen_addr_t address)
    : EigenCommandSimple(address, "R", "", EIGEN_PACKET_DEFAULT, EIGEN_CMD_RUN){
}

EigenCommandZero::EigenCommandZero(eigen_addr_t address)
    : EigenCommandSimple(address, "Z", "", EIGEN_PACKET_DEFAULT, EIGEN_CMD_ZERO){
}

EigenCommandTopology::EigenCommandTopology(eigen_addr_t address)
    : EigenCommandSimple(address, "O", "S", EIGEN_PACKET_TOPO, EIGEN_CMD_TOPOLOGY){
}
