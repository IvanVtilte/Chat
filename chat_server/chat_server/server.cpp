#include <winsock2.h>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <vector>
#include <thread>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/exception.h>
#include <iostream>
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
#define MAX_CLIENTS 10

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

std::vector<SOCKET> clients;
std::mutex clients_mutex;
Logger logger("server_log.txt");

void broadcast(const std::string& message, SOCKET sender) {
    std::lock_guard<std::mutex> guard(clients_mutex);
    for (SOCKET client : clients) {
        if (client != sender) {
            send(client, message.c_str(), message.size() + 1, 0);
        }
    }
}

void handle_client(SOCKET client_sock, sql::Connection* conn) {
    {
        std::lock_guard<std::mutex> guard(clients_mutex);
        clients.push_back(client_sock);
    }

    char buffer[1024];
    while (true) {
        int bytes = recv(client_sock, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;

        buffer[bytes] = '\0';
        std::string message(buffer);

        if (message.find("REGISTER|") == 0) {
            size_t pos = message.find('|', 9);
            if (pos != std::string::npos) {
                std::string login = message.substr(9, pos - 9);
                std::string password = message.substr(pos + 1);

                try {
                    sql::Statement* stmt = conn->createStatement();
                    stmt->execute("INSERT INTO users(user_name, user_password) VALUES('" +
                        login + "','" + password + "')");
                    send(client_sock, "OK", 2, 0);
                    logger.write_log("Зарегистрирован новый пользователь: " + login);
                    delete stmt;
                }
                catch (const sql::SQLException& e) {
                    logger.write_log("Ошибка регистрации: " + std::string(e.what()));
                    send(client_sock, "ERROR", 5, 0);
                }
            }
        }
        else {
            logger.write_log("Получено сообщение: " + message);
            broadcast(message, client_sock);
        }
    }

    {
        std::lock_guard<std::mutex> guard(clients_mutex);
        clients.erase(std::remove(clients.begin(), clients.end(), client_sock), clients.end());
    }
    closesocket(client_sock);
    logger.write_log("Клиент отключился");
}

int main() {
    SOCKET server_socket = INVALID_SOCKET;
    sql::Connection* conn = nullptr;
    std::vector<std::thread> client_threads;

    try {
        setlocale(LC_ALL, "ru");

        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            throw std::runtime_error("Ошибка инициализации Winsock");
        }

        try {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            conn = driver->connect("tcp://127.0.0.1:3306", "root", "13131");
            conn->setSchema("chatdb");
            logger.write_log("Подключение к базе данных успешно");
        }
        catch (const sql::SQLException& e) {
            throw std::runtime_error("Ошибка MySQL: " + std::string(e.what()));
        }

        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == INVALID_SOCKET) {
            throw std::runtime_error("Ошибка создания сокета");
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(PORT);

        if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            throw std::runtime_error("Ошибка привязки сокета");
        }

        if (listen(server_socket, MAX_CLIENTS) == SOCKET_ERROR) {
            throw std::runtime_error("Ошибка прослушивания порта");
        }

        logger.write_log("Сервер запущен. Ожидание подключений...");
        std::cout << "Сервер запущен. Ожидание подключений..." << std::endl;

        while (true) {
            SOCKET client_socket = accept(server_socket, NULL, NULL);
            if (client_socket == INVALID_SOCKET) {
                continue;
            }

            logger.write_log("Новое подключение клиента");
            client_threads.emplace_back(handle_client, client_socket, conn);
        }
    }
    catch (const std::exception& e) {
        logger.write_log("Критическая ошибка: " + std::string(e.what()));
        std::cerr << "Критическая ошибка: " << e.what() << std::endl;

        if (server_socket != INVALID_SOCKET) {
            closesocket(server_socket);
        }
        if (conn) {
            delete conn;
        }

        for (auto& thread : client_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        WSACleanup();
        return 1;
    }

    if (server_socket != INVALID_SOCKET) {
        closesocket(server_socket);
    }
    if (conn) {
        delete conn;
    }
    WSACleanup();
    return 0;
}
