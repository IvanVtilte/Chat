#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <string>
#include <winsock2.h>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mysqlcppconn")

#define PORT 12345
#define BUFFER_SIZE 1024

using namespace std;

void receive_messages(SOCKET sock) {
    char buffer[BUFFER_SIZE];
    while (true) {
        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';
        cout << "��������: " << buffer << endl;
    }
}

int main() {
    setlocale(LC_ALL, "ru");

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cerr << "������ ������������� Winsock" << endl;
        return 1;
    }

    sql::mysql::MySQL_Driver* mysql_driver;
    sql::Connection* mysql_conn;
    try {
        mysql_driver = sql::mysql::get_mysql_driver_instance();
        mysql_conn = mysql_driver->connect("tcp://127.0.0.1:3306", "root", "13131");
        mysql_conn->setSchema("chatdb");
    }
    catch (sql::SQLException& e) {
        cerr << "������ MySQL: " << e.what() << endl;
        WSACleanup();
        return 1;
    }

    SOCKET reg_sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(reg_sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "������ ����������� � �������" << endl;
        delete mysql_conn;
        WSACleanup();
        return 1;
    }

    SOCKET chat_sock = INVALID_SOCKET;
    string current_user;

    while (true) {
        cout << "\n1. �����������\n2. ����\n3. ��������� ���������\n0. �����\n��������: ";
        int choice;
        cin >> choice;
        cin.ignore();

        if (choice == 0) break;

        string login, password;
        if (choice == 1 || choice == 2) {
            cout << "�����: ";
            getline(cin, login);
            cout << "������: ";
            getline(cin, password);
        }

        if (choice == 1) {
            string data = "REGISTER|" + login + "|" + password;
            send(reg_sock, data.c_str(), data.size() + 1, 0);

            char response[BUFFER_SIZE];
            recv(reg_sock, response, BUFFER_SIZE, 0);
            cout << "����� �������: " << response << endl;
        }
        else if (choice == 2) {
            sql::Statement* stmt = mysql_conn->createStatement();
            sql::ResultSet* res = stmt->executeQuery(
                "SELECT 1 FROM users WHERE user_name='" + login +
                "' AND user_password='" + password + "'");

            if (res->next()) {
                cout << "�������� ����!" << endl;
                current_user = login;

                chat_sock = socket(AF_INET, SOCK_STREAM, 0);
                if (connect(chat_sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
                    cerr << "������ ����������� � ����" << endl;
                    chat_sock = INVALID_SOCKET;
                }
                else {
                    thread(receive_messages, chat_sock).detach();
                }
            }
            else {
                cout << "������: �������� ����� ��� ������" << endl;
            }

            delete res;
            delete stmt;
        }
        else if (choice == 3) {
            if (chat_sock == INVALID_SOCKET) {
                cout << "������� ������� � �������" << endl;
                continue;
            }

            string message;
            cout << "���������: ";
            getline(cin, message);
            message = current_user + ": " + message;
            send(chat_sock, message.c_str(), message.size() + 1, 0);
        }
    }

    delete mysql_conn;
    closesocket(reg_sock);
    if (chat_sock != INVALID_SOCKET) {
        closesocket(chat_sock);
    }
    WSACleanup();
    return 0;
}