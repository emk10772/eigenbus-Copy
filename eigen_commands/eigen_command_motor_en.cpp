#include "eigen_command_motor_en.h"


EigenCommandMotorEnable::EigenCommandMotorEnable(eigen_addr_t address, uint8_t enable)
    : EigenCommand(address, EIGEN_PACKET_DEFAULT, EIGEN_CMD_MOTOR_EN), enable_(enable) {
}

std::string EigenCommandMotorEnable::packet(){
    return strprintf("%02xM%02x", address_, enable_);
}

std::string EigenCommandMotorEnable::expected_response(){
    //We do not expect a response to this command
    return "";
}
