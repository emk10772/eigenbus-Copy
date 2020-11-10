#include "eigen_response_mailbox.h"


EigenResponseMailboxRead::EigenResponseMailboxRead(eigen_addr_t address, std::string packet)
    : EigenResponse(address, packet, EigenResponse::EIGEN_MAIL_READ) {
    
}

EigenUpdate *EigenResponseMailboxRead::update_module(ModuleShared mod){
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
                responses_.push_back(strprintf("|(00,%02X", i));
            }

            mod->parameters.set_expected_parameters(mail_type);
            return nullptr;
        } else {
            std::string name = tokens[2];
            //service_eigencomms should null terminate the string for us

            mod->mailboxes.add(mail_addr, EigenMailbox(mail_type), name);
            return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_MAIL_ADD, mail_addr);
        }
    } else {
        //Write value to module
        mod->mailboxes.ref(id_).update_value(packet_.substr(ind + 1));
        return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_MAIL_READ, id_);
    }
}


EigenResponseMailboxWrite::EigenResponseMailboxWrite(eigen_addr_t address, std::string packet)
    : EigenResponse(address, packet, EigenResponse::EIGEN_MAIL_WRITE) {

    
}

EigenUpdate *EigenResponseMailboxWrite::update_module(ModuleShared mod){
    size_t ind = 0;
    id_ = stoul(packet_, &ind, EIGENBUS_BASE);

    if(ind != 2 || packet_[ind] != ',')
        return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_MAIL_ERR, id_);;

    if(packet_[ind + 1] == 's'){
        return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_MAIL_WRITE, id_);
    } else {
        return new EigenUpdate(mod->get_address(), EigenUpdate::MODULE_MAIL_ERR, id_);
    }
}

