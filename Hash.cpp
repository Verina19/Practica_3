#include "P3_server.h"
//#include <iostream>
//#include <stdio.h>
//#include "windows.h"
//#include "windows.h"

//#define TABLE_SIZE 200// TABLE_SIZE - размер хэш-таблицы
using namespace std;
extern std::mutex g_lock;
struct hashbase {
	char key1[TABLE_SIZE][101];
	char value[TABLE_SIZE][101];
};
hashbase myhash; // хэш-таблица


// хэш-функция для вычисления индекса в хэш-таблице. 
// суммирует значения ASCII-кодов символов ключа и возвращает остаток от деления на размер хэш-таблицы
int hash1(char* key) {
	int hash_value = 0;
	for (int i = 0; key[i] != '\0'; i++) {
		hash_value += key[i];
	}
	return hash_value % TABLE_SIZE;
}

// хэш-функция для вычисления индекса в хэш-таблице. 
// суммирует значения ASCII-кодов символов ключа и предыдущее значение хэш, и возвращает остаток от деления на размер хэш-таблицы
int hash2(int hash_value, char* key) {
	for (int i = 0; key[i] != '\0'; i++) {
		hash_value += key[i];
	}
	return hash_value % (TABLE_SIZE - 1) + 1;
}

// Функция проверки наличия элемента в базе
int FindHash(char* key) {
	int index = hash1(key);// вычисляем индекс (хэш)
	if (!strcmp(myhash.key1[index], key)) {// проверяем что элемент равен содержимому ячейки
		return index;
	}
	else {// если по первому хэшу не нашли смотрим всю таблицу
		int count = 0;
		while (1) {
			index = hash2(index, key);
			if (!strcmp(myhash.key1[index], key)) {// проверяем что элемент равен содержимому ячейки
				return index;
			}
			count++;
			if (count > 200) break;
		}
	}
	return -1;
}

// Функция добавляет новую запись в хэш-таблицу
int AddElement(char* element, char* key) {
	if (FindHash(key) >= 0) {
		return 1;
	}
	int index = hash1(key); // вычисляем индекс (хэш)
	if (!strcmp(myhash.key1[index], "\0")) { // проверяем что ячейка пустая
		strcpy_s(myhash.key1[index], 101, key); // копируем ключ
		strcpy_s(myhash.value[index], 101, element); // копируем данные
		return 0;
	}
	else {// если ячейка была занята отправляем на второе хэширование
		int count = 0;
		while (1) {
			index = hash2(index, key);
			if (!strcmp(myhash.key1[index], "\0")) { // проверяем что ячейка пустая
				strcpy_s(myhash.key1[index], 101, key); // копируем ключ
				strcpy_s(myhash.value[index], 101, element); // копируем данные
				return 0;
			}
			count++;
			if (count > 200) break;
		}
	}
	return 1; // Ошибка, все занято
}
void AddHesh(SOCKET ClientConn) {
	char element[101];
	char key[101];
	char m1[31] = { "Введите новый ключ элемента:\n" };
	SendToTCP(m1, strlen(m1), ClientConn);
	ReceiveTCP(key, 101, ClientConn);

	printf("Введён ключ %s", key); printf("\n");

	char m2[30] = { "Введите новый элемент:\n" };

	SendToTCP(m2, strlen(m2), ClientConn);
	ReceiveTCP(element, 101, ClientConn);

	printf("Введён элемент %s", element); printf("\n");

	g_lock.lock();//мьтекс
	if (AddElement(element, key)) {// Если = 1 значит Такой элемент уже есть
		printf("Такой key уже есть\n");
		char m3[30] = { "Такой key уже есть\n" };
		SendToTCP(m3, strlen(m3), ClientConn);
	}
	else {// Элемент добавлен
		printf("Элемент добавлен\n");
		char m3[30] = { "Элемент добавлен\n" };
		SendToTCP(m3, strlen(m3), ClientConn);
	}
	g_lock.unlock();//мьтекс
}

