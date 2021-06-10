#ifndef EIGEN_COMMAND_MAILBOX_H
#define EIGEN_COMMAND_MAILBOX_H

#include "eigen_command.h"
#include <stdint.h>
#include <string>

class EigenCommandMailboxRead : public EigenCommand{
public:
    EigenCommandMailboxRead(eigen_addr_t address, uint8_t id);

    std::string packet() const override;
    std::string expected_response() const override;
    EigenCommand *clone() const override;
    EigenCommand *clone(eigen_addr_t addr) const override;

private:
    uint8_t id_;
};

class EigenCommandMailboxWrite : public EigenCommand{
public:
    EigenCommandMailboxWrite(eigen_addr_t address, uint8_t id, EigenMailbox data);

    std::string packet() const override;
    std::string expected_response() const override;
    EigenCommand *clone() const override;
    EigenCommand *clone(eigen_addr_t addr) const override;

private:
    EigenMailbox data_;
    uint8_t id_;
};

#endif // EIGEN_COMMAND_MAILBOX_H
