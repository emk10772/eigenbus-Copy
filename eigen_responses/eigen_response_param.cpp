#include "eigen_response_param.h"


EigenResponseParamRead::EigenResponseParamRead(std::string packet)
    : EigenResponse(packet, EigenResponse::EIGEN_PARAM_READ) {
    
}

bool EigenResponseParamRead::update_module(ModuleShared mod){
       size_t ind = 0;
    id_ = stoul(packet_, &ind, EIGENBUS_BASE);

    if(ind != 2 || packet_[ind] != ',') return;

    if(id_ == 0){
        //Push the pointer along, process values
        auto tokens = stringtok(packet_.substr(ind+1), ",");
        uint8_t param_addr = strtol((char *)(++ptr), (char**)&ptr, 16);
        uint8_t param_aux = strtol((char *)(++ptr), (char**)&ptr, 16);

        //If we are receiving the number of parameters, fill in that we expect n parameters
        if(param_addr == 0){
            //TODO: Figure out how to do this with new style
            /*
            for(int i = 1; i < param_aux; i++){
                uint8_t s[32];
                std::snprintf((char *)s, 32, "|(00,%02X", i);
                add_packet(addr, "", std::string((char *)s), EIGEN_PACKET_DEBUG);
            }*/
            for(int i = 1; i < param_aux; i++){
                responses_.push_back(strprintf("|(00,%02X", i));
            }

            mod->set_expected_parameters(param_aux);
        }

        ++ptr;

        std::string param_name = std::string((char *)ptr);
        //service_eigencomms should null terminate the string for us

        module->add_parameter(param_addr, param_aux, param_name);

        add_module_update(addr, MODULE_PARAM_ADD, param_addr);
    } else {
        //TODO: Error checking
        uint8_t type = mod->parameter_type(id_);
        uint64_t value = 0;
        if(type == _FLOAT || type == _DOUBLE){
            double val = stod(packet_.substr(ind+1), NULL);
            memcpy(&value, &val, sizeof(val));
        } else {
            value = stoull(packet_.substr(ind+1), NULL, EIGENBUS_BASE);
        }
        //Write value to module
        mod->update_parameter(id_, value);

        add_module_update(address, MODULE_PARAM_READ, id_);
        return true;
    }
}

module_update_enum EigenResponseParamRead::update_type(){
    return MODULE_PARAM_READ;
}


EigenResponseParamWrite::EigenResponseParamWrite(std::string packet)
    : EigenResponse(packet, EigenResponse::EIGEN_PARAM_WRITE) {

    
}

bool EigenResponseParamWrite::update_module(ModuleShared mod){
    return true;
}

module_update_enum EigenResponseParamWrite::update_type(){
    size_t ind = 0;
    id_ = stoul(packet_, &ind, EIGENBUS_BASE);

    if(ind != 2 || packet_[ind] != ',') return MODULE_PARAM_ERR;
    
    if(packet_[ind + 1] == 's'){
        return MODULE_PARAM_WRITE;
    } else {
        return MODULE_PARAM_ERR;
    }
}
