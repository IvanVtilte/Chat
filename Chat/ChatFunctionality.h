#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cmath>
#include "User.h"
#include "Message.h"

// Простая хеш-функция методом умножения
size_t hashFunction(const std::string& key, size_t tableSize) {
    const double A = 0.6180339887; // Константа A (1 - золотое сечение)
    double fracPart = fmod(key.length() * A, 1.0); // Дробная часть
    return static_cast<size_t>(tableSize * fracPart);
}

class Chat {
private:
    struct HashEntry {
        std::string username;
        std::string passwordHash;
        bool occupied = false;
        bool deleted = false;
    };

    std::vector<HashEntry> hashTable;
    size_t userCount = 0;

    // Хеш-таблица увеличивается при превышении коэффициента заполнения
    void resizeTable() {
        size_t newSize = hashTable.size() * 2 + 1;
        std::vector<HashEntry> newTable(newSize);

        for (const auto& entry : hashTable) {
            if (entry.occupied && !entry.deleted) {
                size_t index = hashFunction(entry.username, newSize);
                size_t i = 0;

                while (newTable[index].occupied) {
                    index = (index + i * i) % newSize;
                    ++i;
                }

                newTable[index] = entry;
            }
        }

        hashTable = std::move(newTable);
    }

public:
    Chat(size_t initialSize = 11) : hashTable(initialSize) {}

    // Регистрация пользователя
    void registerUser(const std::string& username, const std::string& password, const std::string& name) {
        if (userCount >= hashTable.size() / 2) {
            resizeTable();
        }

        size_t index = hashFunction(username, hashTable.size());
        size_t i = 0;

        while (hashTable[index].occupied && !hashTable[index].deleted) {
            if (hashTable[index].username == username) {
                throw std::runtime_error("Пользователь с таким логином уже существует!");
            }
            index = (index + i * i) % hashTable.size();
            ++i;
        }

        hashTable[index] = { username, password, true, false };
        ++userCount;

        std::cout << "Пользователь " << name << " зарегистрирован." << std::endl;
    }

    // Авторизация пользователя
    User* authenticateUser(const std::string& username, const std::string& password) {
        size_t index = hashFunction(username, hashTable.size());
        size_t i = 0;

        while (hashTable[index].occupied || hashTable[index].deleted) {
            if (hashTable[index].occupied && !hashTable[index].deleted && hashTable[index].username == username) {
                if (hashTable[index].passwordHash == password) {
                    return new User(username, password, username);
                }
                else {
                    throw std::runtime_error("Неверный пароль!");
                }
            }
            index = (index + i * i) % hashTable.size();
            ++i;
        }

        throw std::runtime_error("Пользователь не найден!");
    }

    // Отображение всех пользователей (для отладки)
    void displayUsers() const {
        for (const auto& entry : hashTable) {
            if (entry.occupied && !entry.deleted) {
                std::cout << "Логин: " << entry.username << ", Хеш пароля: " << entry.passwordHash << std::endl;
            }
        }
    }
};
