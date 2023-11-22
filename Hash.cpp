#include "P3_server.h"
//#include <iostream>
//#include <stdio.h>
//#include "windows.h"
//#include "windows.h"

//#define TABLE_SIZE 200// TABLE_SIZE - ������ ���-�������
using namespace std;
extern std::mutex g_lock;
struct hashbase {
	char key1[TABLE_SIZE][101];
	char value[TABLE_SIZE][101];
};
hashbase myhash; // ���-�������


// ���-������� ��� ���������� ������� � ���-�������. 
// ��������� �������� ASCII-����� �������� ����� � ���������� ������� �� ������� �� ������ ���-�������
int hash1(char* key) {
	int hash_value = 0;
	for (int i = 0; key[i] != '\0'; i++) {
		hash_value += key[i];
	}
	return hash_value % TABLE_SIZE;
}

// ���-������� ��� ���������� ������� � ���-�������. 
// ��������� �������� ASCII-����� �������� ����� � ���������� �������� ���, � ���������� ������� �� ������� �� ������ ���-�������
int hash2(int hash_value, char* key) {
	for (int i = 0; key[i] != '\0'; i++) {
		hash_value += key[i];
	}
	return hash_value % (TABLE_SIZE - 1) + 1;
}

// ������� �������� ������� �������� � ����
int FindHash(char* key) {
	int index = hash1(key);// ��������� ������ (���)
	if (!strcmp(myhash.key1[index], key)) {// ��������� ��� ������� ����� ����������� ������
		return index;
	}
	else {// ���� �� ������� ���� �� ����� ������� ��� �������
		int count = 0;
		while (1) {
			index = hash2(index, key);
			if (!strcmp(myhash.key1[index], key)) {// ��������� ��� ������� ����� ����������� ������
				return index;
			}
			count++;
			if (count > 200) break;
		}
	}
	return -1;
}

// ������� ��������� ����� ������ � ���-�������
int AddElement(char* element, char* key) {
	if (FindHash(key) >= 0) {
		return 1;
	}
	int index = hash1(key); // ��������� ������ (���)
	if (!strcmp(myhash.key1[index], "\0")) { // ��������� ��� ������ ������
		strcpy_s(myhash.key1[index], 101, key); // �������� ����
		strcpy_s(myhash.value[index], 101, element); // �������� ������
		return 0;
	}
	else {// ���� ������ ���� ������ ���������� �� ������ �����������
		int count = 0;
		while (1) {
			index = hash2(index, key);
			if (!strcmp(myhash.key1[index], "\0")) { // ��������� ��� ������ ������
				strcpy_s(myhash.key1[index], 101, key); // �������� ����
				strcpy_s(myhash.value[index], 101, element); // �������� ������
				return 0;
			}
			count++;
			if (count > 200) break;
		}
	}
	return 1; // ������, ��� ������
}
void AddHesh(SOCKET ClientConn) {
	char element[101];
	char key[101];
	char m1[31] = { "������� ����� ���� ��������:\n" };
	SendToTCP(m1, strlen(m1), ClientConn);
	ReceiveTCP(key, 101, ClientConn);

	printf("����� ���� %s", key); printf("\n");

	char m2[30] = { "������� ����� �������:\n" };

	SendToTCP(m2, strlen(m2), ClientConn);
	ReceiveTCP(element, 101, ClientConn);

	printf("����� ������� %s", element); printf("\n");

	g_lock.lock();//������
	if (AddElement(element, key)) {// ���� = 1 ������ ����� ������� ��� ����
		printf("����� key ��� ����\n");
		char m3[30] = { "����� key ��� ����\n" };
		SendToTCP(m3, strlen(m3), ClientConn);
	}
	else {// ������� ��������
		printf("������� ��������\n");
		char m3[30] = { "������� ��������\n" };
		SendToTCP(m3, strlen(m3), ClientConn);
	}
	g_lock.unlock();//������
}

// ������� ������� ������ �� �������
int  RemoveElement(char* key) {
	int index = FindHash(key); // ���� ����� ����
	if (index >= 0) {// ���� ����� �������
		strcpy_s(myhash.key1[index], 101, "\0"); // ������� ������ �����
		strcpy_s(myhash.value[index], 101, "\0"); // ������� ������ ������
		return 0;
	}
	return 1;
}
void DelHesh(SOCKET ClientConn) {
	char key[101];
	char m1[40] = { "������� ���� ���������� �������:\n" };
	SendToTCP(m1, strlen(m1), ClientConn);
	ReceiveTCP(key, 101, ClientConn);

	printf("����� ���� %s", key); printf("\n");

	g_lock.lock();//������
	if (RemoveElement(key)) {// ���� = 1 ������ ����� ������� ��� ����
		printf("������� �� ������\n");
		char m3[30] = { "������� �� ������\n" };
		SendToTCP(m3, strlen(m3), ClientConn);
	}
	else {// ������� ������
		printf("������� ������\n");
		char m3[30] = { "������� ������\n" };
		SendToTCP(m3, strlen(m3), ClientConn);
	}
	g_lock.unlock();//������
}

