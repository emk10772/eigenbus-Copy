#ifndef EIGEN_COMMAND_MOTOR_ENABLE_H
#define EIGEN_COMMAND_MOTOR_ENABLE_H

#include "eigen_command.h"

#define EIGEN_MOTOR_ENABLE  (1)
#define EIGEN_MOTOR_DISABLE (0)

class EigenCommandMotorEnable : public EigenCommand{
public:
    EigenCommandMotorEnable(eigen_addr_t address, uint8_t enable);

    std::string packet() const override;
    std::string expected_response() const override;
    EigenCommand *clone() const override;

private:
    uint8_t enable_;
};

#endif // EIGEN_COMMAND_MOTOR_ENABLE_H
