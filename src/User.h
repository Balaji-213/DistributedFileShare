#ifndef USER_H
#define USER_H

#include <string>
#include <vector>

struct UserInfo {
    int userId;
    std::string username;
    std::string email;
};

class User {
public:
    static bool registerUser(const std::string& username, const std::string& password, const std::string& email = "");
    static bool authenticateUser(const std::string& username, const std::string& password);
    static std::string createSession(int userId);
    static bool validateSession(const std::string& sessionToken, int& userId);
    static bool getUserInfo(int userId, UserInfo& userInfo);
    static void cleanupExpiredSessions();
};

#endif