void GetHesh(SOCKET ClientConn) {
	char key[101];
	char m1[40] = { "������� ���� ��������:\n" };
	SendToTCP(m1, strlen(m1), ClientConn);
	ReceiveTCP(key, 101, ClientConn);

	printf("����� ���� %s", key); printf("\n");

	g_lock.lock();//������
	int index = FindHash(key); // ���� ����� ����
	if (index >= 0) {// ���� ����� ��������
		SendToTCP(myhash.value[index], 101, ClientConn);
	}
	else {
		char m3[30] = { "������ �������� ���\n" };
		SendToTCP(m3, strlen(m3), ClientConn);
	}
	g_lock.unlock();//������
}

void Hash(SOCKET ClientConn) {
	char command[15];
	printf("�������� ������� 7 - ���-�������\n");
	char Menu[] = "\n������� ��������:\nHSET - ����� �������� �������\nHDEL - ������� '�������\n"
		"HGET - ��������� �������\nP    - ������ ���� ����\nRET  - ������� � ������� ����\n";
	SendToTCP(Menu, strlen(Menu), ClientConn);
	while (true) {
		ReceiveTCP(command, 15, ClientConn);

		if (!strcmp(command, "HELP") || !strcmp(command, "help") || !strcmp(command, "?")) {// �������� �������
			printf("�������� ������� HELP\n");
			SendToTCP(Menu, strlen(Menu), ClientConn);
			continue;
		}

		if (!strcmp(command, "HSET") || !strcmp(command, "hset") || !strcmp(command, "s")) {// �������� �������
			printf("�������� ������� HSET\n");
			AddHesh(ClientConn);
			continue;
		}

		if (!strcmp(command, "HDEL") || !strcmp(command, "hdel") || !strcmp(command, "d")) {//������� �������
			printf("�������� ������� HDEL\n");
			DelHesh(ClientConn);
			continue;
		}
		if (!strcmp(command, "HGET") || !strcmp(command, "hget") || !strcmp(command, "g")) {//��������� �������
			printf("�������� ������� HGET\n");
			GetHesh(ClientConn);
			continue;
		}
		if (!strcmp(command, "P") || !strcmp(command, "p")) {// ������ ���� ����
			printf("�������� ������� �\n");
			char buff0[1000]; buff0[0] = '\0';
			int j = 0, k;
			for (int i = 0; i < TABLE_SIZE; i++) {
				if (strcmp(myhash.key1[i], "\0")) {// ���������� ������ ������
					char buff[20];
					_itoa_s(i, buff, _countof(buff), 10);//������� ��� � ���
					k = strlen(buff);// ��������� ����� ������ �������
					for (int m = 0; m < k; m++) {
						buff0[j] = buff[m];
						j++;
					}
					char buff1[] = { " - " };
					for (int m = 0; m < 3; m++) {
						buff0[j] = buff1[m];
						j++;
					}
					k = strlen(myhash.key1[i]);// ��������� ����� ������ �������
					for (int m = 0; m < k; m++) {
						buff0[j] = myhash.key1[i][m];
						j++;
					}
					buff0[j] = ';'; j++; buff0[j] = ' '; j++;
					k = strlen(myhash.value[i]);// ��������� ����� ������ �������
					for (int m = 0; m < k; m++) {
						buff0[j] = myhash.value[i][m];
						j++;
					}
					buff0[j] = '\n'; j++;
				}
			}
			buff0[j] = '\0';
			size_t i = strlen(buff0);
			if (buff0[0] == '\0') {
				char buf[] = { "������� �����" };
				SendToTCP(buf, 15, ClientConn);
			}
			else {
				SendToTCP(buff0, i + 1, ClientConn);
			}
			continue;
		}

		if (!strcmp(command, "RET") || !strcmp(command, "ret") || !strcmp(command, "r")) {
			printf("������� � �������� ����: ?\n");
			break;
		}

		char mess[] = "\n�������� ������\n������� ? ��� ������\n";
		SendToTCP(mess, strlen(mess), ClientConn);
	}//while
}