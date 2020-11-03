#include "eigen_packet_parser.h"
#include "eigen_comms.h"
#include "eigen_responses/eigen_response_utility.h"
#include "eigen_responses/eigen_response_float.h"
#include "eigen_responses/eigen_response_topology.h"

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
}

EigenPacketParser::~EigenPacketParser(){
}

void EigenPacketParser::register_packet_type(std::string command,  
    EigenResponse *(*parser)(std::string packet)){

    response_map[command] = parser;
}

EigenResponse *EigenPacketParser::parse_packet(std::string packet){
    EigenResponse *test = (response_map["test"])(packet);
    return test;
}