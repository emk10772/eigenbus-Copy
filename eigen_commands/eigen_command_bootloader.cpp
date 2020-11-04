#include "eigen_command_bootloader.h"


EigenCommandBootloader::EigenCommandBootloader(eigen_addr_t address, btldr_op_t action)
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_BOOTLOADER), action_(action) {
    this->msg_ = "";
}

EigenCommandBootloader::EigenCommandBootloader(eigen_addr_t address, std::string msg)
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_BOOTLOADER), msg_(msg) {
    this->action_ = BOOTLOADER_RESEND;
}

std::string EigenCommandBootloader::packet(){
    switch(action_){
        case BOOTLOADER_ACK:
            return strprintf("%02x~a", address_);
        case BOOTLOADER_RESEND:
            return strprintf("%02x~s,%s", address_, msg_);
        case BOOTLOADER_START:
            return strprintf("%02x~b", address_);
        default:
            break;
    }
}
std::string EigenCommandBootloader::expected_response(){
    //Expecting no response
    return "";
}
