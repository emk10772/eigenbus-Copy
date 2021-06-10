#ifndef EIGENVARIABLE_H
#define EIGENVARIABLE_H

#include <string>
#include <vector>
#include <unordered_map>

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
    virtual bool strong_match(const EigenVariable &variable) const = 0;
    virtual std::string print() const = 0;

private:
    const std::string name_;
    const EigenVarType type_;
};

class EigenUint8 : public EigenVariable{
public:
    EigenUint8(std::string name, uint8_t value = 0);

    bool strong_match(const EigenVariable &variable) const;
    std::string print() const;

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

    operator std::string() const {
        return value_;
    }
    void operator = (const std::string &value ) {
        value_ = value;
    }

private:
    std::string value_;
};

/* EigenVariableGroup
    A strict observer of EigenVariables. Used to look for common values between variables.
    Must ensure that this class does not outlive its variables
*/
class EigenVariableGroup {
public:
    EigenVariableGroup();

    void add_variable(const EigenVariable *variable);
    void add_variables(const std::vector<const EigenVariable *> variables);

    std::string print_variable(const std::string key) const;
    std::string print_variable_list(const std::string key) const;
    const std::vector<std::string> common_keys() const;
private:
    std::unordered_multimap<std::string, const EigenVariable *> variable_map_;
    std::vector<std::string> common_keys_;
};

#endif // EIGENVARIABLE_H
