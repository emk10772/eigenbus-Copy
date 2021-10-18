#include "eigen_response_mailbox.h"


EigenResponseMailboxRead::EigenResponseMailboxRead(eigen_addr_t address, std::string packet)
    : EigenResponse(address, packet, EigenResponse::EIGEN_MAIL_READ) {
    
}

EigenUpdate *EigenResponseMailboxRead::update_module(ModuleShared mod, uint64_t latency){
    try{
        size_t ind = 0;
        id_ = stoul(packet_, &ind, EIGENBUS_BASE);

        if(ind != 2 || packet_[ind] != ',') return nullptr;

        if(id_ == 0){
            //Push the pointer along, process values
            auto tokens = stringtok(packet_.substr(ind+1), ",");
            uint8_t mail_addr = stoul(tokens[0], nullptr, EIGENBUS_BASE);
            uint8_t mail_type = stoul(tokens[1], nullptr, EIGENBUS_BASE);

            //If we are receiving the number of parameters, fill in that we expect n parameters
            if(mail_addr == 0){
                for(int i = 1; i < mail_type; i++){
                    responses_.push_back(strprintf("|[00,%02X", i));
                }

                mod->mailboxes.set_expected_parameters(mail_type);
                return nullptr;
            } else {
                std::string name = tokens[2];
                //service_eigencomms should null terminate the string for us

                EigenVariable *variable = nullptr;
                if(mail_type & MAILBOX_INT){
                    variable = new EigenUint64(name);
                } else if(mail_type & MAILBOX_DOUBLE){
                    variable = new EigenDouble(name);
                } else if(mail_type & MAILBOX_STRING){
                    variable = new EigenString(name);
                }

                if(variable) {
                    mod->mailboxes.add(mail_addr, variable, name);
                    variable->set_id(mail_addr);
                    variable->set_address(mod->address);
                    return new EigenUpdate(mod->address, EigenUpdate::MODULE_MAIL_ADD, latency, mod, mail_addr);
                }
            }
        } else {
            //Write value to module
            EigenVariable *variable = mod->mailboxes.value(id_);
            bool valid = false;
            if(variable)
                valid = variable->parse_value(packet_.substr(ind + 1));

            if(valid)
                return new EigenUpdate(mod->address, EigenUpdate::MODULE_MAIL_READ, latency, mod, id_);
        }
    } catch(std::exception e){
        return new EigenUpdate(mod->address, EigenUpdate::MODULE_MAIL_ERR, latency, mod);
    }

    return nullptr;
}


EigenResponseMailboxWrite::EigenResponseMailboxWrite(eigen_addr_t address, std::string packet)
    : EigenResponse(address, packet, EigenResponse::EIGEN_MAIL_WRITE) {

}

EigenUpdate *EigenResponseMailboxWrite::update_module(ModuleShared mod, uint64_t latency){
    size_t ind = 0;
    try {
        id_ = stoul(packet_, &ind, EIGENBUS_BASE);
    } catch (std::exception e) {
        return new EigenUpdate(mod->address, EigenUpdate::MODULE_MAIL_ERR, latency, mod);
    }

    if(ind != 2 || packet_[ind] != ',')
        return new EigenUpdate(mod->address, EigenUpdate::MODULE_MAIL_ERR, latency, mod, id_);

    if(packet_[ind + 1] == 's'){
        return new EigenUpdate(mod->address, EigenUpdate::MODULE_MAIL_WRITE, latency, mod, id_);
    } else {
        return new EigenUpdate(mod->address, EigenUpdate::MODULE_MAIL_ERR, latency, mod, id_);
    }
}

