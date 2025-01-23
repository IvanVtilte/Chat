#include <iostream>
#include <string>
#include "User.h"
#include "Message.h"
#include "ChatFunctionality.h"

int main() {
    setlocale(LC_ALL, "ru");
    Chat chat;
    User* currentUser = nullptr;

    while (true) {
        std::cout << "\n1. Регистрация\n2. Вход\n3. Просмотр пользователей\n4. Выход\n";
        std::cout << "Выберите действие: ";
        int choice;
        std::cin >> choice;
        std::cin.ignore(); // Чтобы очистить буфер ввода после cin >> choice

        switch (choice) {
        case 1: {  // Регистрация
            std::string username, password, name;
            std::cout << "Введите логин: ";
            std::cin >> username;
            std::cout << "Введите пароль: ";
            std::cin >> password;
            std::cout << "Введите ваше имя: ";
            std::cin.ignore();  // Чтобы очистить буфер после cin >> password
            std::getline(std::cin, name);

            try {
                chat.registerUser(username, password, name);
            }
            catch (const std::runtime_error& e) {
                std::cout << "Ошибка: " << e.what() << std::endl;
            }
            break;
        }

        case 2: {  // Вход
            std::string username, password;
            std::cout << "Введите логин: ";
            std::cin >> username;
            std::cout << "Введите пароль: ";
            std::cin >> password;

            try {
                currentUser = chat.authenticateUser(username, password);
                std::cout << "Добро пожаловать, " << currentUser->name << "!" << std::endl;
            }
            catch (const std::runtime_error& e) {
                std::cout << "Ошибка: " << e.what() << std::endl;
            }
            break;
        }

        case 3: {  // Просмотр всех пользователей (для отладки)
            chat.displayUsers();
            break;
        }

        case 4: {  // Выход
            std::cout << "Выход из программы...\n";
            return 0;
        }

        default:
            std::cout << "Неверный выбор! Попробуйте снова.\n";
        }
    }

    return 0;
}
