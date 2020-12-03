#ifndef EIGEN_RESPONSE_STRING_H
#define EIGEN_RESPONSE_STRING_H

#include "eigen_response.h"

class EigenResponseString : public EigenResponse{
public:
    EigenResponseString(eigen_addr_t address, std::string packet, response_t message_type);
};

class EigenResponseEcho : public EigenResponseString{
public:
    EigenResponseEcho(eigen_addr_t address, std::string packet);

    EigenUpdate *update_module(ModuleShared mod, uint64_t latency) override;
};
#endif // EIGEN_RESPONSE_FLOAT_H
