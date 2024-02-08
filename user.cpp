#include <iostream>
#include <WinSock2.h>
#include <thread>
#include <mutex>
#include <string>

#pragma comment (lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS

using namespace std;

int a;

mutex coutMutex;

void getUserInput(SOCKET clientSocket) {
    string userInput;
    while (true) {
        getline(cin, userInput);
        send(clientSocket, userInput.c_str(), userInput.size(), 0);
    }
}

void receiveMessages(SOCKET clientSocket) {
    char buffer[1024];
    string receivedMessage;
    while (true) {
        // ���� �ʱ�ȭ
        memset(buffer, 0, sizeof(buffer));

        // �����κ��� �޽��� ����
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        // ���ŵ� �޽��� ���
        cout << buffer << "\n";

        if (bytesReceived > 0) {
            // ��� ����ȭ�� ���� ���ؽ� ���
            lock_guard<std::mutex> lock(coutMutex);
            receivedMessage = buffer;

            // "GAME END" �޽��� Ȯ��
            if (receivedMessage.find("GAME END") != string::npos) {
                cout << "=====================================" << endl;
                cout << "������ �����Ϸ��� �ƹ� Ű�� �����ּ���";
                cin >> receivedMessage;
                closesocket(clientSocket);
                WSACleanup();
                break;
            }

            // "GAME START" �޽��� Ȯ��
            if (strcmp(buffer, "GAME START") == 0) {
                cout << "=====================================" << endl;
                // ���ο� �����忡�� ����� �Է� ó��
                thread userInputThread(getUserInput, clientSocket);
                userInputThread.detach();
            }
        }
    }
}

int main() {
    // WinSock �ʱ�ȭ
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "winsock �ʱ�ȭ ����\n";
        cout << "������ �����Ϸ��� �ƹ� Ű�� �Է����ּ���\n";
        cin >> a;
        WSACleanup();
        return -1;
    }

    // ���� ����
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        cout << "���� ���� ����\n";
        cout << "������ �����Ϸ��� �ƹ� Ű�� �Է����ּ���\n";
        cin >> a;
        WSACleanup();
        return -1;
    }

    // ���� �ּ� ����
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(22222);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    // ������ ����
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cout << "������ ���� ����\n";
        cout << "������ �����Ϸ��� �ƹ� Ű�� �Է����ּ���\n";
        cin >> a;
        closesocket(clientSocket);
        WSACleanup();
        return -1;
    }

    // ���� ���� �޽��� ���
    cout << "=======================================================================\n";
    cout << "========================      ���Ǿ� ����      ========================\n";
    cout << "=======================================================================\n";
    cout << "������ ���ӵǾ����ϴ�.\n";
    cout << "�������� ������ ������ ������ ����� �ּ���\n";
    cout << "=======================================================================\n";

    // �޽��� ������ ó���ϴ� ������ ����
    thread receiveThread(receiveMessages, clientSocket);

    // �޽��� ���� ������ ���
    receiveThread.join();

    return 0;
}
