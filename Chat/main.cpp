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
        std::cout << "\n1. Регистрация\n2. Вход\n3. Отправить сообщение\n4. Отправить всем\n5. Просмотр сообщений\n6. Выход\n";
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

        case 3: {  // Отправить сообщение конкретному пользователю
            if (!currentUser) {
                std::cout << "Пожалуйста, войдите в систему, чтобы отправлять сообщения." << std::endl;
                break;
            }

            std::string recipientName, content;
            std::cout << "Введите логин получателя: ";
            std::cin >> recipientName;
            std::cout << "Введите сообщение: ";
            std::cin.ignore();  // Чтобы очистить буфер после cin >> recipientName
            std::getline(std::cin, content);

            User* recipient = chat.findUserByUsername(recipientName);
            if (!recipient) {
                std::cout << "Пользователь с таким логином не найден." << std::endl;
                break;
            }

            try {
                chat.sendMessage(currentUser, recipient, content);
                std::cout << "Сообщение отправлено!" << std::endl;
            }
            catch (const std::runtime_error& e) {
                std::cout << "Ошибка: " << e.what() << std::endl;
            }
            break;
        }

        case 4: {  // Отправить сообщение всем пользователям
            if (!currentUser) {
                std::cout << "Пожалуйста, войдите в систему, чтобы отправлять сообщения." << std::endl;
                break;
            }

            std::string content;
            std::cout << "Введите сообщение, которое будет отправлено всем пользователям: ";
            std::cin.ignore();  // Чтобы очистить буфер после cin
            std::getline(std::cin, content);

            try {
                chat.sendMessageToAll(currentUser, content);
                std::cout << "Сообщение отправлено всем пользователям!" << std::endl;
            }
            catch (const std::runtime_error& e) {
                std::cout << "Ошибка: " << e.what() << std::endl;
            }
            break;
        }

        case 5: {  // Просмотр сообщений
            if (!currentUser) {
                std::cout << "Пожалуйста, войдите в систему, чтобы просматривать сообщения." << std::endl;
                break;
            }

            std::cout << "Все сообщения: " << std::endl;
            chat.displayMessages();
            break;
        }

        case 6: {  // Выход
            std::cout << "Выход из программы...\n";
            return 0;
        }

        default:
            std::cout << "Неверный выбор! Попробуйте снова.\n";
        }
    }

    return 0;
}
