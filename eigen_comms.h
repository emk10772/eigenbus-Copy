#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <mutex>
#include <exception>
#include <memory>

/* POLL DEFINITIONS */
#define EIGEN_POLL_LOCATION         (0x01)
#define EIGEN_POLL_VELOCITY         (0x02)
#define EIGEN_POLL_ACCELERATION     (0x04)
#define EIGEN_POLL_TORQUE           (0x08)
#define EIGEN_POLL_EFFORT           (0x10)
#define EIGEN_POLL_IMU              (0x20)
#define EIGEN_POLL_ENC_STATUS       (0x40)
#define EIGEN_POLL_RESERVED         (0x80)

/* Encoder Status bits */
#define ENC_COUNTER_ERROR       (0x01 << 15)
#define ENC_SIG_AMP_ERROR       (0x01 << 14)
#define ENC_SIG_AMP_WARNING     (0x01 << 13)
#define ENC_MAG_SENSOR_ERROR    (0x01 << 12)
#define ENC_SENSOR_READ_ERROR   (0x01 << 11)
#define ENC_ENC_CONFIG_ERROR    (0x01 << 10)
#define ENC_DATA_INVALID_ERROR  (0x01 << 9)
#define ENC_OP_LIMITS_WARNING   (0x01 << 8)
#define ENC_SIG_AMP_HIGH_WARN   (0x01 << 7)
#define ENC_SIG_AMP_LOW_WARN    (0x01 << 6)
#define ENC_SIG_LOST_ERROR      (0x01 << 5)
#define ENC_TEMP_WARNING        (0x01 << 4)
#define ENC_SUPPLY_ERROR        (0x01 << 3)
#define ENC_SYSTEM_ERROR        (0x01 << 2)
#define ENC_MAG_PATTERN_ERROR   (0x01 << 1)
#define ENC_ACCELERATION_ERROR  (0x01 << 0)

#define EIGEN_ENABLED               (0x01)
#define EIGEN_DISABLED              (0x00)

#define EIGEN_PACKET_SEND           (0x01)
#define EIGEN_PACKET_RECV           (0x02)

/* Command Support Definitions */
#define POSITION_CAPABLE    (0x01)
#define SPEED_CAPABLE       (0x02)
#define EFFORT_CAPABLE      (0x04)

/* Param Types */
#define _UINT8                  (1)
#define _UINT16                 (2)
#define _UINT32                 (3)
#define _FLOAT                  (4)
#define _UINT64                 (5)
#define _DOUBLE                 (6)

/* Sizes in EEPROM */
#define S_UINT8                 (1)
#define S_UINT16                (2)
#define S_UINT32                (4)
#define S_FLOAT                 (4)
#define S_UINT64                (8)
#define S_DOUBLE                (8)

#define TOPOLOGY_CONSISTENCY_COUNT (3)
#define MAX_LED_CODE_LEN (5)

/* Firmware Utility Commands */
#define EIGEN_UTIL_STAT_CODE            (0x001)
#define EIGEN_UTIL_COMMIT_VERSION       (0x002)
#define EIGEN_UTIL_BUILD_TIME           (0x004)
#define EIGEN_UTIL_BUILD_USER           (0x008)
#define EIGEN_UTIL_GIT_DESCRIBE         (0x010)
#define EIGEN_UTIL_MODULE_STATUS        (0x020)
#define EIGEN_UTIL_MODULE_CAPABILITY    (0x040)
#define EIGEN_UTIL_MODULE_UID           (0x080)
#define EIGEN_UTIL_MODULE_PORTS         (0x100)
#define EIGEN_UTIL_DISABLE_CHECKSUM     (0x200)

/* Node Types
    Documented Here: https://docs.google.com/document/d/10HxQWy6gR4vNm7ubD_OZE42J9Y9vgy9ribtj6P2n49I/edit?usp=sharing
*/
#define NODE_TYPE_WHEEL         (1)
#define NODE_TYPE_TWIST         (2)
#define NODE_TYPE_BEND          (3)
#define NODE_TYPE_GRIPPER_FOOT  (4)
#define NODE_TYPE_GRIPPER       (5)
#define NODE_TYPE_O_6           (6)
#define NODE_TYPE_BATTERY       (7)
#define NODE_TYPE_EIGENBODY     (8)
#define NODE_TYPE_TEE           (9)
#define NODE_TYPE_FOOT          (10)
#define NODE_TYPE_STAT_NO_BEND  (11)
#define NODE_TYPE_STAT_45_BEND  (12)
#define NODE_TYPE_STAT_90_BEND  (13)
#define NODE_TYPE_HUB_9         (14)

/* Hardware Types */
#define HARDWARE_O6             (0)
#define HARDWARE_EIGEN          (1)
#define HARDWARE_HUB_9          (2)
#define HARDWARE_MISC           (3)

class Module;
using ModuleConst = std::shared_ptr<Module const>;

typedef struct eigen_config_struct{
    uint8_t poll_encoder_status;
    uint8_t poll_module_position;
    uint8_t poll_module_velocity;
    uint8_t poll_module_effort;
    uint8_t raw_packet_en;
    uint8_t max_retries;
} eigen_config;

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

