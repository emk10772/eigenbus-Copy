#ifndef EIGEN_RESPONSE_MAILBOX_H
#define EIGEN_RESPONSE_MAILBOX_H

#include "eigen_response.h"

class EigenResponseMailboxRead : public EigenResponse{
public:
    EigenResponseMailboxRead(eigen_addr_t address, std::string packet);

    EigenUpdate *update_module(ModuleShared mod, uint64_t latency) override;

private:
    uint8_t id_;
};

class EigenResponseMailboxWrite : public EigenResponse{
public:
    EigenResponseMailboxWrite(eigen_addr_t address, std::string packet);

    EigenUpdate *update_module(ModuleShared mod, uint64_t latency) override;
private:
    uint8_t id_;

};

#endif // EIGEN_RESPONSE_PARAM_H
