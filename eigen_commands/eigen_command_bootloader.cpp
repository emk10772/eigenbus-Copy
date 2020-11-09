#include "eigen_command_bootloader.h"


EigenCommandBootloader::EigenCommandBootloader(eigen_addr_t address, btldr_op_t action, uint8_t mode, std::string msg)
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_BOOTLOADER), action_(action) {
    this->msg_ = msg;
    this->mode_ = mode;
}

EigenCommandBootloader::EigenCommandBootloader(eigen_addr_t address, std::string msg)
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_BOOTLOADER), msg_(msg) {
    this->action_ = BOOTLOADER_RESEND;
    this->mode_ = 0;
}

EigenCommandBootloader::EigenCommandBootloader(eigen_addr_t address, uint8_t mode, std::string msg)
    : EigenCommand(address, EIGEN_PACKET_DEBUG, EIGEN_CMD_BOOTLOADER), msg_(msg) {
    this->action_ = BOOTLOADER_START;
    this->mode_ = mode;
}

std::string EigenCommandBootloader::packet() const{
    switch(action_){
        case BOOTLOADER_ACK:
            return strprintf("%02x~a", address_);
        case BOOTLOADER_RESEND:
            return strprintf("%02x~s,%s", address_, msg_.c_str());
        case BOOTLOADER_START:
            return strprintf("%02x~b", address_);
        default:
            break;
    }
    return "";
}
std::string EigenCommandBootloader::expected_response() const{
    //Expecting no response
    return "";
}

EigenCommand *EigenCommandBootloader::clone() const{
    return new EigenCommandBootloader(*this);
}


std::string EigenCommandBootloader::msg(){
    return msg_;
}

uint8_t EigenCommandBootloader::mode(){
    return mode_;
}

EigenCommandBootloader::btldr_op_t EigenCommandBootloader::action(){
    return action_;
}
