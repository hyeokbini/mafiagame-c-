#include "gamelogic.h"
#include <iostream>
#include <WinSock2.h>
#include <string>
#include <ctime>
#include <vector>
#include <mutex>
#include <thread>
#include <future>
#include <chrono>
#include <algorithm>

#pragma comment (lib, "ws2_32.lib")

using namespace std;

mutex userCountMutex;
bool gamestart = false;

int main() {
    // Winsock �ʱ�ȭ
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "winsock �ʱ�ȭ ����\n";
        WSACleanup();
        return -1;
    }

    // ���� ���� ����
    SOCKET serversocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serversocket == INVALID_SOCKET) {
        cout << "���� ���� ����\n";
        WSACleanup();
        return -1;
    }

    // ���� �ּ� ����
    sockaddr_in serveraddress;
    serveraddress.sin_family = AF_INET;
    serveraddress.sin_port = htons(22222);
    serveraddress.sin_addr.s_addr = INADDR_ANY;

    // ���ε�
    if (::bind(serversocket, (struct sockaddr*)&serveraddress, sizeof(serveraddress)) == SOCKET_ERROR) {
        cout << "���ε� ����\n";
        closesocket(serversocket);
        WSACleanup();
        return -1;
    }

    // ��� ���·� ����
    if (listen(serversocket, 10) == SOCKET_ERROR) {
        cout << "���� ������ ���� ����\n";
        closesocket(serversocket);
        WSACleanup();
        return -1;
    }

    cout << "=================================================\n";
    cout << "������ �����Ǿ����ϴ�. ������ ������ ��ٸ��ϴ�.\n";

    cout << "=================================================\n";
    cout << "������ �����Ϸ��� start�� �Է��ϼ���.\n";

    int usercount = 0;
    int usernum[10];

    // �Է��� �޴� ������
    thread inputThread([&] {
        while (true) {
            string inputCommand;
            cin >> inputCommand;
            if (inputCommand == "start") {
                {
                    // ���ؽ��� ����Ͽ� ���� ���� ���� üũ �� ���� ����
                    lock_guard<mutex> lock(userCountMutex);
                    if (!gamestart) {
                        cout << "==================\n������ ���۵˴ϴ�\n==================\n";
                        for (int i = 0; i < usercount; i++) {
                            send(usernum[i], "GAME START", strlen("GAME START"), 0);
                        }
                        gamestart = true;
                        gamelogicstart(usernum, usercount);
                        for (int i = 0; i < usercount; i++) {
                            send(usernum[i], "GAME END", strlen("GAME END"), 0);
                        }
                        cout << "���� ����. ������ �����÷��� �ƹ� Ű�� �����ּ���.\n===============================================\n";
                        cin >> inputCommand;
                        closesocket(serversocket);
                        WSACleanup();
                        return 0;
                    }
                }
            }
        }
        });

    while (!gamestart)
    {
        // ������ ������ ���
        SOCKET usersocket = accept(serversocket, nullptr, nullptr);
        if (usersocket == INVALID_SOCKET && !gamestart) {
            cout << "������ ������ �õ������� �����߽��ϴ�\n";
            closesocket(usersocket);
        }
        else {
            {
                // ���ؽ��� ����Ͽ� ���� �� ���� �� ���� ���
                lock_guard<mutex> lock(userCountMutex);
                if (gamestart) {
                    break;
                }
                usernum[usercount] = usersocket;
                usercount += 1;
                cout << "====================================\n���ο� ������ �����Ͽ����ϴ�\n";
                cout << "���� ��� ���� ���� ���� " << usercount << "�� �Դϴ�.\n";
            }
        }
        if (gamestart) {
            break;
        }
    }

    // �Է� ������ ���� ���
    inputThread.join();
}
