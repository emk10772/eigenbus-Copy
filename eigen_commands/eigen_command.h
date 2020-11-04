#ifndef EIGEN_COMMAND_H
#define EIGEN_COMMAND_H

#include "../eigen_comms.h"
#include <stdint.h>
#include <string>
#include "../eigen_packet_filter.h"

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

    virtual std::string packet();
    virtual std::string expected_response();

    packet_type type();
    command_t command_type();
    eigen_addr_t address();
    void update_module(ModuleShared mod);

protected:
    const eigen_addr_t address_;
    const packet_type type_;
    const command_t command_;
};

#define MAX_COMMAND_OUT (128)
template< typename... argv >
inline std::string strprintf(const char* format, argv... args){
    char buffer[MAX_COMMAND_OUT];

    snprintf(buffer, MAX_COMMAND_OUT, format, args);
    return std::string(buffer);
}

#endif // EIGEN_COMMAND_H
