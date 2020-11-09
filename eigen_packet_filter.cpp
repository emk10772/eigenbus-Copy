#include "eigen_packet_filter.h"
#include "eigen_comms.h"

EigenPacketTracker::EigenPacketTracker(){
    packets_sent = 0;
    packets_dropped = 0;
    successful_packets = 0;
    unrequested_packets = 0;
    retried_packets = 0;
    last_packet_dropped = "";
}

EigenPacketTracker::~EigenPacketTracker(){
    while(packet_filter_list.size() > 0){
        packet_filter_list.pop_front();
    }

    while(raw_packet_list.size() > 0){
        auto packet = raw_packet_list.front();
        raw_packet_list.pop_front();
        delete packet;
    }
}

raw_packet *EigenPacketTracker::get_raw_packet(){
    raw_packet *retval;
    std::lock_guard<std::mutex> lock(raw_packet_mutex);

    if(raw_packet_list.size() > 0){
        retval = raw_packet_list.front();
        raw_packet_list.pop_front();
    } else {
        return NULL;
    }

    return retval;
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

    std::lock_guard<std::mutex> lock(raw_packet_mutex);

    //Add to raw packet list
    raw_packet *pkt = new raw_packet;
    pkt->packet = pkt_string;
    pkt->type = type;
    pkt->dir = dir;
    raw_packet_list.push_back(pkt);
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
        } else if (it->is_broadcast() || it->num_retries() >= max_retries){
            packets_dropped++;
            last_packet_dropped = it->packet_string();
        } else {
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
}

packet_type EigenPacketTracker::match_response(uint8_t address, std::string packet){
    //Search our filter list for a matching packet
    auto it = packet_filter_list.begin();
    while(it != packet_filter_list.end() && !it->matches_filter(address, packet)){
        it++;
    }

    //If we found a matching filter, add it to that filter
    if(it != packet_filter_list.end() && it->matches_filter(address, packet)){
        it->add_response(address, packet);
        add_raw_packet(packet, it->get_type(), EIGEN_PACKET_RECV);
        return it->get_type();
    } else {
        add_raw_packet(packet, EIGEN_PACKET_DEFAULT, EIGEN_PACKET_RECV);
        unrequested_packets++;

        //If we get an unrequested packet for a valid address there are a few possibilities:
        //1. Duplicate addresses
        //2. Garbled packet
        //To be sure that we have no duplicate addresses, poll the UIDs for this particular address

        add_command(new EigenCommandUtility(0xFF, EIGEN_UTIL_MODULE_UID));
    }

    return EIGEN_PACKET_NONE;
}

/* Eigen Packet Class Definitions */
//TODO: What to do when we expect multiple responses to a packet, and do not know how many?
EigenPacketFilter::EigenPacketFilter(uint8_t address, std::string response_filter,
                                     packet_type packet, std::string packet_string){
    this->address = address;
    this->response_filter = response_filter;
    this->t_sent = current_time_ms();
    this->classification = packet;
    this->packet_string_ = packet_string;
    this->retries = 0;
}

EigenPacketFilter::~EigenPacketFilter(){
    this->matched_responses.clear();
    this->matched_addresses.clear();
}

bool EigenPacketFilter::expects_response(){
    return response_filter != "";
}

uint8_t EigenPacketFilter::num_responses(){
    //Should not get more than 255 responses to a single request. If we do, there is something wrong
    return matched_responses.size();
}

bool EigenPacketFilter::matches_filter(uint8_t address, std::string packet){
    if(!is_broadcast() && address != this->address) return false;
    if(matched_addresses.count(address) == 0){
        if(packet.find(response_filter) != std::string::npos){
            return true;
        }
    }
    return false;
}

void EigenPacketFilter::add_response(uint8_t address, std::string response){
    //Change to end of list for better efficiency?
    matched_responses.insert(matched_responses.begin(), response);
    matched_addresses.insert(address);
}

bool EigenPacketFilter::packet_timeout(){
    return (current_time_ms() - t_sent) > PACKET_TIMEOUT;
}

bool EigenPacketFilter::is_broadcast(){
    return address == 0xFF;
}

packet_type EigenPacketFilter::get_type(){
    return this->classification;
}

uint8_t EigenPacketFilter::num_retries(){
    return retries;
}

void EigenPacketFilter::increment_retry_count(){
    retries++;
}

void EigenPacketFilter::reset_timeout(){
    t_sent = current_time_ms();
}

std::string EigenPacketFilter::packet_string(){
    return this->packet_string_;
}
