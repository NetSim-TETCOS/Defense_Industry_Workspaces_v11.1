#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Ws2tcpip.h>
#include <synchapi.h>
#include <windows.h>
#include <stdbool.h>
#include <signal.h>
#include "CLI.h"
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Synchronization.lib")

WSADATA wsa;
SOCKET listenSocket;
#define DEFAULT_BUFLEN 1024
#define DEFAULT_PORT	"8999"

HANDLE listenThread;
DWORD listenThreadId;

void send_to_socket(ptrCLIENTINFO info,char* buf, int len)
{
	//fprintf(stderr, "\nSending %s\n", buf);
	int iSendResult = send(info->CLIENT.sockClient.clientSocket, buf, len, 0);
	if (iSendResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(info->CLIENT.sockClient.clientSocket);
	}
}

DWORD WINAPI listen_for_client(LPVOID lpParam)
{
	struct addrinfo hints;
	int iResult;
	struct addrinfo *result = NULL;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		fprintf(stderr, "getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return false;
	}

	// Create a SOCKET for connecting to server
	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listenSocket == INVALID_SOCKET)
	{
		fprintf(stderr, "socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}

	// Setup the TCP listening socket
	iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		fprintf(stderr, "bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(listenSocket);
		WSACleanup();
		return false;
	}

	freeaddrinfo(result);

	SOCKET clientSocket;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	while (1)
	{
		iResult = listen(listenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR)
		{
			fprintf(stderr, "listen failed with error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return false;
		}

		// Accept a client socket
		clientSocket = accept(listenSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET)
		{
			fprintf(stderr, "accept failed with error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return false;
		}

		iResult = recv(clientSocket, recvbuf, recvbuflen, 0);

		if (iResult > 0)
		{
			add_new_socket_client(clientSocket, recvbuf);
		}
		else
		{
			fprintf(stderr, "recv failed with error: %d\n", WSAGetLastError());
			closesocket(clientSocket);
			return false;
		}
	}
}

void init_socket()
{
	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		fprintf(stderr, "Failed. Error Code : %d\n", WSAGetLastError());
		return;
	}
	printf("Initialized.\n");

	CreateThread(NULL, 1024, listen_for_client, NULL, false, &listenThreadId);

}

DWORD WINAPI command_recv_process(LPVOID lpParam)
{
	ptrCLIENTINFO clientInfo = lpParam;
	SOCKCLIENTINFO sockClient = clientInfo->CLIENT.sockClient;
	int iResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	// Receive until the peer shuts down the connection
	do
	{

		iResult = recv(sockClient.clientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			process_command(clientInfo, recvbuf, iResult);
			WaitOnAddress(&sockClient.iswait,
						  &sockClient.dontUseMe,
						  sizeof sockClient.dontUseMe,
						  INFINITE);
			send_to_socket(clientInfo, "__continue__", (int)(strlen("__continue__")) + 1);
		}
		else if (iResult == 0)
			fprintf(stderr, "Connection closing...\n");
		else
		{
			fprintf(stderr,"recv failed with error: %d\n", WSAGetLastError());
			closesocket(sockClient.clientSocket);
			return 1;
		}

	} while (iResult > 0);

	// shutdown the connection since we're done
	iResult = shutdown(sockClient.clientSocket, SD_BOTH);
	if (iResult == SOCKET_ERROR)
	{
		fprintf(stderr, "shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(sockClient.clientSocket);
		return 1;
	}

	// cleanup
	closesocket(sockClient.clientSocket);
	return 0;
}
