#pragma once

#include "User.h"
#include "Message.h"
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <iostream>

class Chat {
private:
    std::unordered_map<std::string, User*> users;
    std::vector<Message> messages;

public:
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
        if (it != users.end() && it->second->password == password) {
            return it->second;
        }
        throw std::runtime_error("Неверный логин или пароль!");
    }

    // Отправка сообщения
    void sendMessage(User* sender, User* recipient, const std::string& content) {
        if (users.find(recipient->username) == users.end()) {
            throw std::runtime_error("Получатель не найден!");
        }
        messages.push_back(Message(sender, recipient, content));
    }
    //Отправка сообщений всем
    void sendMessageToAll(User* sender, const std::string& content) {
        for (auto& pair : users) {
            if (pair.second != sender) {
                messages.push_back(Message(sender, pair.second, content));
            }
        }
    }

    // Просмотр всех сообщений
    void displayMessages() const {
        if (messages.empty()) {
            std::cout << "Сообщений нет." << std::endl;
            return;
        }
        for (const auto& message : messages) {
            message.display();
        }
    }

    // Поиск пользователя по логину
    User* findUserByUsername(const std::string& username) {
        auto it = users.find(username);
        return (it != users.end()) ? it->second : nullptr;
    }
};

