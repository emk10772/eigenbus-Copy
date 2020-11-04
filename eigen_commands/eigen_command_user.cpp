#include "eigen_command_user.h"


EigenCommandUser::EigenCommandUser(eigen_addr_t address, std::string command, std::string response = "")
    : EigenCommand(address, EIGEN_PACKET_CLI, EIGEN_CMD_USER), command_(command), response_(response) {
}

std::string EigenCommandUser::packet(){
    return command_;
}
std::string EigenCommandUser::expected_response(){
    return response_;
}
