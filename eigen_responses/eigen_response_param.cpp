#include "eigen_response_param.h"


EigenResponseParamRead::EigenResponseParamRead(eigen_addr_t address, std::string packet)
    : EigenResponse(address, packet, EigenResponse::EIGEN_PARAM_READ) {
    
}

bool EigenResponseParamRead::update_module(ModuleShared mod){
       size_t ind = 0;
    id_ = stoul(packet_, &ind, EIGENBUS_BASE);

    if(ind != 2 || packet_[ind] != ',') return false;

    if(id_ == 0){
        //Push the pointer along, process values
        auto tokens = stringtok(packet_.substr(ind+1), ",");
        uint8_t param_addr = stoul(tokens[0], nullptr);
        uint8_t param_aux = stoul(tokens[1], nullptr);

        //If we are receiving the number of parameters, fill in that we expect n parameters
        if(param_addr == 0){
            for(int i = 1; i < param_aux; i++){
                responses_.push_back(strprintf("|(00,%02X", i));
            }

            mod->set_expected_parameters(param_aux);
            return false;
        } else {
            std::string param_name = tokens[2];
            //service_eigencomms should null terminate the string for us

            mod->add_parameter(param_addr, param_aux, param_name);

            update_ = MODULE_PARAM_ADD;
            return true;
        }
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

        //add_module_update(address, MODULE_PARAM_READ, id_);
        update_ = MODULE_PARAM_READ;
        return true;
    }
}

module_update_enum EigenResponseParamRead::update_type(){
    return update_;
}


EigenResponseParamWrite::EigenResponseParamWrite(eigen_addr_t address, std::string packet)
    : EigenResponse(address, packet, EigenResponse::EIGEN_PARAM_WRITE) {

    
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
