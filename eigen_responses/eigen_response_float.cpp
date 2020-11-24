#include "eigen_response_float.h"


EigenResponseFloat::EigenResponseFloat(eigen_addr_t address, std::string packet, response_t message_type)
    : EigenResponse(address, packet, message_type) {
    value = 0.0;
    value_valid = false;

    value = strtod(packet_.c_str(), nullptr);
    if(isfinite(value))
        value_valid = true;
}



/* Position Responses */
EigenResponsePosition::EigenResponsePosition(eigen_addr_t address, std::string packet)
    : EigenResponseFloat(address, packet, EigenResponse::EIGEN_POSITION){
}

EigenUpdate *EigenResponsePosition::update_module(ModuleShared mod){
    if(value_valid)
        mod->set_position(value);

    return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_POS_UPDATE, mod);
}




/* Velocity Responses */
EigenResponseVelocity::EigenResponseVelocity(eigen_addr_t address, std::string packet)
    : EigenResponseFloat(address, packet, EigenResponse::EIGEN_VELOCITY){
}

EigenUpdate *EigenResponseVelocity::update_module(ModuleShared mod){
    if(value_valid)
        mod->set_velocity(value);
    
    return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_VEL_UPDATE, mod);
}





/* Effort Responses */
EigenResponseEffort::EigenResponseEffort(eigen_addr_t address, std::string packet)
    : EigenResponseFloat(address, packet, EigenResponse::EIGEN_EFFORT){
}

EigenUpdate *EigenResponseEffort::update_module(ModuleShared mod){
    if(value_valid)
        mod->set_effort(value);

    return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_EFF_UPDATE, mod);
}