typedef enum update_enum{
    MODULE_TOUCHED = 0,
    MODULE_ERROR = 1,
    MODULE_ADDED = 2,
    MODULE_STALE = 3,
    MODULE_PARAM_READ = 4,
    MODULE_PARAM_ADD = 5,
    MODULE_DOWNSTREAM = 6,
    MODULE_REMOVED = 7,
    MODULE_BTLDR_PROGRESS = 8,
    MODULE_BTLDR_END = 9,
    MODULE_POS_UPDATE = 10,
    MODULE_VEL_UPDATE = 11,
    MODULE_EFF_UPDATE = 12
} module_update_enum;

typedef struct module_update_struct{
    uint8_t index;
    uint8_t arg;
    update_enum update_type;
    std::string data;
} module_update;

typedef enum cmd_enum{
    CMD_POS,
    CMD_SPEED,
    CMD_EFFORT,
    CMD_READ_PARAM,
    CMD_WRITE_PARAM,
    CMD_READ_PARAM_LIST,
    CMD_SET_MOTOR_ENABLE,
    CMD_USER,
    CMD_RUN, //Run position, speed, effort commands
    CMD_POLL_TOPOLOGY,
    CMD_UID_WR_ADDR,
#ifdef EIGEN_BTLDR_SUPPORT
    CMD_BTLDR,
#endif
    CMD_ZERO
} command_enum;

typedef struct eigen_command_struct{
    uint8_t address;
    command_enum command_type;
    uint64_t arg1;
    std::string arg2;
} eigen_command;

typedef enum packet_type_enum{
    EIGEN_PACKET_DEFAULT,       //Used for everything else
    EIGEN_PACKET_POLL,          //Used for general poll commands
    EIGEN_PACKET_TOPO,          //Used for topology commands
    EIGEN_PACKET_DEBUG,         //Debug messages
    EIGEN_PACKET_CLI            //Used for user command line input
} packet_type;

typedef struct raw_packet_struct{
    std::string packet;
    packet_type type;
    uint8_t dir;
} raw_packet;

typedef struct eigen_stats_struct{
    uint64_t uptime_ms;
    uint64_t sent_packets;
    uint64_t successful_packets;
    uint64_t dropped_packets;
    uint64_t unrequested_packets;
    uint64_t retried_packets;
    uint64_t frame_time_ms;
    std::string last_dropped_packet;
} eigen_stats;

//Interface running functions
void start_eigen_comms(uint16_t (*read)(uint8_t *buf, uint16_t max_len, int t_wait_ms),
                       void (*write)(uint8_t *buf, uint16_t len));
void clean_eigen_comms();
int service_eigen_comms();
bool get_poll_enabled();
void set_poll_enabled(bool state);
module_update *get_module_update();
uint64_t current_time_ms();
bool is_bootloader_active();
bool is_bootloader_finished();

//Config Interface
void set_eigen_config(eigen_config config);
eigen_config get_eigen_config();

//Command Interface
void add_command(eigen_command *command);
void add_command(uint8_t addr, command_enum cmd_type, uint64_t arg1);
void add_command(uint8_t addr, command_enum cmd_type, std::string arg2);
void add_command(uint8_t addr, command_enum cmd_type, uint64_t arg1, std::string arg2);
void clear_commands();

//Packet tracking interface
raw_packet *get_raw_packet();
eigen_stats get_eigen_stats(); //TODO: Change to ptr?

ModuleConst get_module(uint8_t address);
ModuleConst get_module_by_index(uint8_t index);
uint8_t num_modules();
bool list_updated();

/* Module class
    Used to store information about modules for use by the program
*/
class Module{
public:
    Module(uint8_t address);
    ~Module();

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

    //Parameters
    std::vector<module_param> param_list;
    uint64_t t_last_param_update;
    uint8_t expected_num_params;
    uint8_t received_params;

    std::vector<module_down_port> downstream_list;

public:
    void set_encoder_status(uint16_t status);
    void set_position(double position);
    void set_velocity(double velocty);
    void set_effort(double effort);
    void add_downstream(uint8_t node_addr);
    uint8_t update_downstream(uint8_t ind, uint8_t node_addr);
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

    bool operator<(Module other) const{
        return address < other.address;
    }
    bool operator<(uint8_t other) const{
        return address < other;
    }

    void add_parameter(uint8_t id, uint8_t type, std::string name);
    void update_parameter(uint8_t param, uint64_t value);
    uint64_t read_parameter(uint8_t param);
    void set_param_last_update();
    void set_expected_parameters(uint8_t num_parameters);
    uint8_t parameters_left() const;
    uint64_t d_t_param_last_update() const;
    std::string print_parameter(uint8_t param) const;
    std::string parameter_name(uint8_t id) const;
    uint8_t parameter_type(uint8_t id) const;

};

class EigenPacketFilter{
public:
    EigenPacketFilter(uint8_t address, std::string response_filter, packet_type packet, std::string packet_string);
    ~EigenPacketFilter();

private:
    uint8_t address;
    packet_type classification;
    std::string response_filter;
    std::vector<std::string> matched_responses;
    std::set<uint8_t> matched_addresses;
    uint64_t t_sent;
    uint8_t retries;
    std::string packet_string_;

public:
    bool expects_response();
    bool packet_timeout();
    uint8_t num_responses();
    bool matches_filter(uint8_t address, std::string packet);
    bool is_broadcast();
    void add_response(uint8_t address, std::string response);
    packet_type get_type();
    uint8_t num_retries();
    void increment_retry_count();
    void reset_timeout();
    std::string packet_string();
};

