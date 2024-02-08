#include "gamelogic.h"
#include <iostream>
#include <WinSock2.h>
#include <string>
#include <cstring>
#include <thread>
#include <ctime>
#include <vector>
#include <mutex>
#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <future>
#include <functional>
#include <type_traits>

#pragma comment (lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS

using namespace std;

// 게임에 필요한 메세지들
const char* manmsg = "===================\n당신은 시민입니다.\n===================\n";
const char* polmsg = "===================\n당신은 경찰입니다.\n===================\n";
const char* mafmsg = "===================\n당신은 마피아입니다.\n===================\n";
const char* votemsg = "=====================================================================\n투표가 시작됩니다. 자신의 순서가 되면 유저 번호를 입력해 투표를 진행해 주세요.\n=====================================================================\n";
const char* espvotemsg = "번 유저님, 숫자를 입력해 투표해 주세요.기권표는 0을 입력해 주세요.\n=====================================================================";
const char* espvoteendmsg = "번 유저님이 투표를 완료했습니다.\n=================================\n";
const char* voteendmsg = "투표가 종료되었습니다.\n";
const char* bothscoremsg = "투표에서 동률이 나왔습니다. 아무도 죽지 않았습니다.\n";
const char* mafkillchoosemsg = "죽일 유저의 번호를 입력하세요...";
const char* polsearchchoosemsg = "조사할 유저의 번호를 입력하세요...";
const char* notmafia = "조사한 유저는 마피아가 아닙니다.\n";
const char* yesmafia = "조사한 유저는 마피아가 맞습니다.\n";
const char* daymsg = "===============\n낮이 되었습니다. 2분 후 투표가 시작됩니다.\n===============\n";
const char* nightmsg = "===============\n밤이 되었습니다.\n===============\n";
const char* byvotediemsg = "번이 투표로 죽었습니다.\n";
const char* bymafdiemsg = "번이 마피아에게 죽었습니다.\n";
const char* mafiadiemsg = "마피아가 죽었습니다. 시민들의 승리\n";
const char* mafwinmsg = "마피아의 승리\n";
const char* manwinmsg = "시민의 승리\n";
const char* gameendmsg = "게임 종료.";
const char* mafiasuicide = "마피아가 자살했습니다. 시민의 승리\n";
const char* noonediemsg = "아무도 죽지 않았습니다.\n";
const char* noticemem = "번 유저님 환영합니다.\n";
int mafia, police, man;
int mafiacount = 1, policecount = 1;
int daycount;
const int daylimittime = 120;
const int nightlimittime = 30;
char buffer[300];
mutex newmutex;

// 비동기적으로 채팅을 처리하는 함수
void asyncChat(int userIndex, int userSocket, int* userNum, int userCount, const vector<int>& userAlive, mutex& newmutex)
{
    // 채팅 시간 제한 설정
    const time_t start_time = time(nullptr);
    const time_t timeout = daylimittime;

    // 주어진 시간동안 루프 실행
    while (time(nullptr) - start_time < timeout)
    {
        // 주어진 시간 내에 루프를 벗어나기 위한 체크
        if (time(nullptr) - start_time > timeout)
        {
            break;
        }

        // 소켓의 상태를 감시하는 fd_set 초기화
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(userSocket, &readfds);

        // 소켓 대기 시간 설정
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        // 소켓에서 데이터를 수신하면 처리
        if (FD_ISSET(userSocket, &readfds))
        {
            char buffer[300];
            memset(buffer, 0, sizeof(buffer));
            int bytesReceived = recv(userSocket, buffer, sizeof(buffer), 0);

            // 데이터를 성공적으로 수신하고 사용자가 살아있을 때
            if (bytesReceived > 0 && userAlive[userIndex])
            {
                {
                    // 뮤텍스를 사용하여 출력 동기화
                    lock_guard<mutex> lock(newmutex);

                    // 메시지에 사용자 번호를 추가하여 전체 사용자에게 전송
                    string messageWithUserIndex = to_string(userIndex + 1) + "번 유저 : " + buffer;
                    for (int j = 0; j < userCount; j++)
                    {
                        if (j != userIndex && userAlive[j])
                        {
                            send(userNum[j], messageWithUserIndex.c_str(), strlen(messageWithUserIndex.c_str()), 0);
                        }
                    }
                }
            }
        }
    }
}



