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
        // 버퍼 초기화
        memset(buffer, 0, sizeof(buffer));

        // 서버로부터 메시지 수신
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        // 수신된 메시지 출력
        cout << buffer << "\n";

        if (bytesReceived > 0) {
            // 출력 동기화를 위한 뮤텍스 사용
            lock_guard<std::mutex> lock(coutMutex);
            receivedMessage = buffer;

            // "GAME END" 메시지 확인
            if (receivedMessage.find("GAME END") != string::npos) {
                cout << "=====================================" << endl;
                cout << "게임을 종료하려면 아무 키나 눌러주세요";
                cin >> receivedMessage;
                closesocket(clientSocket);
                WSACleanup();
                break;
            }

            // "GAME START" 메시지 확인
            if (strcmp(buffer, "GAME START") == 0) {
                cout << "=====================================" << endl;
                // 새로운 스레드에서 사용자 입력 처리
                thread userInputThread(getUserInput, clientSocket);
                userInputThread.detach();
            }
        }
    }
}

int main() {
    // WinSock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "winsock 초기화 실패\n";
        cout << "접속을 종료하려면 아무 키나 입력해주세요\n";
        cin >> a;
        WSACleanup();
        return -1;
    }

    // 소켓 생성
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        cout << "소켓 생성 오류\n";
        cout << "접속을 종료하려면 아무 키나 입력해주세요\n";
        cin >> a;
        WSACleanup();
        return -1;
    }

    // 서버 주소 설정
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(22222);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 서버에 연결
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        cout << "서버에 접속 실패\n";
        cout << "접속을 종료하려면 아무 키나 입력해주세요\n";
        cin >> a;
        closesocket(clientSocket);
        WSACleanup();
        return -1;
    }

    // 접속 성공 메시지 출력
    cout << "=======================================================================\n";
    cout << "========================      마피아 게임      ========================\n";
    cout << "=======================================================================\n";
    cout << "서버에 접속되었습니다.\n";
    cout << "서버에서 게임을 시작할 때까지 대기해 주세요\n";
    cout << "=======================================================================\n";

    // 메시지 수신을 처리하는 스레드 시작
    thread receiveThread(receiveMessages, clientSocket);

    // 메시지 수신 스레드 대기
    receiveThread.join();

    return 0;
}
