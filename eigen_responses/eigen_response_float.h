#ifndef EIGEN_RESPONSE_FLOAT_H
#define EIGEN_RESPONSE_FLOAT_H

#include "eigen_response.h"

class EigenResponseFloat : public EigenResponse{
public:
    EigenResponseFloat(std::string packet);

protected:
    double value;
    bool value_valid;
};

class EigenResponsePosition : public EigenResponseFloat{
public:
    EigenResponsePosition(std::string packet);

    module_update_enum type() override;
    bool update_module(ModuleShared mod) override;
};

class EigenResponseVelocity : public EigenResponseFloat{
public:
    EigenResponseVelocity(std::string packet);

    module_update_enum type() override;
    bool update_module(ModuleShared mod) override;
};

class EigenResponseEffort : public EigenResponseFloat{
public:
    EigenResponseEffort(std::string packet);

    module_update_enum type() override;
    bool update_module(ModuleShared mod) override;
};

#endif // EIGEN_RESPONSE_FLOAT_H
