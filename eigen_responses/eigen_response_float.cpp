#include "eigen_response_float.h"


EigenResponseFloat::EigenResponseFloat(eigen_addr_t address, std::string packet, response_t message_type)
    : EigenResponse(address, packet, message_type) {
    value = 0.0;
    value_valid = false;

    try{
        value = strtod(packet_.c_str(), nullptr);
        if(std::isfinite(value))
            value_valid = true;
    } catch (std::exception e) {
        value_valid = false;
    }
}



/* Position Responses */
EigenResponsePosition::EigenResponsePosition(eigen_addr_t address, std::string packet)
    : EigenResponseFloat(address, packet, EigenResponse::EIGEN_POSITION){
}

EigenUpdate *EigenResponsePosition::update_module(ModuleShared mod, uint64_t latency){
    if(value_valid)
        mod->set_position(value);

    return new EigenUpdate(mod->address(), EigenUpdate::MODULE_POS_UPDATE, latency, mod);
}




/* Velocity Responses */
EigenResponseVelocity::EigenResponseVelocity(eigen_addr_t address, std::string packet)
    : EigenResponseFloat(address, packet, EigenResponse::EIGEN_VELOCITY){
}

EigenUpdate *EigenResponseVelocity::update_module(ModuleShared mod, uint64_t latency){
    if(value_valid)
        mod->set_velocity(value);
    
    return new EigenUpdate(mod->address(), EigenUpdate::MODULE_VEL_UPDATE, latency, mod);
}





/* Effort Responses */
EigenResponseEffort::EigenResponseEffort(eigen_addr_t address, std::string packet)
    : EigenResponseFloat(address, packet, EigenResponse::EIGEN_EFFORT){
}

EigenUpdate *EigenResponseEffort::update_module(ModuleShared mod, uint64_t latency){
    if(value_valid)
        mod->set_effort(value);

    return new EigenUpdate(mod->address(), EigenUpdate::MODULE_EFF_UPDATE, latency, mod);
}
