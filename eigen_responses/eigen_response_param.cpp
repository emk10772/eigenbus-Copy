#include "eigen_response_param.h"


EigenResponseParamRead::EigenResponseParamRead(std::string packet)
    : EigenResponse(packet, EigenResponse::EIGEN_PARAM_READ) {

}

bool EigenResponseParamRead::update_module(ModuleShared mod){
    return false;
}

module_update_enum EigenResponseParamRead::update_type(){
    return MODULE_PARAM_READ;
}
