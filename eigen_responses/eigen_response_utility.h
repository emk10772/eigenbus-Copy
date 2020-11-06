#ifndef EIGEN_RESPONSE_UTILITY_H
#define EIGEN_RESPONSE_UTILITY_H

#include "eigen_response.h"

class EigenResponseUtility : public EigenResponse{
public:
    EigenResponseUtility(eigen_addr_t address, std::string packet);

    EigenUpdate *update_module(ModuleShared mod) override;

private:
    uint16_t id_;
};

#endif // EIGEN_RESPONSE_UTILITY_H
