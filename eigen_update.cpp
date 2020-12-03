#include "eigen_update.h"

EigenUpdate::EigenUpdate(eigen_addr_t address, update_t type, uint64_t latency, ModuleConst mod, uint8_t arg, std::string data)
    : address_(address), type_(type), mod_(mod), arg_(arg), data_(data), t_received_(current_time_ms()), latency_(latency) {

}

EigenUpdate::EigenUpdate(eigen_addr_t address, update_t type, ModuleConst mod, uint8_t arg, std::string data)
    : address_(address), type_(type), mod_(mod), arg_(arg), data_(data), t_received_(current_time_ms()), latency_(0) {

}

EigenUpdate::~EigenUpdate(){

}

EigenUpdate::update_t EigenUpdate::type(){
    return type_;
}

eigen_addr_t EigenUpdate::address(){
    return address_;
}

uint8_t EigenUpdate::arg(){
    return arg_;
}

std::string EigenUpdate::data(){
    return data_;
}

ModuleConst EigenUpdate::module(){
    return mod_;
}

uint64_t EigenUpdate::t_received(){
    return t_received_;
}

uint64_t EigenUpdate::t_adjusted(){
    return t_received_ - (latency_/2);
}
