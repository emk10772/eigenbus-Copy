#include "eigen_update.h"

EigenUpdate::EigenUpdate(eigen_addr_t address, update_t type, ModuleConst mod, uint8_t arg, std::string data)
    : address_(address), type_(type), mod_(mod), arg_(arg), data_(data) {

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
