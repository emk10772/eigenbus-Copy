#include "eigen_response.h"

EigenResponse::EigenResponse(std::string packet, response_t message_type)
    :packet_(packet), message_type_(message_type){

}

EigenResponse::~EigenResponse(){

}

EigenResponse::response_t EigenResponse::message_type(){
    return message_type_;
}

std::string EigenResponse::packet(){
    return packet_;
}