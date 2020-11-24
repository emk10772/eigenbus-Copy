#include "eigen_packet_parser.h"
#include "eigen_comms.h"
#include "eigen_responses/eigen_response_utility.h"
#include "eigen_responses/eigen_response_float.h"
#include "eigen_responses/eigen_response_topology.h"
#include "eigen_responses/eigen_response_param.h"
#include "eigen_responses/eigen_response_bootloader.h"
#include "eigen_responses/eigen_response_mailbox.h"
#include "eigen_responses/eigen_response_uptime.h"

template <class Response>
EigenResponse *new_response(eigen_addr_t address, std::string packet){
    return new Response(address, packet);
}

EigenPacketParser::EigenPacketParser(){
    register_packet_type("U", &new_response<EigenResponseUtility>);
    register_packet_type("S", &new_response<EigenResponseTopology>);
    register_packet_type("L", &new_response<EigenResponsePosition>);
    register_packet_type("V", &new_response<EigenResponseVelocity>);
    register_packet_type("I", &new_response<EigenResponseEffort>);
    register_packet_type("|(", &new_response<EigenResponseParamRead>);
    register_packet_type("|)", &new_response<EigenResponseParamWrite>);
    register_packet_type("~", &new_response<EigenResponseBootloader>);
    register_packet_type("|[", &new_response<EigenResponseMailboxRead>);
    register_packet_type("|]", &new_response<EigenResponseMailboxWrite>);
    register_packet_type("T", &new_response<EigenResponseUptime>);
}

EigenPacketParser::~EigenPacketParser(){
}

void EigenPacketParser::register_packet_type(std::string command,  
    EigenResponse *(*parser)(eigen_addr_t address, std::string packet)){

    response_map[command] = parser;
}

//parse_packet: takes a command packet trimmed of the address
EigenResponse *EigenPacketParser::parse_packet(eigen_addr_t address, std::string packet){
    //If the first character is a hex digit, this packet is not a valid response
    if(isxdigit((unsigned char)packet[0])) return nullptr;

    //Check the second character to see if this is a "dual character" response such as |(
    std::string key;
    uint8_t key_size = 0;
    key_size = (isxdigit((unsigned char)packet[1]) ? 1 : 2);
    
    while(key_size > 0){
        key = packet.substr(0, key_size);

        if(response_map.count(key))
            return (response_map[key])(address, packet.substr(key_size));

        key_size--;
    }
    
    return nullptr;
}
