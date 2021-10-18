#ifndef EIGENVARIABLE_H
#define EIGENVARIABLE_H

#include <string>
#include <vector>
#include <unordered_map>
#include "eigen_utils.h"

typedef enum{
    EIGEN_UINT8,
    EIGEN_UINT16,
    EIGEN_UINT32,
    EIGEN_UINT64,
    EIGEN_DOUBLE,
    EIGEN_STRING
} EigenVarType;

class EigenVariable {
public:
    EigenVariable(std::string name, EigenVarType type);

    std::string name() const;
    EigenVarType type() const;
    bool weak_match(const EigenVariable &variable) const;
    eigen_addr_t id() const;
    eigen_addr_t module_address() const;
    bool parent_info_valid() const;
    bool plottable() const;
    void set_id(eigen_addr_t id);
    void set_address(eigen_addr_t addr);

    virtual bool strong_match(const EigenVariable &variable) const = 0;
    virtual std::string print() const = 0;
    virtual bool parse_value(std::string encoded) = 0;
    virtual EigenVariable *clone() const = 0;
    virtual double as_plottable() const;

protected:
    bool plottable_;
    bool addr_set_;
    bool id_set_;
    eigen_addr_t mod_address_;
    eigen_addr_t id_;
    const std::string name_;
    const EigenVarType type_;
};

class EigenUint8 : public EigenVariable{
public:
    EigenUint8(std::string name, uint8_t value = 0);

    bool strong_match(const EigenVariable &variable) const;
    std::string print() const;
    bool parse_value(std::string encoded);
    EigenVariable *clone() const;
    double as_plottable() const;

    operator uint8_t() const {
        return value_;
    }
    void operator = (const uint8_t &value ) {
        value_ = value;
    }

private:
    uint8_t value_;
};

class EigenUint16 : public EigenVariable{
public:
    EigenUint16(std::string name, uint16_t value = 0);

    bool strong_match(const EigenVariable &variable) const;
    std::string print() const;
    bool parse_value(std::string encoded);
    EigenVariable *clone() const;
    double as_plottable() const;

    operator uint16_t() const {
        return value_;
    }
    void operator = (const uint16_t &value ) {
        value_ = value;
    }

private:
    uint16_t value_;
};

class EigenUint32 : public EigenVariable{
public:
    EigenUint32(std::string name, uint32_t value = 0);

    bool strong_match(const EigenVariable &variable) const;
    std::string print() const;
    bool parse_value(std::string encoded);
    EigenVariable *clone() const;
    double as_plottable() const;

    operator uint32_t() const {
        return value_;
    }
    void operator = (const uint32_t &value ) {
        value_ = value;
    }

private:
    uint32_t value_;
};

class EigenUint64 : public EigenVariable{
public:
    EigenUint64(std::string name, uint64_t value = 0);

    bool strong_match(const EigenVariable &variable) const;
    std::string print() const;
    bool parse_value(std::string encoded);
    EigenVariable *clone() const;
    double as_plottable() const;

    operator uint64_t() const {
        return value_;
    }
    void operator = (const uint64_t &value ) {
        value_ = value;
    }

private:
    uint64_t value_;
};

class EigenDouble : public EigenVariable{
public:
    EigenDouble(std::string name, double value = 0.0);

    bool strong_match(const EigenVariable &variable) const;
    std::string print() const;
    bool parse_value(std::string encoded);
    EigenVariable *clone() const;
    double as_plottable() const;

    operator double() const {
        return value_;
    }
    void operator = (const double &value ) {
        value_ = value;
    }

private:
    double value_;
};

class EigenString : public EigenVariable{
public:
    EigenString(std::string name, std::string value = "");

    bool strong_match(const EigenVariable &variable) const;
    std::string print() const;
    bool parse_value(std::string encoded);
    EigenVariable *clone() const;

    operator std::string() const {
        return value_;
    }
    void operator = (const std::string &value ) {
        value_ = value;
    }

private:
    std::string value_;
};

#endif // EIGENVARIABLE_H
