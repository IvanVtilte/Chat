#pragma once

#include "User.h"
#include <string>
#include <iostream>

class Message {
public:
    User* sender;
    User* recipient;
    std::string content;

    // Конструктор
    Message(User* sender, User* recipient, const std::string& msg)
        : sender(sender), recipient(recipient), content(msg) {}

    // Вывод сообщения
    void display() const {
        std::cout << sender->username << " to " << recipient->username << ": " << content << std::endl;
    }
};

