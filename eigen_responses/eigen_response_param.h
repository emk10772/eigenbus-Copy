#ifndef EIGEN_RESPONSE_PARAM_H
#define EIGEN_RESPONSE_PARAM_H

#include "eigen_response.h"

class EigenResponseParamRead : public EigenResponse{
public:
    EigenResponseParamRead(eigen_addr_t address, std::string packet);

    EigenUpdate *update_module(ModuleShared mod, uint64_t latency) override;

private:
    uint8_t id_;
};

class EigenResponseParamWrite : public EigenResponse{
public:
    EigenResponseParamWrite(eigen_addr_t address, std::string packet);

    EigenUpdate *update_module(ModuleShared mod, uint64_t latency) override;
private:
    uint8_t id_;

};

#endif // EIGEN_RESPONSE_PARAM_H
