#include "eigen_response_bootloader.h"

#include <string>


EigenResponseBootloader::EigenResponseBootloader(eigen_addr_t address, std::string packet)
    : EigenResponse(address, packet, EigenResponse::EIGEN_BOOTLOADER) {

    data_ = "";

    switch(packet_[0]){
        case 'r':
            action_ = EigenResponseBootloader::BOOTLOADER_READ;
            data_ = packet.substr(1);
            break;
        case 'h':
            action_ = EigenResponseBootloader::BOOTLOADER_ACK;
            break;
        case 's':
            action_ = EigenResponseBootloader::BOOTLOADER_RESEND;
            break;
        default:
            action_ = EigenResponseBootloader::BOOTLOADER_INVALID;
            break;
    }
}

EigenResponseBootloader::btldr_response_t EigenResponseBootloader::btldr_action(){
    return action_;
}

std::string EigenResponseBootloader::data(){
    return data_;
}

EigenUpdate *EigenResponseBootloader::update_module(ModuleShared mod, uint64_t latency){
    return nullptr;
}


bool EigenResponseBootloader::isSpontaneous(){
    return true;
}

