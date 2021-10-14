#include "eigen_packet_filter.h"
#include "eigen_comms.h"

#define AVG_MAX_SIZE        50

#include <QDebug>

EigenPacketTracker::EigenPacketTracker(){
    last_packet_dropped = "";
    latency_avg_ = 0.0;
    latency_peak_ = 0.0;
    latency_total_ = 0;

    raw_packets_.setMax(MAX_RAW_PACKETS);
}

EigenPacketTracker::~EigenPacketTracker(){
    while(packet_filter_list.size() > 0){
        packet_filter_list.pop_front();
    }

    raw_packets_.clear();
}

raw_packet *EigenPacketTracker::get_raw_packet(){
    return raw_packets_.get();
}

void EigenPacketTracker::add_packet(eigen_addr_t addr, std::string filter, 
    std::string packet, packet_type type = EIGEN_PACKET_DEFAULT){
    
    //Add the filter to the filter list and the raw packet to the raw packet list if it is not empty
    packet_filter_list.push_back(EigenPacketFilter(addr, filter, type, packet));
    if(packet != "")
        add_raw_packet(packet, type, EIGEN_PACKET_SEND);

    //Track that we sent a packet
    packets_sent++;
}

void EigenPacketTracker::add_raw_packet(std::string pkt_string, packet_type type, uint8_t dir){
    auto config = get_eigen_config();

    if(config.raw_packet_en == EIGEN_DISABLED) return;

    //Add to raw packet list
    raw_packet *pkt = new raw_packet;
    pkt->packet = pkt_string;
    pkt->type = type;
    pkt->dir = dir;
    raw_packets_.add(pkt);

}

void EigenPacketTracker::handle_timeout_packets(){
    uint8_t max_retries = get_eigen_config().max_retries;

    //Newest packets are put into this queue from the back. Will be sorted in order of time because of this
    //Clear the timed out packets from the front of the queue
    auto it = packet_filter_list.begin();
    while(it != packet_filter_list.end() && it->packet_timeout()){
        if(it->is_broadcast() && it->num_responses() > 0){
            //Bar for success for a broadcast packet is pretty low. Just want to get at least one response
            successful_packets++;
        } else if (/*it->is_broadcast() ||*/ it->num_retries() >= max_retries){
            packets_dropped++;
            last_packet_dropped = it->packet_string();
        } else if(it->packet_string() != "") {
            retried_packets++;
            //Reset the timer, and increase the number of tries. Then resend the packet
            EigenPacketFilter packet = *it;
            packet.increment_retry_count();
            packet.reset_timeout();
            packet_filter_list.push_back(packet);

            packets_out.push_back(packet.packet_string());
        }

        packet_filter_list.pop_front();
        it = packet_filter_list.begin();
    }

    update_rate_calculations();
}

void EigenPacketTracker::handle_successful_packets(){
    auto it = packet_filter_list.begin();
    //Check for successful packets and remove them from the list
    while(it != packet_filter_list.end()){
        if(it->expects_response()){
            if(!it->is_broadcast() && it->num_responses() > 0) {
                successful_packets++;
                it = packet_filter_list.erase(it);
            } else {
                it++;
            }
        } else {
            it = packet_filter_list.erase(it);
            successful_packets++;
        }
    }

    update_rate_calculations();
}

response_match_t EigenPacketTracker::match_response(ModuleShared module, EigenResponse *response, std::string raw_packet){
    uint64_t t_now = current_time_ms();
    if(response != nullptr)
        t_now = response->t_received();

    //Search our filter list for a matching packet
    auto it = packet_filter_list.begin();
    while(response != nullptr && it != packet_filter_list.end() && !it->matches_filter(response->address(), raw_packet)){
        it++;
    }

    //If we found a matching filter, add it to that filter
    //First condition is technically redundant because it->matches_filter is always false for a null response
    //It is left for robustness and clarity
    if(response != nullptr && it != packet_filter_list.end() && it->matches_filter(response->address(), raw_packet)){
        it->add_response(response);
        add_raw_packet(raw_packet, it->get_type(), EIGEN_PACKET_RECV);

        if(!it->is_broadcast()){
            module->add_latency_measurement(t_now - it->t_sent());
        }
        add_latency(t_now - it->t_sent());

        return response_match_t(it->get_type(), t_now - it->t_sent());
    } else if(response == nullptr || !response->isSpontaneous()){
        add_raw_packet(raw_packet, EIGEN_PACKET_DEFAULT, EIGEN_PACKET_RECV);
        unrequested_packets++;

        //If we get an unrequested packet for a valid address there are a few possibilities:
        //1. Duplicate addresses
        //2. Garbled packet
        //3. Packets that the modules will send on their own (Such as uptime packets)
        //To be sure that we have no duplicate addresses, poll the UIDs for this particular address

        add_command(new EigenCommandUtility(0xFF, EIGEN_UTIL_MODULE_UID));
    } else {
        add_raw_packet(raw_packet, EIGEN_PACKET_POLL, EIGEN_PACKET_RECV);
        spontaneous_packets++;
    }

    return response_match_t(EIGEN_PACKET_NONE, 0);
}

