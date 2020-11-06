#ifndef EIGEN_PARAMETERS_H
#define EIGEN_PARAMETERS_H

#include "eigen_utils.h"


template <class T>
class EigenParameterSet{
public:
    void add_param(uint8_t id, T item, std::string name);
    void update_param(uint8_t param, T item);
    T read_param(uint8_t param);

    void set_last_update();
    void set_expected_parameters(uint8_t num_parameters);

    uint8_t parameters_left() const;
    uint64_t d_t_last_update() const;
    std::string param_name(uint8_t id) const;

private:
    typedef struct{
        bool dirty;
        std::string name;
        T value;
    } param_t;

    mutable std::mutex mutex;
    std::vector<param_t> param_list;
    uint64_t t_last_update;
    uint8_t expected_num_params;
    uint8_t received_params;

};

/* Param Types */
#define _UINT8                  (1)
#define _UINT16                 (2)
#define _UINT32                 (3)
#define _FLOAT                  (4)
#define _UINT64                 (5)
#define _DOUBLE                 (6)
#define PARAM_TYPE_MAX          (_DOUBLE)

class EigenParameter{
public:
    EigenParameter(std::string printed);
    EigenParameter(uint8_t value);
    EigenParameter(uint16_t value);
    EigenParameter(uint32_t value);
    EigenParameter(uint64_t value);
    EigenParameter(float value);
    EigenParameter(double value);

    ~EigenParameter();

    static EigenParameter from_type(uint8_t type);

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

class EigenMailbox{

};

#endif // EIGEN_PARAMETERS_H
