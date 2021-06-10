#include "eigen_command.h"
#include "../eigen_comms.h"

EigenCommand::EigenCommand(eigen_addr_t address, packet_type type, command_t command) 
    : address_(address), type_(type), command_(command){

}

EigenCommand::~EigenCommand(){
    
}

void EigenCommand::update_module(ModuleShared mod){
    
}

packet_type EigenCommand::type(){
    return type_;
}

eigen_addr_t EigenCommand::address(){
    return address_;
}

EigenCommand::command_t EigenCommand::command_type(){
    return command_;
}
