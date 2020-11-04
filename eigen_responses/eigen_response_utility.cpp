#include "eigen_response_utility.h"

#include <string>


EigenResponseUtility::EigenResponseUtility(std::string packet)
    : EigenResponse(packet, EigenResponse::EIGEN_UTILITY) {
}

bool EigenResponseUtility::update_module(ModuleShared mod){
    size_t ind = 0;
    id_ = stoul(packet_, &ind, EIGENBUS_BASE);

    switch(id_){
        case EIGEN_UTIL_STAT_CODE: {
            uint8_t status_code = stoul(packet_.substr(ind+1), nullptr, EIGENBUS_BASE);
            if(status_code != mod->status_code){
                add_command(mod->get_address(), CMD_FIRMWARE_UTIL, EIGEN_UTIL_MODULE_STATUS);
            }
            break;
        }
        case EIGEN_UTIL_COMMIT_VERSION: {
            mod->firmware_version = packet_.substr(ind+1);
            break;
        }
        case EIGEN_UTIL_BUILD_TIME: {
            mod->firmware_build_time = packet_.substr(ind+1);
            break;
        }
        case EIGEN_UTIL_BUILD_USER: {
            mod->firmware_build_name = packet_.substr(ind+1);
            break;
        }
        case EIGEN_UTIL_GIT_DESCRIBE: {
            mod->firmware_tag = packet_.substr(ind+1);
            break;
        }
        case EIGEN_UTIL_MODULE_STATUS: {
            //TODO: Error checking!

            mod->t_sync = current_time_ms();
            auto tokens = stringtok(packet_.substr(ind+1), ",");
            mod->status_code = stoul(tokens[0], nullptr, EIGENBUS_BASE);
            mod->sync_ind = stoul(tokens[1], nullptr, EIGENBUS_BASE);
            mod->sync_reg = stoul(tokens[2], nullptr, EIGENBUS_BASE);
            mod->LED_code[MAX_LED_CODE_LEN] = stoul(tokens[3], nullptr, EIGENBUS_BASE);

            for(uint8_t i = 0; i < mod->LED_code[MAX_LED_CODE_LEN]; i++){
                mod->LED_code[i] = stoul(tokens[4+i], nullptr, EIGENBUS_BASE);
            }

            mod->module_status = tokens.back();
            break;
        }
        case EIGEN_UTIL_MODULE_CAPABILITY: {
            mod->command_support = stoul(packet_.substr(ind+1), nullptr, EIGENBUS_BASE);
            break;
        }
        case EIGEN_UTIL_MODULE_UID: {
            mod->update_UID(stoull(packet_.substr(ind+1), nullptr, EIGENBUS_BASE));
            break;
        }
        case EIGEN_UTIL_MODULE_PORTS: { 
            auto tokens = stringtok(packet_.substr(ind + 1), ",");

            uint8_t down_count = 0;
            for(auto port : tokens){
                if(port[0] == 'd'){
                    mod->set_downstream_name(down_count, port.substr(1));
                    down_count++;
                }
            }
            break;
        }
        default: {
            return false;
        }
            
    }

    return false;
}

module_update_enum EigenResponseUtility::update_type(){
    return MODULE_ERROR;
}
