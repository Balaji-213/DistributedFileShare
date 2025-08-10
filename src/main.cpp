#include <iostream>
#include <string>
#include "Database.h"
#include "User.h"
#include "FileManager.h"
#include "WebServer.h"
#include "Utils.h"

void printMenu() {
    std::cout << "\n=== Distributed File Sharing System ===\n";
    std::cout << "1. Start Web Server\n";
    std::cout << "2. Register User (CLI)\n";
    std::cout << "3. Login User (CLI)\n";
    std::cout << "4. Exit\n";
    std::cout << "Choose option: ";
}

int main() {
    std::cout << "Initializing Distributed File Sharing System...\n";
    
    // Initialize database
    if (!Database::getInstance().initialize()) {
        std::cerr << "Failed to initialize database. Please check YugabyteDB is running.\n";
        return 1;
    }
    
    // Create uploads directory
    Utils::createDirectory("./uploads/");
    
    std::cout << "System initialized successfully!\n";
    
    int choice = 0;
    WebServer* server = nullptr;
    
    while (true) {
        printMenu();
        std::cin >> choice;
        std::cin.ignore();
        
        switch (choice) {
            case 1: {
                if (!server) {
                    server = new WebServer(8080);
                    server->start();
                    std::cout << "Web server started. Access at http://localhost:8080\n";
                    std::cout << "Press Enter to continue...\n";
                    std::cin.get();
                } else {
                    std::cout << "Server already running!\n";
                }
                break;
            }
            
            case 2: {
                std::string username, password, email;
                std::cout << "Enter username: ";
                std::getline(std::cin, username);
                std::cout << "Enter password: ";
                std::getline(std::cin, password);
                std::cout << "Enter email (optional): ";
                std::getline(std::cin, email);
                
                User::registerUser(username, password, email);
                break;
            }
            
            case 3: {
                std::string username, password;
                std::cout << "Enter username: ";
                std::getline(std::cin, username);
                std::cout << "Enter password: ";
                std::getline(std::cin, password);
                
                if (User::authenticateUser(username, password)) {
                    std::cout << "Login successful!\n";
                } else {
                    std::cout << "Login failed!\n";
                }
                break;
            }
            
            case 4: {
                if (server) {
                    server->stop();
                    delete server;
                }
                std::cout << "Goodbye!\n";
                return 0;
            }
            
            default:
                std::cout << "Invalid choice!\n";
        }
    }
    
    return 0;
}
