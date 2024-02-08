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

// ���ӿ� �ʿ��� �޼�����
const char* manmsg = "===================\n����� �ù��Դϴ�.\n===================\n";
const char* polmsg = "===================\n����� �����Դϴ�.\n===================\n";
const char* mafmsg = "===================\n����� ���Ǿ��Դϴ�.\n===================\n";
const char* votemsg = "=====================================================================\n��ǥ�� ���۵˴ϴ�. �ڽ��� ������ �Ǹ� ���� ��ȣ�� �Է��� ��ǥ�� ������ �ּ���.\n=====================================================================\n";
const char* espvotemsg = "�� ������, ���ڸ� �Է��� ��ǥ�� �ּ���.���ǥ�� 0�� �Է��� �ּ���.\n=====================================================================";
const char* espvoteendmsg = "�� �������� ��ǥ�� �Ϸ��߽��ϴ�.\n=================================\n";
const char* voteendmsg = "��ǥ�� ����Ǿ����ϴ�.\n";
const char* bothscoremsg = "��ǥ���� ������ ���Խ��ϴ�. �ƹ��� ���� �ʾҽ��ϴ�.\n";
const char* mafkillchoosemsg = "���� ������ ��ȣ�� �Է��ϼ���...";
const char* polsearchchoosemsg = "������ ������ ��ȣ�� �Է��ϼ���...";
const char* notmafia = "������ ������ ���Ǿư� �ƴմϴ�.\n";
const char* yesmafia = "������ ������ ���Ǿư� �½��ϴ�.\n";
const char* daymsg = "===============\n���� �Ǿ����ϴ�. 2�� �� ��ǥ�� ���۵˴ϴ�.\n===============\n";
const char* nightmsg = "===============\n���� �Ǿ����ϴ�.\n===============\n";
const char* byvotediemsg = "���� ��ǥ�� �׾����ϴ�.\n";
const char* bymafdiemsg = "���� ���Ǿƿ��� �׾����ϴ�.\n";
const char* mafiadiemsg = "���Ǿư� �׾����ϴ�. �ùε��� �¸�\n";
const char* mafwinmsg = "���Ǿ��� �¸�\n";
const char* manwinmsg = "�ù��� �¸�\n";
const char* gameendmsg = "���� ����.";
const char* mafiasuicide = "���Ǿư� �ڻ��߽��ϴ�. �ù��� �¸�\n";
const char* noonediemsg = "�ƹ��� ���� �ʾҽ��ϴ�.\n";
const char* noticemem = "�� ������ ȯ���մϴ�.\n";
int mafia, police, man;
int mafiacount = 1, policecount = 1;
int daycount;
const int daylimittime = 120;
const int nightlimittime = 30;
char buffer[300];
mutex newmutex;

// �񵿱������� ä���� ó���ϴ� �Լ�
void asyncChat(int userIndex, int userSocket, int* userNum, int userCount, const vector<int>& userAlive, mutex& newmutex)
{
    // ä�� �ð� ���� ����
    const time_t start_time = time(nullptr);
    const time_t timeout = daylimittime;

    // �־��� �ð����� ���� ����
    while (time(nullptr) - start_time < timeout)
    {
        // �־��� �ð� ���� ������ ����� ���� üũ
        if (time(nullptr) - start_time > timeout)
        {
            break;
        }

        // ������ ���¸� �����ϴ� fd_set �ʱ�ȭ
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(userSocket, &readfds);

        // ���� ��� �ð� ����
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        // ���Ͽ��� �����͸� �����ϸ� ó��
        if (FD_ISSET(userSocket, &readfds))
        {
            char buffer[300];
            memset(buffer, 0, sizeof(buffer));
            int bytesReceived = recv(userSocket, buffer, sizeof(buffer), 0);

            // �����͸� ���������� �����ϰ� ����ڰ� ������� ��
            if (bytesReceived > 0 && userAlive[userIndex])
            {
                {
                    // ���ؽ��� ����Ͽ� ��� ����ȭ
                    lock_guard<mutex> lock(newmutex);

                    // �޽����� ����� ��ȣ�� �߰��Ͽ� ��ü ����ڿ��� ����
                    string messageWithUserIndex = to_string(userIndex + 1) + "�� ���� : " + buffer;
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



// ������ �߰� ������ ó���ϴ� �Լ�
int nightpoliceaction(int policeindex, int* usernum, int usercount, const vector<int>& userAlive)
{
    int result = -1;

    // ������ ������� ���� ����
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

// ���Ǿ��� �߰� ������ ó���ϴ� �Լ�
int nightmafiaaction(int mafiaindex, int* usernum, int usercount)
{
    int result = -1;
    memset(buffer, 0, sizeof(buffer));
    recv(usernum[mafiaindex], buffer, sizeof(buffer), 0);
    result = atoi(buffer);
    return result;
}

//���� ����
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
        //�� �κ� ���� ����
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


        //��ǥ ���� ����
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
        
        //��ǥ ��� ó�� ����
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
        //���Ǿư� ��ǥ�� ����� ��� ���Ǿ��� ������ ���� ����
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
        //��ǥ�� ���� ������ ���������, ���Ǿư� ���� ���� ��� ���Ǿ��� ������ ���� ����
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
        
        
        //�� �κ� ���� ����
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
            //���Ǿư� �ڻ��� ��� ���� ����
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
        //�� �κ� ������ ���� ��, ���Ǿư� ������ �ְ� ���� �������� 2���̶�� ���Ǿ��� �¸�
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