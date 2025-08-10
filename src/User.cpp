#include "User.h"
#include "Database.h"
#include "Utils.h"
#include <Poco/Data/Statement.h>
#include <Poco/Exception.h>
#include <Poco/DateTime.h>
#include <iostream>

using namespace Poco::Data::Keywords;

bool User::registerUser(const std::string& username, const std::string& password, const std::string& email) {
    try {
        auto session = Database::getInstance().getSession();
        std::string hashedPassword = Utils::hashPassword(password);
        
        // Create non-const variables for binding
        std::string user = username;
        std::string pass = hashedPassword;
        std::string mail = email;
        
        Poco::Data::Statement insert(session);
        insert << "INSERT INTO users (username, password_hash, email) VALUES ($1, $2, $3)",
            use(user), use(pass), use(mail);
        insert.execute();
        
        // Get the newly created user ID
        int newUserId = 0;
        Poco::Data::Statement getId(session);
        getId << "SELECT user_id FROM users WHERE username = $1",
            use(user), into(newUserId), limit(1);
        getId.execute();
        
        std::cout << "User '" << username << "' registered successfully with ID: " << newUserId << std::endl;
        return true;
    }
    catch (const Poco::Exception& ex) {
        std::cerr << "Registration failed: " << ex.displayText() << std::endl;
        return false;
    }
}

bool User::authenticateUser(const std::string& username, const std::string& password) {
    try {
        auto session = Database::getInstance().getSession();
        
        std::string storedHash;
        int userId = 0;  // ← ADD: Variable to store user ID
        std::string user = username;
        
        Poco::Data::Statement select(session);
        select << "SELECT password_hash, user_id FROM users WHERE username = $1",  // ← MODIFIED: Get both hash and ID
            use(user), into(storedHash), into(userId), limit(1);  // ← MODIFIED: Retrieve both values
        select.execute();
        
        if (storedHash.empty()) {
            std::cout << "Authentication failed: User '" << username << "' not found" << std::endl;
            return false;
        }
        
        bool isAuthenticated = Utils::hashPassword(password) == storedHash;
        
        if (isAuthenticated) {
            // ← ADD: Print success message with user ID
            std::cout << "User '" << username << "' authenticated successfully with ID: " << userId << std::endl;
        } else {
            // ← ADD: Print failure message
            std::cout << "Authentication failed: Invalid password for user '" << username << "' (ID: " << userId << ")" << std::endl;
        }
        
        return isAuthenticated;
    }
    catch (const Poco::Exception& ex) {
        std::cerr << "Authentication failed: " << ex.displayText() << std::endl;
        return false;
    }
}


std::string User::createSession(int userId) {
    try {
        auto session = Database::getInstance().getSession();
        std::string sessionToken = Utils::generateSessionToken();
        
        // Set expiry to 24 hours from now
        Poco::DateTime expiry;
        expiry += Poco::Timespan(1, 0, 0, 0, 0); // 1 day
        
        Poco::Data::Statement insert(session);
        insert << "INSERT INTO user_sessions (session_id, user_id, expires_at) VALUES ($1, $2, $3)",
            use(sessionToken), use(userId), use(expiry);
        insert.execute();
        
        return sessionToken;
    }
    catch (const Poco::Exception& ex) {
        std::cerr << "Session creation failed: " << ex.displayText() << std::endl;
        return "";
    }
}

bool User::validateSession(const std::string& sessionToken, int& userId) {
    try {
        auto session = Database::getInstance().getSession();
        
        Poco::DateTime now;
        std::string token = sessionToken;
        
        Poco::Data::Statement select(session);
        select << "SELECT user_id FROM user_sessions WHERE session_id = $1 AND expires_at > $2",  // ← Fixed: $1, $2 instead of ?
            use(token), use(now), into(userId), limit(1);
        select.execute();
        
        return userId > 0;
    }
    catch (const Poco::Exception& ex) {
        return false;
    }
}

bool User::getUserInfo(int userId, UserInfo& userInfo) {
    try {
        auto session = Database::getInstance().getSession();
        
        Poco::Data::Statement select(session);
        select << "SELECT user_id, username, email FROM users WHERE user_id = $1",  // ← Fixed: $1 instead of ?
            use(userId), into(userInfo.userId), into(userInfo.username), into(userInfo.email), limit(1);
        select.execute();
        
        return userInfo.userId > 0;
    }
    catch (const Poco::Exception& ex) {
        return false;
    }
}

void User::cleanupExpiredSessions() {
    try {
        auto session = Database::getInstance().getSession();
        Poco::DateTime now;
        
        Poco::Data::Statement cleanup(session);
        cleanup << "DELETE FROM user_sessions WHERE expires_at < $1", use(now);  // ← Fixed: $1 instead of ?
        cleanup.execute();
    }
    catch (const Poco::Exception& ex) {
        std::cerr << "Session cleanup failed: " << ex.displayText() << std::endl;
    }
}
