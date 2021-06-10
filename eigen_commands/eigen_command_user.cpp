#include "eigen_command_user.h"


EigenCommandUser::EigenCommandUser(eigen_addr_t address, std::string command_str, std::string response)
    : EigenCommand(address, EIGEN_PACKET_CLI, EIGEN_CMD_USER), command_str_(command_str), response_(response) {
}

std::string EigenCommandUser::packet() const{
    return command_str_;
}

std::string EigenCommandUser::expected_response() const{
    return response_;
}

EigenCommand *EigenCommandUser::clone() const{
    return new EigenCommandUser(*this);
}

EigenCommand *EigenCommandUser::clone(eigen_addr_t addr) const{
    return new EigenCommandUser(addr, command_str_, response_);
}
