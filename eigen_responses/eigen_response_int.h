#ifndef EIGEN_RESPONSE_INT_H
#define EIGEN_RESPONSE_INT_H

#include "eigen_response.h"

class EigenResponseInt : public EigenResponse{
public:
    EigenResponseInt(eigen_addr_t address, std::string packet, response_t message_type);

protected:
    int64_t value;
    bool value_valid;
};

class EigenResponseUint : public EigenResponse{
public:
    EigenResponseUint(eigen_addr_t address, std::string packet, response_t message_type);

protected:
    uint64_t value;
};

class EigenResponseEncoderStatus : public EigenResponseUint{
public:
    EigenResponseEncoderStatus(eigen_addr_t address, std::string packet);

    EigenUpdate *update_module(ModuleShared mod) override;
};

#endif // EIGEN_RESPONSE_INT_H
