#include "eigen_command_simple.h"

EigenCommandSimple::EigenCommandSimple(eigen_addr_t address, std::string command_char, std::string response, packet_type type, command_t command_type)
    : EigenCommand(address, type, command_type), command_char_(command_char), response_(response) {

}

std::string EigenCommandSimple::packet() const{
    return strprintf("%02x%s", address_, command_char_.c_str());
}

std::string EigenCommandSimple::expected_response() const{
    return response_;
}

EigenCommand *EigenCommandSimple::clone() const{
    return new EigenCommandSimple(*this);
}

EigenCommand *EigenCommandSimple::clone(eigen_addr_t addr) const{
    return new EigenCommandSimple(addr, command_char_, response_, type_, command_);
}


EigenCommandRun::EigenCommandRun(eigen_addr_t address)
    : EigenCommandSimple(address, "R", "", EIGEN_PACKET_DEFAULT, EIGEN_CMD_RUN){
}

EigenCommandZero::EigenCommandZero(eigen_addr_t address, std::string arg)
    : EigenCommandSimple(address, "Z" + arg, "", EIGEN_PACKET_DEFAULT, EIGEN_CMD_ZERO){
}

EigenCommandTopology::EigenCommandTopology(eigen_addr_t address)
    : EigenCommandSimple(address, "O", "S", EIGEN_PACKET_TOPO, EIGEN_CMD_TOPOLOGY){
}

EigenCommandEcho::EigenCommandEcho(eigen_addr_t address, std::string echo_string)
    : EigenCommandSimple(address, "@" + echo_string, "@" + echo_string, EIGEN_PACKET_DEBUG, EIGEN_CMD_ECHO){
}
