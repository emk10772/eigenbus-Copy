#ifndef EIGEN_RESPONSE_H
#define EIGEN_RESPONSE_H

#include <stdint.h>
#include <vector>
#include <string>
#include <mutex>

#include "../eigen_comms.h"

class EigenResponse{
public:
    EigenResponse(std::string packet);
    ~EigenResponse();

    virtual bool update_module(ModuleShared mod);
    virtual module_update_enum type();

protected:
    const std::string packet_;
};

#endif // EIGEN_RESPONSE_H
