#ifndef EIGEN_RESPONSE_BROADCAST_H
#define EIGEN_RESPONSE_BROADCAST_H

#include "eigen_response.h"

class EigenResponseBroadcast : public EigenResponse{
public:
    EigenResponseBroadcast(eigen_addr_t address, std::string packet);

    EigenUpdate *update_module(ModuleShared mod, uint64_t latency) override;

    bool isSpontaneous() override;

private:
    std::string data_;
};

#endif // EIGEN_RESPONSE_BROADCAST_H
