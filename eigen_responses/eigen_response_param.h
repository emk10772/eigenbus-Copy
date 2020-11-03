#ifndef EIGEN_RESPONSE_PARAM_H
#define EIGEN_RESPONSE_PARAM_H

#include "eigen_response.h"

class EigenResponseParamRead : public EigenResponse{
public:
    EigenResponseParamRead(std::string packet);

    bool update_module(ModuleShared mod) override;
    module_update_enum type() override;

};

class EigenResponseParamWrite : public EigenResponse{
public:
    EigenResponseParamWrite(std::string packet);

    bool update_module(ModuleShared mod) override;
    module_update_enum type() override;

};

#endif // EIGEN_RESPONSE_PARAM_H
