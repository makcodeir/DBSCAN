#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "csv_adapter.hpp"

class Person {
public:
    std::string name;
    int age = 0;
    double height = 0.0;
    bool active = false;
    std::string email;

    static std::shared_ptr<Person> fromRecord(const csvmap::Record& record) {
        auto person = std::make_shared<Person>();
        person->name   = record.at("name").get<std::string>();
        person->age    = record.at("age").get<int>();
        person->height = record.at("height").asDouble();
        person->active = record.at("active").get<bool>();
        const value& email = record.at("email");
        if (!email.isNull()) person->email = email.get<std::string>();
        return person;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Person& p) {
    os << "Person{name='" << p.name << "', age=" << p.age << ", height=" << p.height
       << ", active=" << (p.active ? "true" : "false") << ", email=";
    if (p.email.empty()) os << "null";
    else os << "'" << p.email << "'";
    return os << "}";
}
