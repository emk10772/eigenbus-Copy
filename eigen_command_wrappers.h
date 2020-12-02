#ifndef EIGEN_COMMAND_WRAPPERS_H
#define EIGEN_COMMAND_WRAPPERS_H

#include "eigen_commands/eigen_commands.h"

/* Locomotion commands */
void eigen_set_position(eigen_addr_t address, double position);
void eigen_set_velocity(eigen_addr_t address, double velocity);
void eigen_set_effort(eigen_addr_t address, double effort);
void eigen_command_run(eigen_addr_t address);
void eigen_set_zero(uint8_t address);
void eigen_set_motor_enable(eigen_addr_t address, uint8_t mode);

/* Info commands */
void eigen_poll_status(eigen_addr_t address, uint8_t action);
void eigen_poll_topology(eigen_addr_t address);
void eigen_read_parameter(eigen_addr_t address, uint8_t param_id);
void eigen_write_parameter(eigen_addr_t address, uint8_t param_id, std::string param);
void eigen_read_mailbox(eigen_addr_t address, uint8_t id);
void eigen_write_mailbox(eigen_addr_t address, uint8_t id, std::string value);
void eigen_firmware_utility(eigen_addr_t address, uint16_t action);

/* Special commands */
void eigen_user_command(eigen_addr_t address, std::string command);
void eigen_start_bootload(uint8_t address, uint8_t mode, std::string file);
void eigen_echo(eigen_addr_t address, std::string value);

#endif
