#include "eigen_command_utility.h"

EigenCommandUtility::EigenCommandUtility(eigen_addr_t address, uint16_t mode)
    : EigenCommand(address, EIGEN_PACKET_POLL, EIGEN_CMD_UTILITY), mode_(mode) {
}

std::string EigenCommandUtility::packet() const{
    return strprintf("%02xU%02x", address_, mode_);
}

std::string EigenCommandUtility::expected_response() const{
    return strprintf("U%02x", mode_);
}

EigenCommand *EigenCommandUtility::clone() const{
    return new EigenCommandUtility(*this);
}
