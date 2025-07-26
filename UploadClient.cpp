#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <string>
#include <thread>
#include <filesystem>

#pragma comment(lib, "ws2_32.lib")

std::string getFilenameFromPath(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    return (slash != std::string::npos) ? path.substr(slash + 1) : path;
}

int main() {
    WSADATA wsaData;
    int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaResult != 0) {
        std::cerr << "WSAStartup failed: " << wsaResult << std::endl;
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed." << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection to server failed." << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::string command = "upload";
    std::string username;
    std::string filepath;

    std::cout << "Enter your username: ";
    std::getline(std::cin, username);

    std::cout << "Enter path to file to upload: ";
    std::getline(std::cin, filepath);

    std::string filename = getFilenameFromPath(filepath);

    send(sock, command.c_str(), static_cast<int>(command.size()), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    send(sock, username.c_str(), static_cast<int>(username.size()), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    send(sock, filename.c_str(), static_cast<int>(filename.size()), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        std::cerr << "Could not open file: " << filepath << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    char buffer[4096];
    while (file.good()) {
        file.read(buffer, sizeof(buffer));
        int bytesRead = static_cast<int>(file.gcount());
        if (bytesRead > 0) {
            int sent = send(sock, buffer, bytesRead, 0);
            if (sent == SOCKET_ERROR) {
                std::cerr << "File upload interrupted. Please retry.\n";
                file.close();
                closesocket(sock);
                WSACleanup();
                return 1;
            }
        }
    }

    file.close();
    std::cout << "File uploaded successfully.\n";

    closesocket(sock);
    WSACleanup();
    return 0;
}
