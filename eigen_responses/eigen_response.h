#ifndef EIGEN_RESPONSE_H
#define EIGEN_RESPONSE_H

#include <stdint.h>
#include <vector>
#include <string>
#include <mutex>

#include "../eigen_utils.h"
#include "../eigen_module.h"

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

    EigenResponse(eigen_addr_t address, std::string packet, response_t message_type);
    ~EigenResponse();

    virtual bool update_module(ModuleShared mod);
    virtual module_update_enum update_type();

    std::vector<std::string> additional_responses();
    bool has_additonal_responses();
    response_t message_type();
    std::string packet();
    eigen_addr_t address();

protected:
    const eigen_addr_t address_;
    const std::string packet_;
    const response_t message_type_;
    std::vector<std::string> responses_;
};

#endif // EIGEN_RESPONSE_H
