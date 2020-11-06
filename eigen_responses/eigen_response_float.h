#ifndef EIGEN_RESPONSE_FLOAT_H
#define EIGEN_RESPONSE_FLOAT_H

#include "eigen_response.h"

class EigenResponseFloat : public EigenResponse{
public:
    EigenResponseFloat(eigen_addr_t address, std::string packet, response_t message_type);

protected:
    double value;
    bool value_valid;
};

class EigenResponsePosition : public EigenResponseFloat{
public:
    EigenResponsePosition(eigen_addr_t address, std::string packet);

    EigenUpdate *update_module(ModuleShared mod) override;
};

class EigenResponseVelocity : public EigenResponseFloat{
public:
    EigenResponseVelocity(eigen_addr_t address, std::string packet);

    EigenUpdate *update_module(ModuleShared mod) override;
};

class EigenResponseEffort : public EigenResponseFloat{
public:
    EigenResponseEffort(eigen_addr_t address, std::string packet);

    EigenUpdate *update_module(ModuleShared mod) override;
};

#endif // EIGEN_RESPONSE_FLOAT_H
