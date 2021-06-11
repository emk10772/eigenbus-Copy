#include "eigen_response_utility.h"
#include "../eigen_comms.h"

#include <string>


EigenResponseUtility::EigenResponseUtility(eigen_addr_t address, std::string packet)
    : EigenResponse(address, packet, EigenResponse::EIGEN_UTILITY) {
}

EigenUpdate *EigenResponseUtility::update_module(ModuleShared mod, uint64_t latency){
    size_t ind = 0;

    try{
        id_ = stoul(packet_, &ind, EIGENBUS_BASE);

        switch(id_){
            case EIGEN_UTIL_STAT_CODE: {
                uint8_t status_code = stoul(packet_.substr(ind+1), nullptr, EIGENBUS_BASE);
                if(status_code != mod->status_code){
                    //add_command(mod->get_address(), CMD_FIRMWARE_UTIL, EIGEN_UTIL_MODULE_STATUS);
                    add_command(new EigenCommandUtility(mod->address, EIGEN_UTIL_MODULE_STATUS));
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

                if(tokens.size() < 4) break;

                mod->status_code = stoul(tokens[0], nullptr, EIGENBUS_BASE);
                mod->sync_ind = stoul(tokens[1], nullptr, EIGENBUS_BASE);
                mod->sync_reg = stoul(tokens[2], nullptr, EIGENBUS_BASE);
                mod->LED_code[MAX_LED_CODE_LEN] = stoul(tokens[3], nullptr, EIGENBUS_BASE);

                if(mod->LED_code[MAX_LED_CODE_LEN] > tokens.size() - 5){
                    mod->LED_code[MAX_LED_CODE_LEN] = tokens.size() - 5;
                }

                for(uint8_t i = 0; i < mod->LED_code[MAX_LED_CODE_LEN]; i++){
                    mod->LED_code[i] = stoul(tokens[4+i], nullptr, EIGENBUS_BASE);
                }

                mod->module_status = tokens.back();
                break;
            }
            case EIGEN_UTIL_MODULE_CAPABILITY: {
                mod->command_support = (uint8_t)stoul(packet_.substr(ind+1), nullptr, EIGENBUS_BASE);
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
                        mod->set_downstream_name(down_count, port.substr(2));
                        down_count++;
                    }
                }
                break;
            }
            case EIGEN_UTIL_MODULE_NAME: {
                mod->module_name = packet_.substr(ind+1);
                break;
            }
            default: {
                return nullptr;
            }

        }
    } catch(std::exception e){
        return nullptr;
    }

    return nullptr;
}
