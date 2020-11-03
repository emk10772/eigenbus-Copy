#ifndef EIGEN_COMMAND_H
#define EIGEN_COMMAND_H

#include "../eigen_comms.h"
#include <stdint.h>
#include <string>
#include "../eigen_packet_filter.h"

class EigenCommand{
public:
    EigenCommand(eigen_addr_t address);
    ~EigenCommand();

    virtual std::string packet();
    virtual std::string expected_response();
    virtual packet_type type();
    void update_module(ModuleShared mod);

    const eigen_addr_t address_;
};

#define MAX_COMMAND_OUT (128)
template< typename... argv >
inline std::string strprintf(const char* format, argv... args){
    char buffer[MAX_COMMAND_OUT];

    snprintf(buffer, MAX_COMMAND_OUT, format, args);
    return std::string(buffer);
}

#endif // EIGEN_COMMAND_H
