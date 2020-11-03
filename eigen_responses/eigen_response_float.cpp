#include "eigen_response_float.h"


EigenResponseFloat::EigenResponseFloat(std::string packet)
    : EigenResponse(packet) {
    value = 0.0;
    value_valid = false;

    value = strtod(packet_.c_str(), nullptr);
    if(isfinite(value))
        value_valid = true;
}



/* Position Responses */

bool EigenResponsePosition::update_module(ModuleShared mod){
    if(value_valid)
        mod->set_position(value);
    
    return value_valid;
}

module_update_enum EigenResponsePosition::type(){
    return MODULE_POS_UPDATE;
}




/* Velocity Responses */

bool EigenResponseVelocity::update_module(ModuleShared mod){
    if(value_valid)
        mod->set_velocity(value);
    
    return value_valid;
}

module_update_enum EigenResponseVelocity::type(){
    return MODULE_VEL_UPDATE;
}




/* Effort Responses */

bool EigenResponseEffort::update_module(ModuleShared mod){
    if(value_valid)
        mod->set_effort(value);
    
    return value_valid;
}

module_update_enum EigenResponseEffort::type(){
    return MODULE_VEL_UPDATE;
}