// 경찰의 야간 동작을 처리하는 함수
int nightpoliceaction(int policeindex, int* usernum, int usercount, const vector<int>& userAlive)
{
    int result = -1;

    // 경찰이 살아있을 때만 동작
    if (userAlive[policeindex] != 0)
    {
        memset(buffer, 0, sizeof(buffer));
        recv(usernum[policeindex], buffer, sizeof(buffer), 0);
        result = atoi(buffer);
    }
    else
    {
        result = -1;
    }
    return result;
}

// 마피아의 야간 동작을 처리하는 함수
int nightmafiaaction(int mafiaindex, int* usernum, int usercount)
{
    int result = -1;
    memset(buffer, 0, sizeof(buffer));
    recv(usernum[mafiaindex], buffer, sizeof(buffer), 0);
    result = atoi(buffer);
    return result;
}

//게임 로직
void gamelogicstart(int* usernum, int usercount)
{
    int alivecount = usercount;
    srand(time(NULL));
    mafia = rand() % usercount;
    police = (mafia + 1) % usercount;
    vector<int> useralive(usercount, 1);
    daycount = 0;
    vector<future<void>> chatThreads;
    while (1)
    {
        //낮 부분 로직 시작
        daycount++;
        if (daycount == 1)
        {
            for (int i = 0; i < usercount; i++)
            {
                send(usernum[i], (to_string(i + 1) + noticemem).c_str(), strlen((to_string(i + 1) + noticemem).c_str()), 0);
            }
            for (int i = 0; i < usercount; i++)
            {
                if (i == mafia)
                    send(usernum[i], mafmsg, strlen(mafmsg), 0);
                else if (i == police)
                    send(usernum[i], polmsg, strlen(polmsg), 0);
                else
                    send(usernum[i], manmsg, strlen(manmsg), 0);
            }
        }
        for (int i = 0; i < usercount; i++)
        {
            send(usernum[i], daymsg, strlen(daymsg)-2, 0);
        }
        
        chatThreads.clear();
        for (int i = 0; i < usercount; i++)
        {
            if (useralive[i] == 1)
            {
                chatThreads.push_back(async(launch::async, asyncChat, i, usernum[i], usernum, usercount, cref(useralive), ref(newmutex)));
            }
        }


        this_thread::sleep_for(chrono::seconds(daylimittime));


        //투표 로직 시작
        for (int i = 0; i < usercount; i++)
        {
            send(usernum[i], votemsg, strlen(votemsg), 0);
        }

        vector<int> votes(usercount, 0);

        for (int i = 0; i < usercount; i++)
        {
            for (int j = 0; j < usercount; j++)
            {
                if (useralive[i] == 1)
                {
                    send(usernum[j], (to_string(i + 1) + espvotemsg).c_str(), strlen((to_string(i + 1) + espvotemsg).c_str()), 0);
                }
            }
            if (useralive[i] == 1)
            {
                char voteresultbuffer[5];
                memset(voteresultbuffer, 0, sizeof(voteresultbuffer));
                recv(usernum[i], voteresultbuffer, sizeof(voteresultbuffer), 0);
                if (atoi(voteresultbuffer) != 0)
                {
                    votes[atoi(voteresultbuffer) - 1]++;
                }
            }
            for (int j = 0; j < usercount; j++)
            {
                if (useralive[i] == 1)
                {
                    send(usernum[j], (to_string(i + 1) + espvoteendmsg).c_str(), strlen((to_string(i + 1) + espvoteendmsg).c_str()), 0);
                }
            }
        }
        
        //투표 결과 처리 로직
        int max = 0;
        int votemaxvalue = 0;
        vector<int> maxCandidates;
        for (int i = 0; i < usercount; i++)
        {
            if (votes[i] > max)
            {
                max = votes[i];
                votemaxvalue = i;
                maxCandidates.clear();
            }
            if (votes[i] == max && max != 0)
            {
                maxCandidates.push_back(i);
            }
        }
        if (max == 0)
        {
            for (int i = 0; i < usercount; i++)
            {
                send(usernum[i], noonediemsg, strlen(noonediemsg), 0);
            }
        }
        else if (maxCandidates.size() > 1)
        {
            for (int i = 0; i < usercount; i++)
            {
                send(usernum[i], bothscoremsg, strlen(bothscoremsg), 0);
            }
        }
        //마피아가 투표로 사망한 경우 마피아의 승으로 게임 종료
        else if (votemaxvalue == mafia)
        {
            for (int i = 0; i < usercount; i++)
            {
                send(usernum[i], mafiadiemsg, strlen(mafiadiemsg), 0);
            }
            return;
        }
        else
        {
            for (int i = 0; i < usercount; i++)
            {
                send(usernum[i], (to_string(votemaxvalue + 1) + byvotediemsg).c_str(), strlen((to_string(votemaxvalue + 1) + byvotediemsg).c_str()), 0);
            }
            useralive[votemaxvalue] = 0;
            alivecount--;
        }
        //투표로 인해 유저가 사망했지만, 마피아가 생존 중인 경우 마피아의 승으로 게임 종료
        if (alivecount <= 2)
        {
            for (int i = 0; i < usercount; i++)
            {
                send(usernum[i], mafwinmsg, strlen(mafwinmsg), 0);
            }
            return;
        }
        for (int i = 0; i < usercount; i++)
        {
            send(usernum[i], nightmsg, strlen(nightmsg), 0);
        }
        
        
        //밤 부분 로직 시작
        send(usernum[mafia], mafkillchoosemsg, strlen(mafkillchoosemsg), 0);
        if (useralive[police] != 0)
        {
            send(usernum[police], polsearchchoosemsg, strlen(polsearchchoosemsg), 0);
        }

        int mafiaResult = -1;
        int policeResult = -1;

        future<int> mafiaThread = async(launch::async, nightmafiaaction, mafia, usernum, usercount);
        future<int> policeThread = async(launch::async, nightpoliceaction, police, usernum, usercount, cref(useralive));

        this_thread::sleep_for(chrono::seconds(nightlimittime));

        mafiaResult = mafiaThread.get();
        policeResult = policeThread.get();

        if (mafiaResult != -1)
        {
            //마피아가 자살한 경우 게임 종료
            if (mafiaResult - 1 == mafia)
            {
                for (int i = 0; i < usercount; i++)
                {
                    send(usernum[i], mafiasuicide, strlen(mafiasuicide), 0);
                }
                return;
            }
            for (int i = 0; i < usercount; i++)
            {
                send(usernum[i], (to_string(mafiaResult) + bymafdiemsg).c_str(), strlen((to_string(mafiaResult) + bymafdiemsg).c_str()), 0);
            }
            useralive[mafiaResult - 1] = 0;
            alivecount--;
        }

        if (policeResult != -1)
        {
            if (policeResult - 1 == mafia)
            {
                send(usernum[police], yesmafia, strlen(yesmafia), 0);
            }
            else
            {
                send(usernum[police], notmafia, strlen(notmafia), 0);
            }
        }
        //밤 부분 로직이 끝날 때, 마피아가 생존해 있고 남은 유저수가 2명이라면 마피아의 승리
        if (alivecount <= 2)
        {
            for (int i = 0; i < usercount; i++)
            {
                send(usernum[i], mafwinmsg, strlen(mafwinmsg), 0);
            }
            return;
        }
    }
    
}