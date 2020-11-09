#ifndef EIGEN_MODULE_H
#define EIGEN_MODULE_H

#include <stdint.h>
#include <vector>
#include <string>
#include <mutex>
#include "eigen_parameters.h"

#define MAX_LED_CODE_LEN (5)

typedef struct module_param_struct{
    uint8_t dirty;
    uint8_t type;
    std::string name;
    uint64_t value;
} module_param;

typedef struct module_down_struct{
    uint8_t addr_current;
    uint8_t addr_diff;
    uint8_t consistency_count;
    std::string name;
} module_down_port;

/* EigenModule class
    Used to store information about modules for use by the program
*/
class EigenModule{
public:
    EigenModule(uint8_t address);
    ~EigenModule();

private:
    uint8_t address;
    double position_;
    double velocity_;
    double effort_;
    uint16_t encoder_status;
    mutable std::mutex mutex;
    uint64_t UID;
    uint8_t type;
    uint8_t orientation;

    std::vector<module_down_port> downstream_list;

public:
    void set_encoder_status(uint16_t status);
    void set_position(double position);
    void set_velocity(double velocty);
    void set_effort(double effort);
    void add_downstream(uint8_t node_addr);
    bool update_downstream(uint8_t ind, uint8_t node_addr);
    void set_downstream_name(uint8_t ind, std::string name);
    void clear_downstream();
    std::vector<module_down_port> downstream() const;
    std::string print_topology() const;

    void update_UID(uint64_t UID_);
    void update_type(uint8_t type_);
    void update_orientation(uint8_t orientation_);

    uint16_t get_encoder_status() const;
    double position() const;
    double velocity() const;
    double effort() const;
    uint8_t get_address() const;
    std::string print_mod_name() const;
    std::string print_UID() const;
    uint64_t get_UID() const;
    std::string print_type() const;
    uint8_t get_type() const;
    uint8_t get_hardware_type() const;
    std::string print_orientation() const;

    double last_position_cmd;
    double last_velocity_cmd;
    double last_effort_cmd;

    //Firmware version
    std::string firmware_version;
    std::string firmware_build_name;
    std::string firmware_build_time;
    std::string firmware_tag;

    std::string last_debug_msg;
    uint64_t t_last_update;

    //Status info
    std::string module_status;
    uint8_t status_code;
    uint8_t sync_ind;
    uint16_t sync_reg;
    uint8_t LED_code[MAX_LED_CODE_LEN + 1];
    uint64_t t_sync;
    bool stale;

    //Command Support vector
    uint8_t command_support;

    //Functions
    std::string print_encoder_status() const;

    bool operator<(EigenModule other) const{
        return address < other.address;
    }
    bool operator<(uint8_t other) const{
        return address < other;
    }

    EigenParameterSet<EigenParameter> parameters;

};

using ModuleConst = std::shared_ptr<EigenModule const>;
using ModuleShared = std::shared_ptr<EigenModule>;

#endif // EIGEN_MODULE_H
