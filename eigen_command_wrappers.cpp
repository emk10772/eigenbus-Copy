#include "eigen_command_wrappers.h"
#include "eigen_comms.h"


/* Locomotion commands */
void eigen_set_position(eigen_addr_t address, double position){
    add_command(new EigenCommandPosition(address, position));
}

void eigen_set_velocity(eigen_addr_t address, double velocity){
    add_command(new EigenCommandVelocity(address, velocity));
}

void eigen_set_effort(eigen_addr_t address, double effort){
    add_command(new EigenCommandEffort(address, effort));
}

void eigen_command_run(eigen_addr_t address){
    add_command(new EigenCommandRun(address));
}

void eigen_set_zero(uint8_t address){
    add_command(new EigenCommandZero(address));
}

void eigen_set_motor_enable(eigen_addr_t address, uint8_t mode){
    add_command(new EigenCommandMotorEnable(address, mode));
}

/* Info commands */
void eigen_poll_status(eigen_addr_t address, uint8_t action){
    add_command(new EigenCommandQuery(address, action));
}

void eigen_poll_topology(eigen_addr_t address){
    add_command(new EigenCommandTopology(address));
}

void eigen_read_parameter(eigen_addr_t address, uint8_t param_id){
    add_command(new EigenCommandParamRead(address, param_id));
}

void eigen_write_parameter(eigen_addr_t address, uint8_t param_id, EigenParameter param){
    add_command(new EigenCommandParamWrite(address, param_id, param));
}

void eigen_firmware_utility(eigen_addr_t address, uint16_t action){
    add_command(new EigenCommandUtility(address, action));
}

/* Special commands */
void eigen_user_command(eigen_addr_t address, std::string command){
    add_command(new EigenCommandUser(address, command));
}
void eigen_start_bootload(uint8_t address, uint8_t mode, std::string file){
    add_command(new EigenCommandBootloader(address, EigenCommandBootloader::BOOTLOADER_START, mode, file));
}
