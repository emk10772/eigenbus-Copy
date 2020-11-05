#include "eigen_response_topology.h"


EigenResponseTopology::EigenResponseTopology(eigen_addr_t address, std::string packet)
    : EigenResponse(address, packet, EigenResponse::EIGEN_TOPOLOGY) {

    //Split the packet into tokens
    uint8_t count = 0;
    bool topology_updated = 0;
    auto tokens = stringtok(packet_, ",");

    //Parse the tokens into individual values
    for(uint8_t ind = 0; ind < tokens.size(); ind++){
        if(ind == 0){
            type_ = strtoul(tokens[ind].c_str(), NULL, 16);
        } else if (ind == 1){
            orientation_ = strtoul(tokens[ind].c_str(), NULL, 16);
        } else {
            down_.push_back(strtoul(tokens[ind].c_str(), NULL, 16));
        }
    }

}

bool EigenResponseTopology::parse_valid(){
    if(orientation_ > ORIENTATION_MAX) return false;
    if(type_ > NODE_TYPE_MAX) return false;

    return true;
}

bool EigenResponseTopology::update_module(ModuleShared mod){
    bool topology_updated = false;

    if(parse_valid()){
        mod->update_orientation(orientation_);
        mod->update_type(type_);
    } else {
        return false;
    }

    /* If we got updated topology info */
    for(uint8_t ind = 0; ind < down_.size(); ind++){
        topology_updated |= mod->update_downstream(ind, down_[ind]);
    }
    
    return topology_updated;
}

module_update_enum EigenResponseTopology::update_type(){
    return MODULE_DOWNSTREAM;
}
