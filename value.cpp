#include <variant>
#include <string>
#include <iostream>
#include <stdexcept>

class value {
public:
    using Storage = std::variant
    <
    std::monostate,
    bool,
    int,
    double,
    char,
    std::string
    >;

    value() : data(std::monostate{}) {}
    value(bool v) : data(v) {}
    value(int v)  : data(v) {}
    value(double v) : data(v) {}
    value(char v) : data(v) {}

    value(const std::string& v)
                  : data(v) {}

    value(const char* v)
                  : data(std::string(v)) {}


    bool isNull()   const {return std::holds_alternative<std::monostate>(data);}
    bool isBool()   const {return std::holds_alternative<bool>(data);}
    bool isInt()    const {return std::holds_alternative<int>(data);}
    bool isDouble() const {return std::holds_alternative<double>(data);}
    bool isChar()   const {return std::holds_alternative<char>(data);}
    bool isString() const {return std::holds_alternative<std::string>(data);}

    template <typename T>
    T get() const {return std::get<T>(data) ;}


    double asDouble() const{
        if(isInt())     return static_cast<double>(std::get<int>(data));
        if(isDouble())  return std::get<double>(data);
        throw std::bad_variant_access();


                            }

    std::string toString() const {
        if (isNull())   return "null";
        if (isBool())   return std::get<bool>(data) ? "true" : "false";
        if (isInt())    return std::to_string(std::get<int>(data));
        if (isDouble()) return std::to_string(std::get<double>(data));
        if (isChar())   return std::string(1, std::get<char>(data));
        return std::get<std::string>(data);
                                  }

    friend std::ostream& operator<<(std::ostream& os, const value& v) {
        return os << v.toString();
    }

private:
    Storage data;               

    };

