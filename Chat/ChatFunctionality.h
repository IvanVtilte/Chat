#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include "User.h"
#include "Message.h"

class Chat {
private:
    std::unordered_map<std::string, User*> users;  // Хранение пользователей по логину
    std::vector<Message> messages;  // Все сообщения в чате

public:
    Chat() {}

    // Регистрация пользователя
    void registerUser(const std::string& username, const std::string& password, const std::string& name) {
        if (users.find(username) != users.end()) {
            throw std::runtime_error("Пользователь с таким логином уже существует!");
        }

        users[username] = new User(username, password, name);
        std::cout << "Пользователь " << name << " зарегистрирован." << std::endl;
    }

    // Авторизация пользователя
    User* authenticateUser(const std::string& username, const std::string& password) {
        auto it = users.find(username);
        if (it == users.end()) {
            throw std::runtime_error("Пользователь не найден!");
        }

        User* user = it->second;
        if (user->password != password) {
            throw std::runtime_error("Неверный пароль!");
        }

        return user;
    }

    // Отправка сообщения
    void sendMessage(User* sender, User* recipient, const std::string& content) {
        if (sender == recipient) {
            throw std::runtime_error("Нельзя отправить сообщение самому себе!");
        }

        messages.push_back(Message(sender, recipient, content));
        std::cout << "Сообщение отправлено!" << std::endl;
    }

    // Просмотр всех сообщений
    void displayMessages() const {
        for (const auto& message : messages) {
            message.display();
        }
    }

    // Отображение всех пользователей (для отладки)
    void displayUsers() const {
        for (const auto& entry : users) {
            std::cout << "Пользователь: " << entry.second->name << " (" << entry.first << ")" << std::endl;
        }
    }

    ~Chat() {
        // Освобождение памяти для пользователей
        for (auto& entry : users) {
            delete entry.second;
        }
    }
};
