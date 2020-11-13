#ifndef EIGEN_RESPONSE_UPTIME_H
#define EIGEN_RESPONSE_UPTIME_H

#include "eigen_response.h"

class EigenResponseUptime : public EigenResponse{
public:
    EigenResponseUptime(eigen_addr_t address, std::string packet);

    EigenUpdate *update_module(ModuleShared mod) override;
    bool isSpontaneous() override;

protected:
    uint32_t time;
};


#endif // EIGEN_RESPONSE_UPTIME_H
