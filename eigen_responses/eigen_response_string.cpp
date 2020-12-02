#include "eigen_response_string.h"


EigenResponseString::EigenResponseString(eigen_addr_t address, std::string packet, response_t message_type)
    : EigenResponse(address, packet, message_type) {
}



/* Position Responses */
EigenResponseEcho::EigenResponseEcho(eigen_addr_t address, std::string packet)
    : EigenResponseString(address, packet, EigenResponse::EIGEN_ECHO){
}


EigenUpdate *EigenResponseEcho::update_module(ModuleShared mod){
    return nullptr;
}