double EigenPacketTracker::avg_latency(){
    return latency_avg_;
}

double EigenPacketTracker::peak_latency(){
    return latency_peak_;
}

void EigenPacketTracker::add_latency(uint64_t latency){
    latencies_.push_back(latency);
    latency_total_ += latency;

    while(latencies_.size() > AVG_MAX_SIZE){
        latency_total_ -= latencies_.front();
        latencies_.pop_front();
    }

    latency_avg_ = ((double) latency_total_) / ((double) latencies_.size());

    if((double) latency > latency_peak_){
        latency_peak_ = latency;
    }
}

void EigenPacketTracker::update_rate_calculations(){
    packets_sent.update_calculation();
    packets_dropped.update_calculation();
    successful_packets.update_calculation();
    unrequested_packets.update_calculation();
    spontaneous_packets.update_calculation();
    retried_packets.update_calculation();
}


/* Eigen Packet Class Definitions */
//TODO: What to do when we expect multiple responses to a packet, and do not know how many?
EigenPacketFilter::EigenPacketFilter(uint8_t address, std::string response_filter,
                                     packet_type packet, std::string packet_string){
    this->address_ = address;
    this->response_filter_ = response_filter;
    this->t_sent_ = current_time_ms();
    this->classification_ = packet;
    this->packet_string_ = packet_string;
    this->retries_ = 0;
}

EigenPacketFilter::~EigenPacketFilter(){
    this->matched_responses_.clear();
    this->matched_addresses_.clear();
}

bool EigenPacketFilter::expects_response(){
    return response_filter_ != "";
}

eigen_addr_t EigenPacketFilter::num_responses(){
    //Should not get more than size(eigen_addr_t) responses to a single request. If we do, there is something wrong
    return matched_responses_.size();
}

bool EigenPacketFilter::matches_filter(eigen_addr_t address, std::string packet){
    if(!is_broadcast() && address != address_) return false;

    if(matched_addresses_.count(address) == 0){
        if(packet.find(response_filter_) != std::string::npos){
            return true;
        }
    }
    return false;
}

void EigenPacketFilter::add_response(EigenResponse *response){
    if(response == nullptr) return;

    //Change to end of list for better efficiency?
    matched_responses_.push_back(response->packet());
    matched_addresses_.insert(response->address());
}

bool EigenPacketFilter::packet_timeout(){
    return (current_time_ms() - t_sent_) > PACKET_TIMEOUT;
}

bool EigenPacketFilter::is_broadcast(){
    return address_ == 0xFF;
}

packet_type EigenPacketFilter::get_type(){
    return this->classification_;
}

uint8_t EigenPacketFilter::num_retries(){
    return retries_;
}

void EigenPacketFilter::increment_retry_count(){
    retries_++;
}

void EigenPacketFilter::reset_timeout(){
    t_sent_ = current_time_ms();
}

std::string EigenPacketFilter::packet_string(){
    return this->packet_string_;
}

uint64_t EigenPacketFilter::t_sent(){
    return t_sent_;
}



EigenCounter::EigenCounter(uint64_t window_ms)
    : window_(window_ms){
    rate_ = 0;
    total_count_ = 0;
}

EigenCounter::~EigenCounter(){

}

uint64_t EigenCounter::total() const{
    return total_count_;
}

double EigenCounter::rate() const{
    return rate_;
}

void EigenCounter::update_calculation(){
    while(t_inc_.size() > 0 && current_time_ms() - t_inc_.front() > window_){
        t_inc_.pop_front();
    }

    if(window_ != 0){
        rate_ = 1000.0 * (double)t_inc_.size() / (double)window_;
    }
}

void EigenCounter::operator++(int) {
    t_inc_.push_back(current_time_ms());
    total_count_++;
}
