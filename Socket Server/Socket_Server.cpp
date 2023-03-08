#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <winsock2.h> // Include the Winsock library


#pragma comment(lib, "ws2_32.lib") // Link with the Winsock library

#define BUFFER_SIZE 1024
#define PORT 12345
#define AUTH_KEY "my_secret_key"

int main()
{
    // Initialize Winsock
    WSADATA wsa;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }

    // Create a socket for listening
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        printf("socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Bind the socket to a local address and port
    struct sockaddr_in localAddress;
    localAddress.sin_family = AF_INET;
    localAddress.sin_addr.s_addr = INADDR_ANY;
    localAddress.sin_port = htons(PORT);

    result = bind(listenSocket, (struct sockaddr*)&localAddress, sizeof(localAddress));
    if (result == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    result = listen(listenSocket, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        printf("listen failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    printf("Server is listening on port %d.\n", PORT);

    // Accept incoming connections and handle them
    bool running = true;
    char buffer[BUFFER_SIZE];
    int bytesReceived;

    while (running) {
        // Wait for a client to connect
        struct sockaddr_in clientAddress;
        int clientAddressSize = sizeof(clientAddress);

        SOCKET clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddress, &clientAddressSize);
        if (clientSocket == INVALID_SOCKET) {
            printf("accept failed: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        // Print client information
        char* clientIPAddress = inet_ntoa(clientAddress.sin_addr);
        printf("Client connected from %s:%d.\n", clientIPAddress, ntohs(clientAddress.sin_port));

        // Receive packets from the client
        while (true) {
            // Receive the packet size
            int packetSize;
            bytesReceived = recv(clientSocket, (char*)&packetSize, sizeof(int), 0);
            if (bytesReceived == SOCKET_ERROR) {
                printf("recv failed Socket Error at packet size: %d\n", WSAGetLastError());
                closesocket(clientSocket);
                break;
            }

            // Check if the client disconnected
            if (bytesReceived == 0) {
                printf("Client disconnected.\n");
                closesocket(clientSocket);
                break;
            }

            // Allocate memory for the packet
            char* packet = (char*)malloc(packetSize);
            memset(packet, 0, packetSize);

            // Receive the packet data
            bytesReceived = recv(clientSocket, packet, packetSize, 0);
            if (bytesReceived == SOCKET_ERROR) {
                printf("recv failed Socket Error at packet data: %d\n", WSAGetLastError());
                closesocket(clientSocket);
                free(packet);
                break;
            }
            // Check if the client disconnected
            if (bytesReceived == 0) {
                printf("Client disconnected.\n");
                closesocket(clientSocket);
                free(packet);
                break;
            }
            
            // Verify the authentication key
            if (strncmp(packet, AUTH_KEY, strlen(AUTH_KEY)) != 0) {
                printf("Authentication failed for client %s:%d.\n", clientIPAddress, ntohs(clientAddress.sin_port));
                closesocket(clientSocket);
                free(packet);
                break;
            }

            // Print the received message
            printf("Received message from %s:%d: %s\n", clientIPAddress, ntohs(clientAddress.sin_port), packet + strlen(AUTH_KEY));

            // Send a response message
            char responseBuffer[BUFFER_SIZE];
            snprintf(responseBuffer, BUFFER_SIZE, "Response from server to %s:%d", clientIPAddress, ntohs(clientAddress.sin_port));
            int responseSize = strlen(responseBuffer) + 1;

            result = send(clientSocket, (char*)&responseSize, sizeof(int), 0);
            if (result == SOCKET_ERROR) {
                printf("send failed: %d\n", WSAGetLastError());
                closesocket(clientSocket);
                free(packet);
                break;
            }

            result = send(clientSocket, responseBuffer, responseSize, 0);
            if (result == SOCKET_ERROR) {
                printf("send failed: %d\n", WSAGetLastError());
                closesocket(clientSocket);
                free(packet);
                break;
            }

            // Free memory for the packet
            free(packet);
        }
    }

    // Clean up
    closesocket(listenSocket);
    WSACleanup();

    return 0;
}