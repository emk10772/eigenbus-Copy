/* eigen_variable.h
 *
 * This file holds the EigenVariable class. The EigenVariable class is
 * the backbone of the parsing and group value comparing infrastructure.
 *
 * The EigenVariable class provides a virtual interface that allows for
 * string-based keying, variable type / value comparing, plotting support,
 * cloning, parsing, and parent information.
 *
 * Created 2021 by Nick Paiva
 */

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

    /* Getters - const */
    std::string name() const;
    EigenVarType type() const;
    bool weak_match(const EigenVariable &variable) const;
    eigen_addr_t id() const;
    eigen_addr_t module_address() const;
    bool parent_info_valid() const;
    bool plottable() const;

    /* Setters */
    void set_id(eigen_addr_t id);
    void set_address(eigen_addr_t addr);

    /* Virtual functions */
    //Does this variable match in type and value to another
    virtual bool strong_match(const EigenVariable &variable) const = 0;
    //Print this variable in string form
    virtual std::string print() const = 0;
    //Parse an encoded string value
    virtual bool parse_value(std::string encoded) = 0;
    //Clone this variable
    virtual EigenVariable *clone() const = 0;
    //Plottable (floating point) value
    virtual double as_plottable() const;

protected:
    bool plottable_;            //Can it be put in a plot?
    bool addr_set_;             //Does it have a module it belongs to?
    bool id_set_;               //Is this part of a set where it has an ID?
    eigen_addr_t mod_address_;  //Address of the module it belongs to
    eigen_addr_t id_;           //ID in the set it belongs to
    const std::string name_;    //String key of this variable
    const EigenVarType type_;   //Type of this variable
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
