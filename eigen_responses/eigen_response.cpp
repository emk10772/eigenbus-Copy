#include "eigen_response.h"

EigenResponse::EigenResponse(eigen_addr_t address, std::string packet, response_t message_type)
    :address_(address), packet_(packet), message_type_(message_type){

}

EigenResponse::~EigenResponse(){

}

EigenResponse::response_t EigenResponse::message_type(){
    return message_type_;
}

std::string EigenResponse::packet(){
    return packet_;
}

std::vector<std::string> EigenResponse::additional_responses(){
    return responses_;
}

bool EigenResponse::has_additonal_responses(){
    return responses_.size() > 0;
}

eigen_addr_t EigenResponse::address(){
    return address_;
}

bool EigenResponse::isSpontaneous(){
    return false;
}
