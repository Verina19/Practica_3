#include <iostream>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include "P3_server.h"
#include "windows.h"
#pragma comment(lib, "Ws2_32.lib")

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;




using namespace std;
std::mutex g_lock;

//#define TABLE_SIZE 200// TABLE_SIZE - размер хэш-таблицы

//const char IP_SERV[] = "192.168.0.104";		// Enter local Server IP address
const char IP_SERV[] = "127.0.0.1";		// Enter local Server IP address
const int PORT_NUM = 6379;				// Enter Open working server port


//Key constants
//const short BUFF_SIZE = 1024;			// Maximum size of buffer for exchange info between server and client
sockaddr_in servInfo;// специальная структура для преобразования IP адреса, порта и др. в спец. числовой формат нужный функции bind()
sockaddr_in clientInfo;
// Key variables for all program
int erStat;		// Keeps socket errors status
SOCKET ServSock;
SOCKET ClientConn;
int TCP_server(const char* IP_SERV, const int PORT_NUM) {	//Если программа отправляет данные в интернет, то в качестве адресата она использует ранее полученный IP-адрес.
	//То есть ей нужно знать IP и порт получателя. Но если программа отправляет дочерней проге своей, модулю, плагину,
	//то здесь ей нужно знать только порт (и обычно она его точно знает). Так как внутри ПК единый для всех один IP-адрес — 127.0.0.1 (localhost)



//IP из формата строки переводится в числовой формат для функций socket. Данные в "ip_to_num"
	in_addr ip_to_num;
	//функция, которая переводит обычную строку типа char[], содержащую IPv4 адрес в привычном виде с точками-разделителями
	// в структуру типа in_addr
	erStat = inet_pton(AF_INET, IP_SERV, &ip_to_num);
	if (erStat <= 0) {// обработка ошибки
		cout << "Error in IP translation to special numeric format" << endl;
		return 1;
	}


	// Этап 1: Инициализация сокетных интерфейсов Win32API
	// структура типа WSADATA, в которую автоматически в момент создания загружаются данные о версии сокетов, используемых ОС
	WSADATA wsData;// WinSock initialization
// int WSAStartup (WORD <запрашиваемая версия сокетов>, WSADATA* <указатель на структуру, хранящую текущую версию реализации сокетов>)
	erStat = WSAStartup(MAKEWORD(2, 2), &wsData);
	// WSAStartup() в случае успеха возвращает 0, 
	// а в случае каких-то проблем возвращает код ошибки, который можно расшифровать последующим вызовом функции WSAGetLastError()
	if (erStat != 0) {// обработка ошибки
		cout << "Error WinSock version initializaion #";
		cout << WSAGetLastError();
		return 1;
	}
	else
		cout << "WinSock initialization is OK" << endl;

	/* Этап 2: Создание сокета и его инициализация
	 Функция socket() возвращает дескриптор с номером сокета, под которым он зарегистрирован в ОС.
	Если же инициализировать сокет по каким-то причинам не удалось – возвращается значение INVALID_SOCKET
	Важно!!! после работы приложения обязательно закрыть использовавшиеся сокеты с помощью функции closesocket(SOCKET <имя сокета>)
	и деинициализировать сокеты Win32API через вызов метода WSACleanup()
	SOCKET socket(int <семейство используемых адресов>, int <тип сокета>, int <тип протокола>)
	*/
	// Server socket initialization
	ServSock = socket(AF_INET, SOCK_STREAM, 0);
	if (ServSock == INVALID_SOCKET) {// обработка ошибки
		cout << "Error initialization socket # " << WSAGetLastError() << endl;
		closesocket(ServSock);
		WSACleanup();
		return 1;
	}
	else
		cout << "Server socket initialization is OK" << endl;

	/* Этап 3: Привязка сокета к паре IP-адрес/Порт
	 int bind(SOCKET <имя сокета, к которому необходимо привязать адрес и порт>,
			sockaddr* <указатель на структуру, содержащую детальную информацию по адресу и порту, к которому надо привязать сокет>,
			int <размер структуры, содержащей адрес и порт>)
	 Функция bind() возвращает 0, если удалось успешно привязать сокет к адресу и порту, и код ошибки в ином случае
	*/
	// Server socket binding
	ZeroMemory(&servInfo, sizeof(servInfo));	// Initializing servInfo structure  Заполняет блок памяти нулями

	servInfo.sin_family = AF_INET;// AF_INET - IPv4
	servInfo.sin_addr = ip_to_num;// IP-адрес в числовом виде
	servInfo.sin_port = htons(PORT_NUM);//порт всегда указывается через вызов функции htons(),
										//которая переупаковывает привычное цифровое значение порта типа unsigned short
										//в побайтовый порядок понятный для протокола TCP/IP 

	erStat = bind(ServSock, (sockaddr*)&servInfo, sizeof(servInfo));
	if (erStat != 0) {// обработка ошибки
		cout << "Error Socket binding to server info. Error # " << WSAGetLastError() << endl;
		closesocket(ServSock);
		WSACleanup();
		return 1;
	}
	else
		cout << "Binding socket to Server info is OK" << endl;

	/*Этап 4 (для сервера): «Прослушивание» привязанного порта для идентификации подключений
	Для того, чтобы реализовать данный этап, нужно вызвать функцию listen()
	int listen(SOCKET <«слушающий» сокет, который мы создавали на предыдущих этапах>,
			   int <максимальное количество процессов, разрешенных к подключению>)
	Второй аргумент: максимально возможное число подключений устанавливается через передачу параметр SOMAXCONN(рекомендуется).
	Если нужно установить ограничения на количество подключений – нужно указать SOMAXCONN_HINT(N), где N – кол-во подключений.
	Если будет подключаться больше пользователей, то они будут сброшены.
   */
   //Starting to listen to any Clients
	erStat = listen(ServSock, SOMAXCONN); // SOMAXCONN - max, SOMAXCONN_HINT(1)
	if (erStat != 0) {// обработка ошибки
		cout << "Can't start to listen to. Error # " << WSAGetLastError() << endl;
		closesocket(ServSock);
		WSACleanup();
		return 1;
	}
	else {
		cout << "Listening..." << endl;
	}

	return 0;
}
int Accept() {
	/*Этап 5 (только для Сервера). Подтверждение подключения
	После начала прослушивания (вызов функции listen()) следующей функцией должна идти функция accept(),
	которую будет искать программа после того, как установится соединение с Клиентом.
	Прототип функции accept():
	SOCKET accept(SOCKET <"слушающий" сокет на стороне Сервера>,
				  sockaddr* <указатель на пустую структуру sockaddr, в которую будет записана информация по подключившемуся Клиенту>,
				  int* <указатель на размер структуры типа sockaddr>)
	 Функция accept() возвращает номер дескриптора, под которым зарегистрирован сокет в ОС.
	 Если произошла ошибка, то возвращается значение INVALID_SOCKET
	*/
	//Client socket creation and acception in case of connection
	ZeroMemory(&clientInfo, sizeof(clientInfo));	// Initializing clientInfo structure

	int clientInfo_size = sizeof(clientInfo);

	ClientConn = accept(ServSock, (sockaddr*)&clientInfo, &clientInfo_size);

	if (ClientConn == INVALID_SOCKET) {// обработка ошибки
		cout << "Client detected, but can't connect to a client. Error # " << WSAGetLastError() << endl;
		closesocket(ServSock);
		closesocket(ClientConn);
		WSACleanup();
		return 1;
	}
	else {
		cout << "Connection to a client established successfully" << endl;
		char clientIP[22];
		inet_ntop(AF_INET, &clientInfo.sin_addr, clientIP, INET_ADDRSTRLEN);	// Convert connected client's IP to standard string format
		cout << "Client connected with IP address " << clientIP << endl;
	}
	return 0;
}
//Exchange text data between Server and Client. Disconnection if a client send "xxx"

