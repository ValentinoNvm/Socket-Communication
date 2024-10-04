#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT "8083"
#define DEFAULT_BUFLEN 512
#pragma comment(lib, "Ws2_32.lib")


HANDLE sendevent, receive;

//Function For Receiving Messages
DWORD WINAPI ClientThread(void* param) {
	SOCKET* client = (SOCKET*)param;
	char buffer[1024];
	int r;
	while (true) {
		WaitForSingleObject(receive, INFINITE);
			std::cout << "Waiting for a message. " << std::endl;
			r = recv(*client, buffer, sizeof(buffer), 0);
			if (r > 0) {
				//buffer[r] = '\0'; 
				std::cout << "Client: " << buffer << std::endl;
				SetEvent(sendevent);
			}
			else {
				std::cout << "Error during receiving a message.. " << std::endl;
				return 1;
			}	
	}
	return 0;
}

//Function For Sending Messages
DWORD WINAPI ServerThread(void* param) {
	SOCKET* client = (SOCKET*)param;
	int r;
	char buffer[1024];
	while (true) {
		WaitForSingleObject(sendevent, INFINITE);
			std::cout << "write: ";
			std::cin.getline(buffer, sizeof(buffer));
			r = send(*client, buffer, strlen(buffer)+1,0);
			if(r!=SOCKET_ERROR) {
				std::cout << "Message sent withouth errors." << std::endl;
				SetEvent(receive);
			}
			else {
				std::cout << "can't send message.. " << std::endl;
				return 1;
			}
	}
	return 0;
}


int main() {
	//initializing winsock
	WSADATA wsaData;
	int ret;
	//error control
	ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret != 0) {
		std::cout << "WSAStartup failed: " << ret << std::endl;
		return 1;
	}

	//struct for resolving addresses
	struct addrinfo* result = NULL, * ptr = NULL, hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	//resolve the local address and port to be used by the server
	ret = getaddrinfo(DEFAULT_IP, DEFAULT_PORT, &hints, &result);
	if (ret != 0) {
		std::cout << "getaddrinfo failed, return: " << ret << std::endl;
		WSACleanup();
		return 1;
	}

	//listen socket
	SOCKET listensocket = INVALID_SOCKET;
	listensocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	//check errors for listening part
	if (listensocket == INVALID_SOCKET) {
		std::cout << "error listensocket" << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	//now we bind it
	//setup the TCP listening socket
	ret = bind(listensocket, result->ai_addr, (int)result->ai_addrlen);
	if (ret == SOCKET_ERROR) {
		std::cout << "binding failed: " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		closesocket(listensocket);
		WSACleanup();
		return 1;
	}
	//free memory, we don't need it anymore
	freeaddrinfo(result);

	//listening and error checking
	ret = listen(listensocket, SOMAXCONN);
	//check errors
	if (ret == SOCKET_ERROR) {
		std::cout << "listen failed " << WSAGetLastError() << std::endl;
		closesocket(listensocket);
		WSACleanup();
		return 1;
	}
	else {
		std::cout << "Listening.. " << std::endl;
	}


	//accepting a connection from clients with temporary socket
	SOCKET clientsocket = INVALID_SOCKET;
	clientsocket = accept(listensocket, NULL, NULL);
	//check errors accept request
	if (clientsocket == INVALID_SOCKET) {
		std::cout << "accept failed: " << WSAGetLastError() << std::endl;
		closesocket(listensocket);
		WSACleanup();
		return 1;
	}
	else  {
		std::cout << "Connection Established " << std::endl;
	}
	

	//Creating events for sync. threads
	sendevent = CreateEvent(NULL, FALSE, FALSE, NULL);
	receive = CreateEvent(NULL, FALSE, FALSE, NULL);

	//Create HANDLES
	HANDLE client_thread = CreateThread(NULL, 0, ClientThread, &clientsocket, 0, NULL);
	HANDLE server_thread = CreateThread(NULL, 0, ServerThread, &clientsocket, 0, NULL);

	
	//start the first thread
	SetEvent(receive);
	WaitForSingleObject(client_thread, INFINITE);
	WaitForSingleObject(server_thread, INFINITE);


	//disconnecting the server:
	ret = shutdown(clientsocket, SD_SEND);
	if (ret == SOCKET_ERROR) {
		std::cout << "socket closing failure.. " << WSAGetLastError() << std::endl;
		closesocket(clientsocket);
		WSACleanup();
		return 1;
	}

	//cleanup
	closesocket(clientsocket);
	CloseHandle(client_thread);
	CloseHandle(server_thread);
	WSACleanup();
	return 0;
}