#include "eigen_command_float.h"

EigenCommandFloat::EigenCommandFloat(eigen_addr_t address, std::string command_char, std::string response, double value, command_t command_type)
    : EigenCommand(address, EIGEN_PACKET_DEFAULT, command_type), command_char_(command_char), response_(response), value_(value){

}

std::string EigenCommandFloat::packet() const{
    return strprintf("%02x%s%08.4f", address_, command_char_.c_str(), value_);
}

std::string EigenCommandFloat::expected_response() const{
    return response_;
}


EigenCommandPosition::EigenCommandPosition(eigen_addr_t address, double value)
    : EigenCommandFloat(address, "P", "", value, EIGEN_CMD_POSITION){
}
void EigenCommandPosition::update_module(ModuleShared mod){
    mod->last_position_cmd = value_;
}
EigenCommand *EigenCommandPosition::clone() const{
    return new EigenCommandPosition(*this);
}
EigenCommand *EigenCommandPosition::clone(eigen_addr_t addr) const{
    return new EigenCommandPosition(addr, value_);
}


EigenCommandVelocity::EigenCommandVelocity(eigen_addr_t address, double value)
    : EigenCommandFloat(address, "S", "", value, EIGEN_CMD_VELOCITY){
}
void EigenCommandVelocity::update_module(ModuleShared mod){
    mod->last_velocity_cmd = value_;
}
EigenCommand *EigenCommandVelocity::clone() const{
    return new EigenCommandVelocity(*this);
}
EigenCommand *EigenCommandVelocity::clone(eigen_addr_t addr) const{
    return new EigenCommandVelocity(addr, value_);
}


EigenCommandEffort::EigenCommandEffort(eigen_addr_t address, double value)
    : EigenCommandFloat(address, "T", "", value, EIGEN_CMD_EFFORT){
}
void EigenCommandEffort::update_module(ModuleShared mod){
    mod->last_effort_cmd = value_;
}
EigenCommand *EigenCommandEffort::clone() const{
    return new EigenCommandEffort(*this);
}
EigenCommand *EigenCommandEffort::clone(eigen_addr_t addr) const{
    return new EigenCommandEffort(addr, value_);
}

