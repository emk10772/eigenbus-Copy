#include "eigen_response_param.h"


EigenResponseParamRead::EigenResponseParamRead(eigen_addr_t address, std::string packet)
    : EigenResponse(address, packet, EigenResponse::EIGEN_PARAM_READ) {
    
}

EigenUpdate *EigenResponseParamRead::update_module(ModuleShared mod, uint64_t latency){
    try{
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

                mod->parameters.set_expected_parameters(param_aux - 1);
                return nullptr;
            } else {
                std::string param_name = "ERR";
                if(tokens.size() > 2)
                    param_name = tokens[2];
                //service_eigencomms should null terminate the string for us
                EigenVariable *variable = nullptr;
                if(param_aux == PARAM_TYPE_UINT8){
                    variable = new EigenUint8(param_name);
                } else if(param_aux == PARAM_TYPE_UINT16){
                    variable = new EigenUint16(param_name);
                } else if(param_aux == PARAM_TYPE_UINT32){
                    variable = new EigenUint32(param_name);
                } else if(param_aux == PARAM_TYPE_UINT64){
                    variable = new EigenUint64(param_name);
                } else if(param_aux == PARAM_TYPE_DOUBLE || param_aux == PARAM_TYPE_FLOAT){
                    variable = new EigenDouble(param_name);
                }

                if(variable) {
                    mod->parameters.add(param_addr, variable, param_name);
                    variable->set_id(param_addr);
                    variable->set_address(mod->address);
                    return new EigenUpdate(mod->address, EigenUpdate::MODULE_PARAM_ADD, latency, mod, param_addr);
                }
                return nullptr;
            }
        } else {
            //Write value to module
            EigenVariable *variable = mod->parameters.value(id_);
            bool valid = false;
            if(variable)
                valid = variable->parse_value(packet_.substr(ind + 1));
            if(valid)
                return new EigenUpdate(mod->address, EigenUpdate::MODULE_PARAM_READ, latency, mod, id_, mod->parameters.name(id_));
        }
    } catch (std::exception e){
        return new EigenUpdate(mod->address, EigenUpdate::MODULE_PARAM_ERR, latency, mod);
    }

    return nullptr;
}


EigenResponseParamWrite::EigenResponseParamWrite(eigen_addr_t address, std::string packet)
    : EigenResponse(address, packet, EigenResponse::EIGEN_PARAM_WRITE) {

    
}

EigenUpdate *EigenResponseParamWrite::update_module(ModuleShared mod, uint64_t latency){
    size_t ind = 0;
    try {
        id_ = stoul(packet_, &ind, EIGENBUS_BASE);
    } catch (std::exception e) {
        return new EigenUpdate(mod->address, EigenUpdate::MODULE_PARAM_ERR, latency, mod);
    }

    if(ind != 2 || packet_[ind] != ',')
        return new EigenUpdate(mod->address, EigenUpdate::MODULE_PARAM_ERR, latency, mod, id_);
    if(packet_[ind + 1] == 's'){
        return new EigenUpdate(mod->address, EigenUpdate::MODULE_PARAM_WRITE, latency, mod, id_);
    } else {
        return new EigenUpdate(mod->address, EigenUpdate::MODULE_PARAM_ERR, latency, mod, id_);
    }
}

