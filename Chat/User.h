#pragma once

#include <string>
#include <iostream>

class User {
public:
    std::string username;
    std::string password;
    std::string name;
    User(std::string uname, std::string pass, std::string fullname)
        : username(uname), password(pass), name(fullname) {
    }
    // Вывод информации о пользователе
    void display() const {
        std::cout << name << " (" << username << ")" << std::endl;
    }
};
