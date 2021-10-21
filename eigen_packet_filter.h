/* eigen_packet_filter.h
 *
 * This file is responsible for tracking and filtering packets. It is based on
 * the principle that we should always be able to predict what the robot will
 * send back to us. This is because Eigenbus-compliant modules will never send
 * anything unless instructed to by the host.
 *      Note: This doesn't necessarily mean that modules cannot broadcast
 *      information on their own. If the host instructs the modules to broadcast
 *      it can keep track of what rate it expects to receive messages at. This
 *      framework has not been completely implemented in firmware / software yet.
 *
 *  Every message that we send should have a defined response. This class works by
 *  watching every command and response that goes through the serial port. Every
 *  time we send a command we add an EigenPacketFilter to the packet filter queue.
 *  Every time we receive a response we check it against the filters to find a
 *  matching response. This enables packet filtering and statistics based on
 *  the context of the command / response.
 *
 *  The packet filter queue is sorted by time so that we can clear out timed out
 *  filters without searching the entire list.
 */

#ifndef EIGEN_PACKET_FILTER_H
#define EIGEN_PACKET_FILTER_H

#include "eigen_utils.h"
#include "eigen_responses/eigen_response.h"
#include <set>
#include <atomic>
#include <deque>

#define MAX_RAW_PACKETS     100

/* class EigenCounter
 *
 * This class is used to calculate an average rate over a period of time.
 * Used for calculating packet statistics. It counts single hits in a
 * specified window of time.
 */
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

/* class EigenPacketFilter
 *
 * This class is used to find responses that match a specific command. It
 * is initialized with an address, a filter, a type, and the raw string form
 * of the original packet.
 */
class EigenPacketFilter{
public:
    EigenPacketFilter(eigen_addr_t address, std::string response_filter,
                      packet_type packet, std::string packet_string);
    ~EigenPacketFilter();

private:
    eigen_addr_t address_;                          //Address of the command
    packet_type classification_;                    //Type of the command - used to filter out automatic poll commands
    std::deque<std::string> matched_responses_;     //Data structure of matched responses
    std::set<uint8_t> matched_addresses_;           //Set of matched addresses for fast inclusion checks
    uint64_t t_sent_;                               //When was this sent
    uint8_t retries_;                               //How many times have we resent this
    std::string packet_string_;                     //Raw command string for resending

public:
    std::string response_filter_;                   //The expected response filter

    bool expects_response();
    bool is_packet_timeout();
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

using response_match_t = std::pair<packet_type, uint64_t>;

class EigenPacketTracker{
public:
    EigenPacketTracker();
    ~EigenPacketTracker();

    raw_packet *get_raw_packet();
    void add_packet(eigen_addr_t addr, std::string filter, std::string packet, packet_type type);
    response_match_t match_response(ModuleShared module, EigenResponse *response, std::string raw_packet);
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
