#ifndef EIGEN_PARAMETERS_H
#define EIGEN_PARAMETERS_H

#include "eigen_utils.h"


template <class T>
class EigenParameterSet{
public:
    inline EigenParameterSet(){
        param_set_item_t param;
        param.name = "";
        param.dirty = false;
        param.value = nullptr;

        param_list.resize(1);
        param_list[0] = param;

        expected_num_params = 0;
        received_params = 0;
        t_last_update = current_time_ms();
        request_in_progress = false;
    }

    inline ~EigenParameterSet(){
        for(param_set_item_t item : param_list){
            delete item.value;
        }
        param_list.clear();
    }

    //void add(uint8_t id, T item, std::string name);
    inline void add(uint8_t id, T *item, std::string name){
        std::lock_guard<std::mutex> lock(mutex);

        param_set_item_t param;
        param.name = "";
        param.dirty = false;
        param.value = nullptr;

        if(id >= param_list.size()){
            //If the ID is past the end, resize to include it
            param_list.resize(id+1, param);
        }

        param.name = name;
        param.value = item;

        if(param_list[id].name == "")
            received_params++;

        if(param_list[id].value)
            delete param_list[id].value;

        //Update the param list
        param_list[id] = param;
        t_last_update = current_time_ms();
    }

    //T& ref(uint8_t param);
    /*inline T& ref(uint8_t param){
        std::lock_guard<std::mutex> lock(mutex);

        if(param >= param_list.size()) return param_list[0].value;
        return param_list[param].value;
    }*/

    //void set_last_update();
    inline void set_last_update(){
        std::lock_guard<std::mutex> lock(mutex);

        t_last_update = current_time_ms();
    }

    //void set_expected_parameters(uint8_t num_parameters);
    inline void set_expected_parameters(uint8_t num_parameters){
        std::lock_guard<std::mutex> lock(mutex);

        expected_num_params = num_parameters;
        t_last_update = current_time_ms();
    }

    //uint8_t parameters_left() const;
    inline uint8_t parameters_left() const{
        std::lock_guard<std::mutex> lock(mutex);

        //TODO: Some sort of error fixing here. If this is wrong, we need to re check everything
        if(received_params > expected_num_params) return 0;

        return expected_num_params - received_params;
    }

    inline uint8_t expected() const {
        return expected_num_params;
    }

    //uint64_t d_t_last_update() const;
    inline uint64_t d_t_last_update() const{
        return current_time_ms() - t_last_update;
    }

    inline void set_request_in_progress(bool value){
        request_in_progress = value;
    }

    inline bool update_required(){
        return (expected_num_params == 0 || parameters_left() > 0) && d_t_last_update() > PACKET_TIMEOUT && !request_in_progress;
    }

    //std::string name(uint8_t id) const;
    inline std::string name(uint8_t id) const{
        std::lock_guard<std::mutex> lock(mutex);
        if(id >= param_list.size()) return "ERR";

        std::string retval = param_list[id].name;
        return retval;
    }

    //T value(uint8_t param) const;
    inline T *value(uint8_t param) const{
        std::lock_guard<std::mutex> lock(mutex);

        if(param >= param_list.size()) return nullptr;
        return param_list[param].value;
    }

    inline std::vector<const T *> list() const{
        std::lock_guard<std::mutex> lock(mutex);

        std::vector<const T *> retval = {};
        for(auto &param : param_list){
            retval.emplace_back(param.value);
        }
        return retval;
    }

private:
    typedef struct{
        bool dirty;
        std::string name;
        T *value;
    } param_set_item_t;

    mutable std::mutex mutex;
    std::vector<param_set_item_t> param_list;
    uint64_t t_last_update;
    uint8_t expected_num_params;
    uint8_t received_params;
    bool request_in_progress;

};

/* Param Types */
#define PARAM_TYPE_UINT8        (1)
#define PARAM_TYPE_UINT16       (2)
#define PARAM_TYPE_UINT32       (3)
#define PARAM_TYPE_FLOAT        (4)
#define PARAM_TYPE_UINT64       (5)
#define PARAM_TYPE_DOUBLE       (6)
#define PARAM_TYPE_MAX          (_DOUBLE)

class EigenParameter{
public:
    EigenParameter(uint8_t type);
    EigenParameter();

    ~EigenParameter();

    bool update_value(std::string val);
    bool update_value(uint8_t val);
    bool update_value(uint16_t val);
    bool update_value(uint32_t val);
    bool update_value(uint64_t val);
    bool update_value(float val);
    bool update_value(double val);

    typedef union{
        uint8_t     uint8_;
        uint16_t    uint16_;
        uint32_t    uint32_;
        uint64_t    uint64_;
        float       float_;
        double      double_;
    } eigen_param_t;

    eigen_param_t value() const;
    uint8_t type() const;

    std::string print() const;
    std::string print_as(uint8_t type) const;

private:
    eigen_param_t value_;
    uint8_t type_;

};


/* Mailbox Type Hints */
#define MAILBOX_RW_MASK         (0x0F)
#define MAILBOX_READ_ONLY       (0x01)
#define MAILBOX_READ_WRITE      (0x02)

#define MAILBOX_TYPE_MASK       (0xF0)
#define MAILBOX_INT             (0x10)
#define MAILBOX_DOUBLE          (0x20)
#define MAILBOX_STRING          (0x40)

class EigenMailbox{
public:
    EigenMailbox(uint8_t type);
    EigenMailbox();

    ~EigenMailbox();

    bool update_value(std::string val);
    bool plottable() const;
    bool valid() const;

    std::string print() const;
    uint8_t type() const;
    double as_float() const;

private:
    std::string raw_value_;
    uint32_t parsed_int_;
    double parsed_double_;
    bool parse_valid;

    uint8_t type_hint_;
    uint8_t access_;
};

#endif // EIGEN_PARAMETERS_H
