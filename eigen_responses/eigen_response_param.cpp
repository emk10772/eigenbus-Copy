#include "eigen_response_param.h"


EigenResponseParamRead::EigenResponseParamRead(eigen_addr_t address, std::string packet)
    : EigenResponse(address, packet, EigenResponse::EIGEN_PARAM_READ) {
    
}

EigenUpdate *EigenResponseParamRead::update_module(ModuleShared mod){
       size_t ind = 0;
    id_ = stoul(packet_, &ind, EIGENBUS_BASE);

    if(ind != 2 || packet_[ind] != ',') return nullptr;

    if(id_ == 0){
        //Push the pointer along, process values
        auto tokens = stringtok(packet_.substr(ind+1), ",");
        uint8_t param_addr = stoul(tokens[0], nullptr, EIGENBUS_BASE);
        uint8_t param_aux = stoul(tokens[1], nullptr, EIGENBUS_BASE);

        //If we are receiving the number of parameters, fill in that we expect n parameters
        if(param_addr == 0){
            for(int i = 1; i < param_aux; i++){
                responses_.push_back(strprintf("|(00,%02X", i));
            }

            mod->set_expected_parameters(param_aux);
            return nullptr;
        } else {
            std::string param_name = tokens[2];
            //service_eigencomms should null terminate the string for us

            mod->add_parameter(param_addr, param_aux, param_name);

            return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_PARAM_ADD, param_addr);
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
        return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_PARAM_READ, id_);
    }
}


EigenResponseParamWrite::EigenResponseParamWrite(eigen_addr_t address, std::string packet)
    : EigenResponse(address, packet, EigenResponse::EIGEN_PARAM_WRITE) {

    
}

EigenUpdate *EigenResponseParamWrite::update_module(ModuleShared mod){
    size_t ind = 0;
    id_ = stoul(packet_, &ind, EIGENBUS_BASE);

    if(ind != 2 || packet_[ind] != ',')
        return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_PARAM_ERR, id_);;

    if(packet_[ind + 1] == 's'){
        return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_PARAM_WRITE, id_);
    } else {
        return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_PARAM_ERR, id_);
    }
}

