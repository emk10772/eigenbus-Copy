#ifndef EIGEN_MODULE_H
#define EIGEN_MODULE_H

#include <stdint.h>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <deque>
#include <memory>
#include <array>
#include <set>
#include "eigen_parameters.h"
#include "eigen_variable.h"

#define MAX_LED_CODE_LEN (5)
#define MAX_MOD_LATENCY_MEASUREMENT (20)

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

#define VAR_LIST \
    ENTRY(EIGEN_ADDRESS,        EigenUint8,     address) \
    ENTRY(EIGEN_UID,            EigenUint64,    UID) \
    ENTRY(EIGEN_TYPE,           EigenUint8,     type) \
    ENTRY(EIGEN_ORIENTATION,    EigenUint8,     orientation) \
    ENTRY(EIGEN_POSITION,       EigenDouble,    position) \
    ENTRY(EIGEN_VELOCITY,       EigenDouble,    velocity) \
    ENTRY(EIGEN_EFFORT,         EigenDouble,    effort) \
    ENTRY(EIGEN_ENC_STATUS,     EigenUint16,    encoder_status) \
    ENTRY(EIGEN_NODE_DEPTH,     EigenUint8,     node_depth) \
    ENTRY(EIGEN_FIRMW_VER,      EigenString,    firmware_version) \
    ENTRY(EIGEN_FIRMW_BLD_NAME, EigenString,    firmware_build_name) \
    ENTRY(EIGEN_FIRMW_BLD_TIME, EigenString,    firmware_build_time) \
    ENTRY(EIGEN_FIRMW_BLD_TAG,  EigenString,    firmware_tag) \
    ENTRY(EIGEN_MODULE_NAME,    EigenString,    module_name) \
    ENTRY(EIGEN_CMD_SUPPORT,    EigenUint8,     command_support) \
    ENTRY(EIGEN_LAST_POS_CMD,   EigenDouble,    last_position_cmd) \
    ENTRY(EIGEN_LAST_VEL_CMD,   EigenDouble,    last_velocity_cmd) \
    ENTRY(EIGEN_LAST_EFF_CMD,   EigenDouble,    last_effort_cmd) \
    ENTRY(EIGEN_MODULE_STATUS,  EigenString,    module_status)


typedef enum{
#define ENTRY(e_name, type, v_name) e_name,
    VAR_LIST
#undef ENTRY
    EIGEN_NUM_VARIABLES
} EigenModuleVariables;

class EigenModule{
public:
    EigenModule(uint8_t address_);
    ~EigenModule();

    //Forward declarations and initializations of all variables
#define ENTRY(e_name, type, v_name) type v_name = type(#e_name);
    VAR_LIST
#undef ENTRY

    //Indexable array of all variables
    std::vector<const EigenVariable *> variable_list;

private:
    mutable std::mutex mutex;

    std::vector<module_down_port> downstream_list;

    std::deque<uint64_t> latencies_;
    uint64_t latency_total_;
    std::atomic<double> latency_avg_;
    std::atomic<double> latency_peak_;

public:
    void set_encoder_status(uint16_t status);
    void add_downstream(uint8_t node_addr);
    bool update_downstream(uint8_t ind, uint8_t node_addr);
    void set_downstream_name(uint8_t ind, std::string name);
    void clear_downstream();
    std::vector<module_down_port> downstream() const;
    std::string print_topology() const;

    void update_UID(uint64_t UID_);
    void update_type(uint8_t type_);
    void update_orientation(uint8_t orientation_);

    void add_latency_measurement(uint64_t latency);
    double avg_latency() const;
    double peak_latency() const;

    uint16_t get_encoder_status() const;
    std::string print_mod_name() const;
    std::string print_UID() const;
    std::string print_type() const;
    uint8_t get_type() const;
    uint8_t get_hardware_type() const;
    std::string print_orientation() const;
    bool valid() const;

    uint64_t t_broadcast_sync_start;
    uint64_t t_broadcast_offset;
    uint8_t broadcast_sync_count;
    uint8_t broadcast_reg;
    uint16_t broadcast_period;

    std::string last_debug_msg;
    uint64_t t_last_update;

    EigenParameterSet<EigenVariable> parameters;
    EigenParameterSet<EigenVariable> mailboxes;

    //Status info
    uint8_t status_code;
    uint8_t sync_ind;
    uint16_t sync_reg;
    uint8_t LED_code[MAX_LED_CODE_LEN + 1];
    uint64_t t_sync;
    bool stale;
    uint32_t t_last_uptime;

    //Functions
    std::string print_encoder_status() const;
};

using ModuleConst = std::shared_ptr<EigenModule const>;
using ModuleShared = std::shared_ptr<EigenModule>;

inline ModuleConst make_const(ModuleShared module){
    return std::const_pointer_cast<const EigenModule>(module);
}

#endif // EIGEN_MODULE_H
