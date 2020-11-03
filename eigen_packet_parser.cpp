#include "eigen_packet_parser.h"
#include "eigen_comms.h"
#include "eigen_responses/eigen_response_utility.h"
#include "eigen_responses/eigen_response_float.h"
#include "eigen_responses/eigen_response_topology.h"
#include "eigen_responses/eigen_response_param.h"

template <class Response>
EigenResponse *new_response(std::string packet){
    return new Response(packet);
}

EigenPacketParser::EigenPacketParser(){
    register_packet_type("U", &new_response<EigenResponseUtility>);
    register_packet_type("S", &new_response<EigenResponseTopology>);
    register_packet_type("L", &new_response<EigenResponsePosition>);
    register_packet_type("V", &new_response<EigenResponseVelocity>);
    register_packet_type("I", &new_response<EigenResponseEffort>);
    register_packet_type("|(", &new_response<EigenResponseParamRead>);
    register_packet_type("|)", &new_response<EigenResponseParamWrite>);
}

EigenPacketParser::~EigenPacketParser(){
}

void EigenPacketParser::register_packet_type(std::string command,  
    EigenResponse *(*parser)(std::string packet)){

    response_map[command] = parser;
}

//parse_packet: takes a command packet trimmed of the address
EigenResponse *EigenPacketParser::parse_packet(std::string packet){
    //If the first character is a hex digit, this packet is not a valid response
    if(isxdigit(packet[0])) return nullptr;

    //Check the second character to see if this is a "dual character" response such as |(
    std::string key;
    uint8_t key_size = 0;
    key_size = (isxdigit(packet[1]) ? 1 : 2);
    key = packet.substr(0, key_size + 1);
    
    if(response_map.count(key)){
        return (response_map[key])(packet.substr(key_size));
    }
    
    return nullptr;
}