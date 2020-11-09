#ifndef EIGEN_COMMAND_UTILITY_H
#define EIGEN_COMMAND_UTILITY_H

#include "eigen_command.h"

#define EIGEN_MOTOR_ENABLE  (1)
#define EIGEN_MOTOR_DISABLE (0)

class EigenCommandUtility : public EigenCommand{
public:
    EigenCommandUtility(eigen_addr_t address, uint16_t mode);

    std::string packet() const override;
    std::string expected_response() const override;
    EigenCommand *clone() const override;

private:
    const uint16_t mode_;
};

#endif // EIGEN_COMMAND_UTILITY_H
