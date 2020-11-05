#ifndef EIGEN_COMMAND_H
#define EIGEN_COMMAND_H

#include "../eigen_utils.h"
#include "../eigen_module.h"
#include <stdint.h>
#include <string>

class EigenCommand{
public:
    typedef enum{
        EIGEN_CMD_POSITION,
        EIGEN_CMD_VELOCITY,
        EIGEN_CMD_EFFORT,
        EIGEN_CMD_PARAM_READ,
        EIGEN_CMD_PARAM_WRITE,
        EIGEN_CMD_TOPOLOGY,
        EIGEN_CMD_UTILITY,
        EIGEN_CMD_BOOTLOADER,
        EIGEN_CMD_USER,
        EIGEN_CMD_RUN,
        EIGEN_CMD_ZERO,
        EIGEN_CMD_QUERY,
        EIGEN_CMD_UID_WRITE,
        EIGEN_CMD_MOTOR_EN
    } command_t;

    EigenCommand(eigen_addr_t address, packet_type type, command_t command);
    ~EigenCommand();

    virtual std::string packet() = 0;
    virtual std::string expected_response() = 0;

    packet_type type();
    command_t command_type();
    eigen_addr_t address();
    void update_module(ModuleShared mod);

protected:
    const eigen_addr_t address_;
    const packet_type type_;
    const command_t command_;
};

#endif // EIGEN_COMMAND_H
