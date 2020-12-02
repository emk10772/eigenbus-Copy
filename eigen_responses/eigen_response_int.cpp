#include "eigen_response_int.h"


EigenResponseInt::EigenResponseInt(eigen_addr_t address, std::string packet, response_t message_type)
    : EigenResponse(address, packet, message_type) {
    value = strtoll(packet_.c_str(), nullptr, EIGENBUS_BASE);
}

EigenResponseUint::EigenResponseUint(eigen_addr_t address, std::string packet, response_t message_type)
    : EigenResponse(address, packet, message_type) {
    value = strtoull(packet_.c_str(), nullptr, EIGENBUS_BASE);
}



/* Position Responses */
EigenResponseEncoderStatus::EigenResponseEncoderStatus(eigen_addr_t address, std::string packet)
    : EigenResponseUint(address, packet, EigenResponse::EIGEN_ENCODER_STAT){
}

EigenUpdate *EigenResponseEncoderStatus::update_module(ModuleShared mod){
    mod->set_encoder_status(value);
    return new EigenUpdate(address_, EigenUpdate::MODULE_ENCODER_STATUS, mod);
}