// Функция удаляет запись из таблицы
int  RemoveElement(char* key) {
	int index = FindHash(key); // Ищем такой ключ
	if (index >= 0) {// если нашли удаляем
		strcpy_s(myhash.key1[index], 101, "\0"); // очищаем ячейку ключа
		strcpy_s(myhash.value[index], 101, "\0"); // очищаем ячейку данных
		return 0;
	}
	return 1;
}
void DelHesh(SOCKET ClientConn) {
	char key[101];
	char m1[40] = { "Введите ключ удаляемого элемент:\n" };
	SendToTCP(m1, strlen(m1), ClientConn);
	ReceiveTCP(key, 101, ClientConn);

	printf("Введён ключ %s", key); printf("\n");

	g_lock.lock();//мьтекс
	if (RemoveElement(key)) {// Если = 1 значит Такой элемент уже есть
		printf("Элемент не найден\n");
		char m3[30] = { "Элемент не найден\n" };
		SendToTCP(m3, strlen(m3), ClientConn);
	}
	else {// Элемент удален
		printf("Элемент удален\n");
		char m3[30] = { "Элемент удален\n" };
		SendToTCP(m3, strlen(m3), ClientConn);
	}
	g_lock.unlock();//мьтекс
}

void GetHesh(SOCKET ClientConn) {
	char key[101];
	char m1[40] = { "Введите ключ элемента:\n" };
	SendToTCP(m1, strlen(m1), ClientConn);
	ReceiveTCP(key, 101, ClientConn);

	printf("Введён ключ %s", key); printf("\n");

	g_lock.lock();//мьтекс
	int index = FindHash(key); // Ищем такой ключ
	if (index >= 0) {// если нашли печатаем
		SendToTCP(myhash.value[index], 101, ClientConn);
	}
	else {
		char m3[30] = { "такого элемента нет\n" };
		SendToTCP(m3, strlen(m3), ClientConn);
	}
	g_lock.unlock();//мьтекс
}

void Hash(SOCKET ClientConn) {
	char command[15];
	printf("Получена команда 7 - Хэш-таблица\n");
	char Menu[] = "\nВведите комманду:\nHSET - чтобы добавить элемент\nHDEL - удалить 'элемент\n"
		"HGET - прочитать элемент\nP    - печать всей базы\nRET  - возврат в главное меню\n";
	SendToTCP(Menu, strlen(Menu), ClientConn);
	while (true) {
		ReceiveTCP(command, 15, ClientConn);

		if (!strcmp(command, "HELP") || !strcmp(command, "help") || !strcmp(command, "?")) {// добавить элемент
			printf("Получена команда HELP\n");
			SendToTCP(Menu, strlen(Menu), ClientConn);
			continue;
		}

		if (!strcmp(command, "HSET") || !strcmp(command, "hset") || !strcmp(command, "s")) {// добавить элемент
			printf("Получена команда HSET\n");
			AddHesh(ClientConn);
			continue;
		}

		if (!strcmp(command, "HDEL") || !strcmp(command, "hdel") || !strcmp(command, "d")) {//удалить элемент
			printf("Получена команда HDEL\n");
			DelHesh(ClientConn);
			continue;
		}
		if (!strcmp(command, "HGET") || !strcmp(command, "hget") || !strcmp(command, "g")) {//прочитать элемент
			printf("Получена команда HGET\n");
			GetHesh(ClientConn);
			continue;
		}
		if (!strcmp(command, "P") || !strcmp(command, "p")) {// печать всей базы
			printf("Получена команда р\n");
			char buff0[1000]; buff0[0] = '\0';
			int j = 0, k;
			for (int i = 0; i < TABLE_SIZE; i++) {
				if (strcmp(myhash.key1[i], "\0")) {// пропускаем пустые строки
					char buff[20];
					_itoa_s(i, buff, _countof(buff), 10);//перевод инт в чар
					k = strlen(buff);// вычисляем длину точную массива
					for (int m = 0; m < k; m++) {
						buff0[j] = buff[m];
						j++;
					}
					char buff1[] = { " - " };
					for (int m = 0; m < 3; m++) {
						buff0[j] = buff1[m];
						j++;
					}
					k = strlen(myhash.key1[i]);// вычисляем длину точную массива
					for (int m = 0; m < k; m++) {
						buff0[j] = myhash.key1[i][m];
						j++;
					}
					buff0[j] = ';'; j++; buff0[j] = ' '; j++;
					k = strlen(myhash.value[i]);// вычисляем длину точную массива
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
				char buf[] = { "Таблица пуста" };
				SendToTCP(buf, 15, ClientConn);
			}
			else {
				SendToTCP(buff0, i + 1, ClientConn);
			}
			continue;
		}

		if (!strcmp(command, "RET") || !strcmp(command, "ret") || !strcmp(command, "r")) {
			printf("Возврат в основное меню: ?\n");
			break;
		}

		char mess[] = "\nНеверный запрос\nВведите ? для помощи\n";
		SendToTCP(mess, strlen(mess), ClientConn);
	}//while
}