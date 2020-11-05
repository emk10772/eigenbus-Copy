#ifndef EIGEN_PACKET_PARSER_H
#define EIGEN_PACKET_PARSER_H

#include "eigen_utils.h"
#include "eigen_responses/eigen_response.h"
#include <map>

class EigenPacketParser{
public:
    EigenPacketParser();
    ~EigenPacketParser();

    EigenResponse *parse_packet(eigen_addr_t address, std::string packet);

private:
    void register_packet_type(std::string command, EigenResponse *(*parser)(eigen_addr_t address, std::string packet));
    std::map<std::string, EigenResponse *(*)(eigen_addr_t address, std::string)> response_map;
};

#endif