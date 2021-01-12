#ifndef EIGEN_UTILS_H
#define EIGEN_UTILS_H

#include <stdint.h>
#include <vector>
#include <map>
#include <string>
#include <mutex>
#include <chrono>
#include <deque>
#include <cmath>
#include <algorithm>

/* Common Timeouts / Periods in milliseconds */
#define UPDATE_PERIOD           (100)
#define UPDATE_TOPOLOGY_PERIOD  (1000)
#define UPDATE_STATUS_PERIOD    (250)
#define PACKET_TIMEOUT          (600)

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
#define PARAM_TYPE_MAX          (_DOUBLE)

typedef union{
    uint8_t     uint8_;
    uint16_t    uint16_;
    uint32_t    uint32_;
    uint64_t    uint64_;
    float       float_;
    double      double_;
} eigen_param_t;

/* Sizes in EEPROM */
#define S_UINT8                 (1)
#define S_UINT16                (2)
#define S_UINT32                (4)
#define S_FLOAT                 (4)
#define S_UINT64                (8)
#define S_DOUBLE                (8)

#define TOPOLOGY_CONSISTENCY_COUNT (3)
#define EIGENBUS_BASE           (16)

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
#define EIGEN_UTIL_MODULE_NAME          (0x400)

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
#define NODE_TYPE_MAX           (NODE_TYPE_HUB_9)

/* Hardware Types */
#define HARDWARE_O6             (0)
#define HARDWARE_EIGEN          (1)
#define HARDWARE_HUB_9          (2)
#define HARDWARE_MISC           (3)

/* Orientation */
#define ORIENTATION_NONE        (0)
#define ORIENTATION_MAX         (8)

#define LIST_PARAM              (0)
#define ADDR_PARAM              (1)
#define TYPE_PARAM              (2)

typedef uint8_t eigen_addr_t;

typedef enum packet_type_enum{
    EIGEN_PACKET_DEFAULT,       //Used for everything else
    EIGEN_PACKET_POLL,          //Used for general poll commands
    EIGEN_PACKET_TOPO,          //Used for topology commands
    EIGEN_PACKET_DEBUG,         //Debug messages
    EIGEN_PACKET_CLI,           //Used for user command line input
    EIGEN_PACKET_NONE
} packet_type;

typedef struct raw_packet_struct{
    std::string packet;
    packet_type type;
    uint8_t dir;
} raw_packet;

//Function source:
//    https://stackoverflow.com/questions/59572907/why-strtok-takes-a-char-and-not-a-const-char

inline std::vector<std::string> stringtok (const std::string& s, const std::string& delim)
{
    std::vector<std::string> v {};  /* vector of strings for tokens */
    size_t beg = 0, end = 0;        /* begin and end positons in str */

    /* while non-delimiter char found */
    while ((beg = s.find_first_not_of (delim, end)) != std::string::npos) {
        end = s.find_first_of (delim, beg);       /* find delim after non-delim */
        v.push_back (s.substr (beg, end - beg));  /* add substr to vector */
        if (end == std::string::npos)             /* if last delim, break */
            break;
    }

    return v;   /* return vector of tokens */
}

extern const uint8_t CRC_8_TABLE[256];

inline uint8_t crc_8_ccitt(const char *data, uint16_t len){
    uint8_t crc = 0xFF; //Seed of 0xFF

    for(uint16_t ind = 0; ind < len; ind++){
        uint8_t temp = crc ^ data[ind];
        crc = CRC_8_TABLE[temp];
    }

    return crc; //Final of 0x00
}

#define STR_PRINT_MAX (128)
std::string strprintf(const char* format, ...);


inline uint64_t current_time_ms() {
    auto current_time = std::chrono::system_clock::now();
    auto epoch = current_time.time_since_epoch();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
    return milliseconds.count();
}

/* EigenQueue:
 * A thread safe wrapper for std::deque
 */
template <class T>
class EigenQueue{
public:
    inline ~EigenQueue(){
        clear();
    }

    inline void add(T *item){
        std::lock_guard<std::mutex> lock(mutex_);
        deque_.push_back(item);
    }

    /* Transfer items from deque to this deque */
    inline void add(std::deque<T*> &items){
        std::lock_guard<std::mutex> lock(mutex_);

        while(items.size() > 0){
            deque_.push_back(items.front());
            items.pop_front();
        }
    }

    /* Copy items from vector to this deque */
    inline void add(std::vector<T*> items){
        std::lock_guard<std::mutex> lock(mutex_);

        for(auto item : items){
            deque_.push_back(item);
        }
    }

    inline T* get(){
        T *retval = nullptr;
        std::lock_guard<std::mutex> lock(mutex_);

        if(deque_.size() > 0){
            retval = deque_.front();
            deque_.pop_front();
        }

        return retval;
    }

    inline void clear(){
        std::lock_guard<std::mutex> lock(mutex_);

        for(T* item : deque_){
            delete item;
        }
        deque_.clear();
    }

private:
    std::deque<T *> deque_;
    std::mutex mutex_;

};

#endif
