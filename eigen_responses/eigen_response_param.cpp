#include "eigen_response_param.h"


EigenResponseParamRead::EigenResponseParamRead(std::string packet)
    : EigenResponse(packet) {

}

bool EigenResponseParamRead::update_module(ModuleShared mod){
    return false;
}

module_update_enum EigenResponseParamRead::type(){
    return MODULE_PARAM_READ;
}
