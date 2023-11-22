#pragma once
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include "windows.h"
#include <thread>
#include <mutex>

#define TABLE_SIZE 200// TABLE_SIZE - размер хэш-таблицы

//std::mutex g_lock;//
int SendToTCP(char* clientBuff, size_t BUFF_SIZE, SOCKET ClientConn);
int ReceiveTCP(char* servBuff, size_t BUFF_SIZE, SOCKET ClientConn);
//int Receive2TCP(int* servBuff, size_t BUFF_SIZE, SOCKET ClientConn);
//________________________________
// 
int hash1(char* key);
int hash2(int hash_value, char* key);

// 
//хэш-таблица
void Hash(SOCKET ClientConn); 
