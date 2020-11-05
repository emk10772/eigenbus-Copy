#ifndef EIGEN_RESPONSE_TOPOLOGY_H
#define EIGEN_RESPONSE_TOPOLOGY_H

#include "eigen_response.h"

class EigenResponseTopology : public EigenResponse{
public:
    EigenResponseTopology(eigen_addr_t address, std::string packet);

    bool update_module(ModuleShared mod) override;
    module_update_enum update_type() override;

private:
    uint8_t type_;
    uint8_t orientation_;
    std::vector<uint8_t> down_;

    bool parse_valid();
};

#endif // EIGEN_RESPONSE_TOPOLOGY_H
