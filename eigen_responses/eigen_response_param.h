#ifndef EIGEN_RESPONSE_PARAM_H
#define EIGEN_RESPONSE_PARAM_H

#include "eigen_response.h"

class EigenResponseParamRead : public EigenResponse{
public:
    EigenResponseParamRead(eigen_addr_t address, std::string packet);

    bool update_module(ModuleShared mod) override;
    module_update_enum update_type() override;

private:
    uint8_t id_;
    module_update_enum update_;

};

class EigenResponseParamWrite : public EigenResponse{
public:
    EigenResponseParamWrite(eigen_addr_t address, std::string packet);

    bool update_module(ModuleShared mod) override;
    module_update_enum update_type() override;
    
private:
    uint8_t id_;

};

#endif // EIGEN_RESPONSE_PARAM_H
