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

EigenUpdate *EigenResponseBootloader::update_module(ModuleShared mod){
    return nullptr;
    //id_ = stoul(packet_, &ind, EIGENBUS_BASE);
    /*
    ptr = buffer + 4;
    if(ptr[0] == 'r'){
        ptr++;
        bootloader_data.push_back(std::string((char *) ptr));
        //add_module_update(addr, MODULE_BTLDR, std::string((char *) ptr));
    } else if(ptr[0] == 'h'){
        ptr++;
        if(bootloader_active == true && bootloader_target_addr == module->get_address()){
            if(!bootloader_ack) {
                bootloader_ack = true;
                acknwoledge_bootload(bootloader_target_addr);

                int retval = 0;
                if(bootloader_mode == 1){
                    retval = CyBtldr_Program(bootloader_file.c_str(), NULL, (uint8_t) 3, &comm_struct, &bootloader_update);
                } else if(bootloader_mode == 2){
                    retval = CyBtldr_Verify(bootloader_file.c_str(), NULL, &comm_struct, &bootloader_update);
                } else if(bootloader_mode == 3) {
                    retval = CyBtldr_Erase(bootloader_file.c_str(), NULL, &comm_struct, &bootloader_update);
                }

                //If successful, retval = 0. Mark as finished.
                if(!retval){
                    bootloader_finished = true;
                    //uid_write_node_addr(module->get_address(), module->get_UID(), module->get_address());
                }

                add_module_update(addr, MODULE_BTLDR_END, bootloader_print_error(retval));
            } else {
                acknwoledge_bootload(bootloader_target_addr);
            }
        }
    } else if(ptr[0] == 's'){
        ptr++;
        (*write_data)((uint8_t *)bootloader_last.c_str(), bootloader_last.length());
        //add_module_update(addr, MODULE_BTLDR, std::string((char *) ptr));
    }*/
}


bool EigenResponseBootloader::isSpontaneous(){
    return true;
}

