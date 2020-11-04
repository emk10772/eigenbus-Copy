#ifndef EIGEN_RESPONSE_H
#define EIGEN_RESPONSE_H

#include <stdint.h>
#include <vector>
#include <string>
#include <mutex>

#include "../eigen_comms.h"

class EigenResponse{
public:
    typedef enum{
        EIGEN_POSITION,
        EIGEN_VELOCITY,
        EIGEN_EFFORT,
        EIGEN_PARAM_READ,
        EIGEN_PARAM_WRITE,
        EIGEN_TOPOLOGY,
        EIGEN_UTILITY,
        EIGEN_BOOTLOADER
    } response_t;

    EigenResponse(std::string packet, response_t message_type);
    ~EigenResponse();

    virtual bool update_module(ModuleShared mod);
    virtual module_update_enum update_type();

    response_t message_type();
    std::string packet();

protected:
    const std::string packet_;
    const response_t message_type_;
};

#endif // EIGEN_RESPONSE_H
