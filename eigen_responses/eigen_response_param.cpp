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

            mod->parameters.set_expected_parameters(param_aux);
            return nullptr;
        } else {
            std::string param_name = tokens[2];
            //service_eigencomms should null terminate the string for us

            mod->parameters.add(param_addr, EigenParameter(param_aux), param_name);

            return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_PARAM_ADD, mod, param_addr);
        }
    } else {
        //Write value to module
        mod->parameters.ref(id_).update_value(packet_.substr(ind + 1));
        return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_PARAM_READ, mod, id_);
    }
}


EigenResponseParamWrite::EigenResponseParamWrite(eigen_addr_t address, std::string packet)
    : EigenResponse(address, packet, EigenResponse::EIGEN_PARAM_WRITE) {

    
}

EigenUpdate *EigenResponseParamWrite::update_module(ModuleShared mod){
    size_t ind = 0;
    id_ = stoul(packet_, &ind, EIGENBUS_BASE);

    if(ind != 2 || packet_[ind] != ',')
        return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_PARAM_ERR, mod, id_);;

    if(packet_[ind + 1] == 's'){
        return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_PARAM_WRITE, mod, id_);
    } else {
        return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_PARAM_ERR, mod, id_);
    }
}

