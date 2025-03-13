#include <iostream>
#include <string>
#include <map>
#include <queue>
#include <set>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <thread>

#define BUFSIZE 512

std::map<std::string, int> clientPipes; // Очереди для клиентов
std::map<std::string, std::queue<std::string>> messageQueues; // Очереди сообщений
std::map<std::string, std::set<std::string>> groups; // Группы и их участники

void HandleClient(int pipeFd, const std::string& clientName) {
    char buffer[BUFSIZE];
    while (true) {
        ssize_t bytesRead = read(pipeFd, buffer, BUFSIZE);
        if (bytesRead > 0) {
            std::string message(buffer, bytesRead);
            std::cout << "Received from " << clientName << ": " << message << std::endl;

            // Обработка сообщений
            if (message.find("TO:") == 0) {
                // Личное сообщение
                size_t pos = message.find(" ");
                std::string recipient = message.substr(3, pos - 3);
                std::string msg = message.substr(pos + 1);

                if (clientPipes.find(recipient) != clientPipes.end()) {
                    write(clientPipes[recipient], msg.c_str(), msg.size());
                } else {
                    messageQueues[recipient].push(msg);
                }
            } else if (message.find("GROUP:") == 0) {
                // Сообщение в группу
                size_t pos = message.find(" ");
                std::string groupName = message.substr(6, pos - 6);
                std::string msg = message.substr(pos + 1);

                if (groups.find(groupName) != groups.end()) {
                    for (const auto& member : groups[groupName]) {
                        if (clientPipes.find(member) != clientPipes.end()) {
                            write(clientPipes[member], msg.c_str(), msg.size());
                        } else {
                            messageQueues[member].push(msg);
                        }
                    }
                }
            }
        }
    }
}

void CreateQueue(const std::string& clientName) {
    std::string pipeName = "/tmp/" + clientName;
    mkfifo(pipeName.c_str(), 0666);

    int pipeFd = open(pipeName.c_str(), O_RDWR);
    if (pipeFd == -1) {
        std::cerr << "Failed to create pipe for " << clientName << std::endl;
        return;
    }

    clientPipes[clientName] = pipeFd;
    std::thread(HandleClient, pipeFd, clientName).detach();
}

void DeleteQueue(const std::string& clientName) {
    if (clientPipes.find(clientName) != clientPipes.end()) {
        close(clientPipes[clientName]);
        clientPipes.erase(clientName);
        std::string pipeName = "/tmp/" + clientName;
        unlink(pipeName.c_str());
    }
}

void CreateGroup(const std::string& groupName) {
    if (groups.find(groupName) == groups.end()) {
        groups[groupName] = std::set<std::string>();
        std::cout << "Group created: " << groupName << std::endl;
    } else {
        std::cerr << "Group already exists: " << groupName << std::endl;
    }
}

void JoinGroup(const std::string& groupName, const std::string& clientName) {
    if (groups.find(groupName) != groups.end()) {
        groups[groupName].insert(clientName);
        std::cout << "Client " << clientName << " joined group " << groupName << std::endl;
    } else {
        std::cerr << "Group does not exist: " << groupName << std::endl;
    }
}

int main() {
    std::string command;
    while (true) {
        std::cout << "Enter command (create/delete/creategroup/joingroup/exit): ";
        std::cin >> command;

        if (command == "create") {
            std::string clientName;
            std::cout << "Enter client name: ";
            std::cin >> clientName;
            CreateQueue(clientName);
        } else if (command == "delete") {
            std::string clientName;
            std::cout << "Enter client name: ";
            std::cin >> clientName;
            DeleteQueue(clientName);
        } else if (command == "creategroup") {
            std::string groupName;
            std::cout << "Enter group name: ";
            std::cin >> groupName;
            CreateGroup(groupName);
        } else if (command == "joingroup") {
            std::string groupName, clientName;
            std::cout << "Enter group name: ";
            std::cin >> groupName;
            std::cout << "Enter client name: ";
            std::cin >> clientName;
            JoinGroup(groupName, clientName);
        } else if (command == "exit") {
            break;
        }
    }

    return 0;
}