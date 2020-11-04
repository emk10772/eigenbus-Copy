#include "eigen_response_float.h"


EigenResponseFloat::EigenResponseFloat(std::string packet, response_t message_type)
    : EigenResponse(packet, message_type) {
    value = 0.0;
    value_valid = false;

    value = strtod(packet_.c_str(), nullptr);
    if(isfinite(value))
        value_valid = true;
}



/* Position Responses */
EigenResponsePosition::EigenResponsePosition(std::string packet)
    : EigenResponseFloat(packet, EigenResponse::EIGEN_POSITION){
}

bool EigenResponsePosition::update_module(ModuleShared mod){
    if(value_valid)
        mod->set_position(value);
    
    return value_valid;
}

module_update_enum EigenResponsePosition::update_type(){
    return MODULE_POS_UPDATE;
}




/* Velocity Responses */
EigenResponseVelocity::EigenResponseVelocity(std::string packet)
    : EigenResponseFloat(packet, EigenResponse::EIGEN_VELOCITY){
}

bool EigenResponseVelocity::update_module(ModuleShared mod){
    if(value_valid)
        mod->set_velocity(value);
    
    return value_valid;
}

module_update_enum EigenResponseVelocity::update_type(){
    return MODULE_VEL_UPDATE;
}




/* Effort Responses */
EigenResponseEffort::EigenResponseEffort(std::string packet)
    : EigenResponseFloat(packet, EigenResponse::EIGEN_EFFORT){
}

bool EigenResponseEffort::update_module(ModuleShared mod){
    if(value_valid)
        mod->set_effort(value);
    
    return value_valid;
}

module_update_enum EigenResponseEffort::update_type(){
    return MODULE_VEL_UPDATE;
}
