#include "eigen_packet_poll.h"

EigenPacketPoll::EigenPacketPoll(){

}

EigenPacketPoll::~EigenPacketPoll(){
    for(auto item : poll_map_){
        delete item.second->command;
        delete item.second;
    }

    poll_map_.clear();
}

void EigenPacketPoll::service_poll(){
    for(auto item : poll_map_){
        uint64_t t_now = current_time_ms();
        if(t_now - item.second->t_last >= item.second->period_ms && item.second->enabled){
            item.second->t_last = t_now;
            commands.add(item.second->command->clone());
        }
    }
}

EigenCommand *EigenPacketPoll::get_command(){
    return commands.get();
}

std::string EigenPacketPoll::add_command(EigenCommand *command, uint64_t period_ms, bool enabled){
    std::lock_guard<std::mutex> lock(poll_mutex_);

    if(command == nullptr) return "";

    std::string key = command->packet();

    if(poll_map_.count(key) > 0){
        delete poll_map_.at(key)->command;
        delete poll_map_.at(key);
        poll_map_.erase(key);
    }

    poll_t *value = new poll_t;
    value->t_last = current_time_ms();
    value->command = command;
    value->period_ms = period_ms;
    value->enabled = enabled;

    poll_map_.insert(std::pair<std::string, poll_t *>(key, value));
    return key;
}

void EigenPacketPoll::set_enable(std::string key, bool enabled){
    std::lock_guard<std::mutex> lock(poll_mutex_);

    if(poll_map_.count(key) > 0){
        poll_map_.at(key)->enabled = enabled;
    }
}

void EigenPacketPoll::remove_command(std::string key){
    std::lock_guard<std::mutex> lock(poll_mutex_);

    if(poll_map_.count(key) > 0){
        delete poll_map_.at(key)->command;
        delete poll_map_.at(key);
        poll_map_.erase(key);
    }
}
