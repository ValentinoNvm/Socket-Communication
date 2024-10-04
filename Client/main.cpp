#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT "8083"

//events
HANDLE sendevent, receive;

//Sending Messages
DWORD WINAPI ClientThread(void* param) {
	SOCKET* client = (SOCKET*)param;
	char buffer[1024];
	int r;
	while (true) {
			WaitForSingleObject(sendevent, INFINITE);
			std::cout << "Sending: " ;
			std::cin >> buffer;
			r = send(*client, buffer, strlen(buffer)+1, 0);
			
			if (r == SOCKET_ERROR) {
				std::cout << "Can't send the message.";
				return 1;

			}
			std::cout << "Message Sent." << std::endl;
			SetEvent(receive);

	}
	return 0;
}


//receiving messages
DWORD WINAPI ServerThread(void* param) {
	SOCKET* client = (SOCKET*)param;
	char buffer[1024];
	int r;
	while (true) {
		WaitForSingleObject(receive, INFINITE);
			std::cout << "I'm waiting a response.. " << std::endl;
			r = recv(*client, buffer, sizeof(buffer), 0);
			if (r > 0) {
				std::cout << "Server:" << buffer << std::endl;
				SetEvent(sendevent);
			}
			if (r == 0) {
				std::cout << "errore ricezione messaggio.." << std::endl;
				SetEvent(sendevent);
			}
	}
	return 0;
}


int main() {
	//Winsock with control
	WSADATA wsa;
	int r = 0;
	r = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (r != 0) {
		std::cout << "wsa failed: " << r << std::endl;
		return 1;
	}

	//addresses struct
	struct addrinfo* result = NULL, * ptr = NULL, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	//resolution address
	r = getaddrinfo(DEFAULT_IP, DEFAULT_PORT, &hints, &result);

	//socket for connecting
	SOCKET connessione = INVALID_SOCKET;
	ptr = result;
	connessione = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	if (connessione == INVALID_SOCKET) {
		std::cout << "can't setup the socket" << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		WSACleanup();
	}

	//socket connection
	int c = connect(connessione, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (c == SOCKET_ERROR) {
		std::cout << "failed to connect to the server" << std::endl;
	}
	else {
		std::cout << "connection established." << std::endl;
	}

	//multithreading
	sendevent = CreateEvent(NULL, FALSE, FALSE, NULL);
	receive = CreateEvent(NULL, FALSE, FALSE, NULL);
	HANDLE client_thread = CreateThread(NULL, 0, ClientThread, &connessione, 0, NULL);
	HANDLE server_thread = CreateThread(NULL, 0, ServerThread, &connessione, 0, NULL);

	
	//start the first thread
	SetEvent(sendevent);
		
	
	// wait the threads to finish
	WaitForSingleObject(client_thread, INFINITE);
	WaitForSingleObject(server_thread, INFINITE);

	//cleaning everything
	closesocket(connessione);
	WSACleanup();
	CloseHandle(sendevent);
	CloseHandle(receive);
	return 0;
}