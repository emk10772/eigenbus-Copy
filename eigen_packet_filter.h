#ifndef EIGEN_PACKET_FILTER_H
#define EIGEN_PACKET_FILTER_H

#include "eigen_utils.h"
#include "eigen_responses/eigen_response.h"
#include <set>
#include <deque>

class EigenPacketFilter{
public:
    EigenPacketFilter(eigen_addr_t address, std::string response_filter, packet_type packet, std::string packet_string);
    ~EigenPacketFilter();

private:
    eigen_addr_t address;
    packet_type classification;
    std::string response_filter;
    std::vector<std::string> matched_responses;
    std::set<uint8_t> matched_addresses;
    uint64_t t_sent;
    uint8_t retries;
    std::string packet_string_;

public:
    bool expects_response();
    bool packet_timeout();
    uint8_t num_responses();
    bool matches_filter(uint8_t address, std::string packet);
    bool is_broadcast();
    void add_response(uint8_t address, std::string response);
    packet_type get_type();
    uint8_t num_retries();
    void increment_retry_count();
    void reset_timeout();
    std::string packet_string();
};

class EigenPacketTracker{
public:
    EigenPacketTracker();
    ~EigenPacketTracker();

    raw_packet *get_raw_packet();
    void add_packet(eigen_addr_t addr, std::string filter, std::string packet, packet_type type);
    packet_type match_response(uint8_t address, std::string packet, bool spontaneous = false);
    bool UID_scan_required();
    void handle_timeout_packets();
    void handle_successful_packets();

    uint64_t packets_sent;
    uint64_t packets_dropped;
    uint64_t successful_packets;
    uint64_t unrequested_packets;
    uint64_t spontaneous_packets;
    uint64_t retried_packets;
    std::string last_packet_dropped;
    std::deque<std::string> packets_out;

private:
    void add_raw_packet(std::string pkt_string, packet_type type, uint8_t dir);

    //Raw packet storage, thread safe
    std::deque<raw_packet *> raw_packet_list;
    std::mutex raw_packet_mutex;
    
    //Internal deque, not thread safe
    std::deque<EigenPacketFilter> packet_filter_list;

    bool requires_UID_scan;
};

#endif
