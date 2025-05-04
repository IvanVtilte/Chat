#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <string>
#include <winsock2.h>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <thread>
#include <vector>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/exception.h>
#include <stdexcept>
#include <mutex>
#include <fstream>
#include <shared_mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mysqlcppconn")

#define PORT 12345
#define BUFFER_SIZE 1024

class Logger {
private:
    std::fstream log_file;
    mutable std::shared_mutex file_mutex;

    std::string get_current_time() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
        return ss.str();
    }

public:
    Logger(const std::string& filename) {
        log_file.open(filename, std::ios::in | std::ios::out | std::ios::app);
        if (!log_file.is_open()) {
            throw std::runtime_error("Не удалось открыть файл логов");
        }
    }

    ~Logger() {
        std::unique_lock<std::shared_mutex> lock(file_mutex);
        if (log_file.is_open()) {
            log_file.close();
        }
    }

    void write_log(const std::string& message) {
        std::unique_lock<std::shared_mutex> lock(file_mutex);
        if (log_file.is_open()) {
            log_file << "[" << get_current_time() << "] " << message << std::endl;
        }
    }

    std::string read_log() {
        std::string line;
        std::shared_lock<std::shared_mutex> lock(file_mutex);
        if (log_file.is_open()) {
            log_file.seekg(0, std::ios::beg);
            std::getline(log_file, line);
        }
        return line;
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

Logger logger("client_log.txt");

void receive_messages(SOCKET sock) {
    char buffer[BUFFER_SIZE];
    while (true) {
        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';
        std::string message = "Получено: " + std::string(buffer);
        std::cout << message << std::endl;
        logger.write_log(message);
    }
}

int main() {
    setlocale(LC_ALL, "ru");

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        logger.write_log("Ошибка инициализации Winsock");
        std::cerr << "Ошибка инициализации Winsock" << std::endl;
        return 1;
    }

    sql::mysql::MySQL_Driver* mysql_driver;
    sql::Connection* mysql_conn;
    try {
        mysql_driver = sql::mysql::get_mysql_driver_instance();
        mysql_conn = mysql_driver->connect("tcp://127.0.0.1:3306", "root", "13131");
        mysql_conn->setSchema("chatdb");
        logger.write_log("Подключение к базе данных успешно");
    }
    catch (sql::SQLException& e) {
        logger.write_log("Ошибка MySQL: " + std::string(e.what()));
        std::cerr << "Ошибка MySQL: " << e.what() << std::endl;
        WSACleanup();
        return 1;
    }

    SOCKET reg_sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(reg_sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        logger.write_log("Ошибка подключения к серверу");
        std::cerr << "Ошибка подключения к серверу" << std::endl;
        delete mysql_conn;
        WSACleanup();
        return 1;
    }

    SOCKET chat_sock = INVALID_SOCKET;
    std::string current_user;

    while (true) {
        std::cout << "\n1. Регистрация\n2. Вход\n3. Отправить сообщение\n4. Просмотреть логи\n0. Выход\nВыберите: ";
        int choice;
        std::cin >> choice;
        std::cin.ignore();

        if (choice == 0) break;

        std::string login, password;
        if (choice == 1 || choice == 2) {
            std::cout << "Логин: ";
            std::getline(std::cin, login);
            std::cout << "Пароль: ";
            std::getline(std::cin, password);
        }

        if (choice == 1) {
            std::string data = "REGISTER|" + login + "|" + password;
            send(reg_sock, data.c_str(), data.size() + 1, 0);
            logger.write_log("Попытка регистрации пользователя: " + login);

            char response[BUFFER_SIZE];
            recv(reg_sock, response, BUFFER_SIZE, 0);
            std::string response_msg = "Ответ сервера: " + std::string(response);
            std::cout << response_msg << std::endl;
            logger.write_log(response_msg);
        }
        else if (choice == 2) {
            sql::Statement* stmt = mysql_conn->createStatement();
            sql::ResultSet* res = stmt->executeQuery(
                "SELECT 1 FROM users WHERE user_name='" + login +
                "' AND user_password='" + password + "'");

            if (res->next()) {
                std::string success_msg = "Успешный вход пользователя: " + login;
                std::cout << success_msg << std::endl;
                logger.write_log(success_msg);
                current_user = login;

                chat_sock = socket(AF_INET, SOCK_STREAM, 0);
                if (connect(chat_sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
                    logger.write_log("Ошибка подключения к чату");
                    std::cerr << "Ошибка подключения к чату" << std::endl;
                    chat_sock = INVALID_SOCKET;
                }
                else {
                    logger.write_log("Успешное подключение к чату");
                    std::thread(receive_messages, chat_sock).detach();
                }
            }
            else {
                std::string error_msg = "Ошибка входа: неверный логин или пароль";
                std::cout << error_msg << std::endl;
                logger.write_log(error_msg);
            }

            delete res;
            delete stmt;
        }
        else if (choice == 3) {
            if (chat_sock == INVALID_SOCKET) {
                std::cout << "Сначала войдите в систему" << std::endl;
                continue;
            }

            std::string message;
            std::cout << "Сообщение: ";
            std::getline(std::cin, message);
            message = current_user + ": " + message;
            send(chat_sock, message.c_str(), message.size() + 1, 0);
            logger.write_log("Отправлено сообщение: " + message);
        }
        else if (choice == 4) {
            std::cout << "\n=== Логи клиента ===" << std::endl;
            std::ifstream log_file("client_log.txt");
            std::string line;
            while (std::getline(log_file, line)) {
                std::cout << line << std::endl;
            }
            std::cout << "===================" << std::endl;
        }
    }

    logger.write_log("Завершение работы клиента");
    delete mysql_conn;
    closesocket(reg_sock);
    if (chat_sock != INVALID_SOCKET) {
        closesocket(chat_sock);
    }
    WSACleanup();
    return 0;
}
