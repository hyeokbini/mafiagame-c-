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
    // Winsock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "winsock 초기화 실패\n";
        WSACleanup();
        return -1;
    }

    // 서버 소켓 생성
    SOCKET serversocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serversocket == INVALID_SOCKET) {
        cout << "소켓 생성 오류\n";
        WSACleanup();
        return -1;
    }

    // 서버 주소 설정
    sockaddr_in serveraddress;
    serveraddress.sin_family = AF_INET;
    serveraddress.sin_port = htons(22222);
    serveraddress.sin_addr.s_addr = INADDR_ANY;

    // 바인딩
    if (::bind(serversocket, (struct sockaddr*)&serveraddress, sizeof(serveraddress)) == SOCKET_ERROR) {
        cout << "바인딩 에러\n";
        closesocket(serversocket);
        WSACleanup();
        return -1;
    }

    // 대기 상태로 설정
    if (listen(serversocket, 10) == SOCKET_ERROR) {
        cout << "서버 대기상태 설정 에러\n";
        closesocket(serversocket);
        WSACleanup();
        return -1;
    }

    cout << "=================================================\n";
    cout << "서버가 생성되었습니다. 유저의 접속을 기다립니다.\n";

    cout << "=================================================\n";
    cout << "게임을 시작하려면 start를 입력하세요.\n";

    int usercount = 0;
    int usernum[10];

    // 입력을 받는 스레드
    thread inputThread([&] {
        while (true) {
            string inputCommand;
            cin >> inputCommand;
            if (inputCommand == "start") {
                {
                    // 뮤텍스를 사용하여 게임 시작 상태 체크 및 게임 시작
                    lock_guard<mutex> lock(userCountMutex);
                    if (!gamestart) {
                        cout << "==================\n게임이 시작됩니다\n==================\n";
                        for (int i = 0; i < usercount; i++) {
                            send(usernum[i], "GAME START", strlen("GAME START"), 0);
                        }
                        gamestart = true;
                        gamelogicstart(usernum, usercount);
                        for (int i = 0; i < usercount; i++) {
                            send(usernum[i], "GAME END", strlen("GAME END"), 0);
                        }
                        cout << "게임 종료. 서버를 닫으시려면 아무 키나 눌러주세요.\n===============================================\n";
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
        // 유저의 접속을 대기
        SOCKET usersocket = accept(serversocket, nullptr, nullptr);
        if (usersocket == INVALID_SOCKET && !gamestart) {
            cout << "유저가 접속을 시도했으나 실패했습니다\n";
            closesocket(usersocket);
        }
        else {
            {
                // 뮤텍스를 사용하여 유저 수 갱신 및 정보 출력
                lock_guard<mutex> lock(userCountMutex);
                if (gamestart) {
                    break;
                }
                usernum[usercount] = usersocket;
                usercount += 1;
                cout << "====================================\n새로운 유저가 접속하였습니다\n";
                cout << "현재 대기 중인 유저 수는 " << usercount << "명 입니다.\n";
            }
        }
        if (gamestart) {
            break;
        }
    }

    // 입력 스레드 종료 대기
    inputThread.join();
}
