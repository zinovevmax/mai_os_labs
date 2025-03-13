#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <thread>

#define BUFSIZE 512

void ConnectToQueue(const std::string& clientName) {
    std::string pipeName = "/tmp/" + clientName;
    int pipeFd = open(pipeName.c_str(), O_RDONLY);
    if (pipeFd == -1) {
        std::cerr << "Failed to connect to pipe for " << clientName << std::endl;
        return;
    }

    char buffer[BUFSIZE];
    while (true) {
        ssize_t bytesRead = read(pipeFd, buffer, BUFSIZE);
        if (bytesRead > 0) {
            std::string message(buffer, bytesRead);
            std::cout << "Received: " << message << std::endl;
        }
    }
}

void SendMessage(const std::string& recipient, const std::string& message) {
    std::string pipeName = "/tmp/" + recipient;
    int pipeFd = open(pipeName.c_str(), O_WRONLY);
    if (pipeFd == -1) {
        std::cerr << "Failed to connect to pipe for " << recipient << std::endl;
        return;
    }

    write(pipeFd, message.c_str(), message.size());
    close(pipeFd);
}

void SendGroupMessage(const std::string& groupName, const std::string& message) {
    std::string pipeName = "/tmp/server"; // Предполагаем, что сервер слушает этот pipe
    int pipeFd = open(pipeName.c_str(), O_WRONLY);
    if (pipeFd == -1) {
        std::cerr << "Failed to connect to server pipe" << std::endl;
        return;
    }

    std::string fullMessage = "GROUP:" + groupName + " " + message;
    write(pipeFd, fullMessage.c_str(), fullMessage.size());
    close(pipeFd);
}

int main() {
    std::string clientName;
    std::cout << "Enter your client name: ";
    std::cin >> clientName;

    std::thread(ConnectToQueue, clientName).detach();

    std::string command;
    while (true) {
        std::cout << "Enter command (send/sendgroup/exit): ";
        std::cin >> command;

        if (command == "send") {
            std::string recipient, message;
            std::cout << "Enter recipient: ";
            std::cin >> recipient;
            std::cout << "Enter message: ";
            std::cin.ignore();
            std::getline(std::cin, message);
            SendMessage(recipient, "TO:" + recipient + " " + message);
        } else if (command == "sendgroup") {
            std::string groupName, message;
            std::cout << "Enter group name: ";
            std::cin >> groupName;
            std::cout << "Enter message: ";
            std::cin.ignore();
            std::getline(std::cin, message);
            SendGroupMessage(groupName, message);
        } else if (command == "exit") {
            break;
        }
    }

    return 0;
}