int SendToTCP(char* clientBuff, size_t BUFF_SIZE, SOCKET ClientConn) {
	short packet_size = 0;			// The size of sending / receiving packet in bytes
	packet_size = send(ClientConn, clientBuff, BUFF_SIZE, 0);
	if (packet_size == SOCKET_ERROR) {
		cout << "Can't send message to Client. Error # " << WSAGetLastError() << endl;
		closesocket(ServSock);
		closesocket(ClientConn);
		WSACleanup();
		return 1;
	}
	return 0;
}
int ReceiveTCP(char* servBuff, size_t BUFF_SIZE, SOCKET ClientConn) {
	short packet_size = 0;			// The size of sending / receiving packet in bytes
	packet_size = recv(ClientConn, servBuff, BUFF_SIZE, 0);//прием из потока тсп
	return 0;
}

////______________________________________________________________

void CloseTCP(SOCKET ClientConn) {
	closesocket(ServSock);
	closesocket(ClientConn);
	WSACleanup();
	return;
}
//
//

void Client(SOCKET ClientConn) {
	char command[15];
	char MainMenu[] = "\nВыберите комманду:\n"
		"Get - ввести\nPost - вывести\nEND - выход:\n";

	while (true) {
		ReceiveTCP(command, 15, ClientConn);

		if (!strcmp(command, "?") || !strcmp(command, "help") || !strcmp(command, "HELP")) {
			printf("Получена команда: ?");
			printf("\n");
			SendToTCP(MainMenu, strlen(MainMenu), ClientConn);
			continue;
		}//help

		if (!strcmp(command, "Get") || !strcmp(command, "get") || !strcmp(command, "g")) {
			Hash(ClientConn);
			SendToTCP(MainMenu, strlen(MainMenu), ClientConn);
			continue;
		}//if_7
		if (!strcmp(command, "Post") || !strcmp(command, "post") || !strcmp(command, "p")) {
			Hash(ClientConn);
			SendToTCP(MainMenu, strlen(MainMenu), ClientConn);
			continue;
		}//if_7

		if (!strcmp(command, "END") || !strcmp(command, "end") || !strcmp(command, "e")) {
			printf("\n\nРабота клиента завершена\n");
			break;
		}
		char mess[] = "\nНеверный запрос\nВведите ? для помощи";
		SendToTCP(mess, strlen(mess), ClientConn);
	}
	CloseTCP(ClientConn);
}



int main() {
    //utility::string_t address = U("http://localhost:8080/api/shorten");
    //http_listener listener(address);

    //listener.support(methods::POST, [](http_request request) {
    //    request.extract_json().then([=](json::value jsonData) {
    //        const utility::string_t& longUrl = jsonData.at(U("longUrl")).as_string();

    //        // Здесь можно добавить логику для генерации короткой ссылки
    //        utility::string_t shortUrl = U("http://короткая-ссылка.com/abcd123"); // Пример генерации короткой ссылки

    //        json::value response;
    //        response[U("shortUrl")] = json::value::string(shortUrl);

    //        request.reply(status_codes::OK, response);
    //        });
    //    });

    //try {
    //    listener.open().then([]() {
    //        std::wcout << "Сервер запущен\n";
    //        }).wait();

    //        while (true);
    //}
    //catch (const std::exception& e) {
    //    std::wcerr << e.what() << std::endl;
    //}

    //return 0;
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);
	if (TCP_server(IP_SERV, PORT_NUM)) return 0;
	while (1) {
		if (Accept()) return 0;
		std::thread thr(Client, ClientConn);
		thr.detach();
	}
}