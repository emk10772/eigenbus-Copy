#ifndef EIGEN_PACKET_FILTER_H
#define EIGEN_PACKET_FILTER_H

#include "eigen_utils.h"
#include "eigen_responses/eigen_response.h"
#include <set>
#include <atomic>
#include <deque>

class EigenCounter {
public:
    EigenCounter(uint64_t window_ms = 2000);
    ~EigenCounter();

    //Thread safe
    uint64_t total() const;
    double rate() const;

    //Call in same thread
    void update_calculation();
    void operator++(int);

private:
    std::atomic<double> rate_;
    std::atomic<uint64_t> total_count_;
    std::deque<uint64_t> t_inc_;
    const uint64_t window_;
};

class EigenPacketFilter{
public:
    EigenPacketFilter(eigen_addr_t address, std::string response_filter, packet_type packet, std::string packet_string);
    ~EigenPacketFilter();

private:
    eigen_addr_t address_;
    packet_type classification_;
    std::deque<std::string> matched_responses_;
    std::set<uint8_t> matched_addresses_;
    uint64_t t_sent_;
    uint8_t retries_;
    std::string packet_string_;

public:
    std::string response_filter_;
    bool expects_response();
    bool packet_timeout();
    eigen_addr_t num_responses();
    bool matches_filter(eigen_addr_t address, std::string packet);
    bool is_broadcast();
    void add_response(EigenResponse *response);
    packet_type get_type();
    uint8_t num_retries();
    void increment_retry_count();
    void reset_timeout();
    std::string packet_string();
    uint64_t t_sent();
};

class EigenPacketTracker{
public:
    EigenPacketTracker();
    ~EigenPacketTracker();

    raw_packet *get_raw_packet();
    void add_packet(eigen_addr_t addr, std::string filter, std::string packet, packet_type type);
    packet_type match_response(ModuleShared module, EigenResponse *response, std::string raw_packet);
    bool UID_scan_required();
    void handle_timeout_packets();
    void handle_successful_packets();
    double avg_latency();
    double peak_latency();

    EigenCounter packets_sent;
    EigenCounter packets_dropped;
    EigenCounter successful_packets;
    EigenCounter unrequested_packets;
    EigenCounter spontaneous_packets;
    EigenCounter retried_packets;
    std::string last_packet_dropped;
    std::deque<std::string> packets_out;

private:
    void add_raw_packet(std::string pkt_string, packet_type type, uint8_t dir);
    void add_latency(uint64_t latency);
    void update_rate_calculations();

    //Raw packet storage, thread safe
    EigenQueue<raw_packet> raw_packets_;
    
    //Internal deques, not thread safe
    std::deque<EigenPacketFilter> packet_filter_list;
    std::deque<uint64_t> latencies_;
    uint64_t latency_total_;
    std::atomic<double> latency_avg_;
    std::atomic<double> latency_peak_;

    bool requires_UID_scan;
};

#endif
