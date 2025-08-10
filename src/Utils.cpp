#include "Utils.h"
#include <Poco/Crypto/DigestEngine.h>
#include <Poco/DigestStream.h>
#include <Poco/UUIDGenerator.h>
#include <Poco/DateTime.h>
#include <Poco/File.h>
#include <sstream>
#include <iomanip>
#include <iostream>

std::string Utils::hashPassword(const std::string& password) {
    try {
        Poco::Crypto::DigestEngine digest("SHA256");
        digest.update(password.c_str(), password.length());
        
        // Get the digest result
        const Poco::Crypto::DigestEngine::Digest& result = digest.digest();
        
        // Convert to hex string
        std::ostringstream oss;
        for (const auto& byte : result) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte);
        }
        
        return oss.str();
    }
    catch (const std::exception& ex) {
        std::cerr << "Password hashing failed: " << ex.what() << std::endl;
        return ""; // Return empty string on error
    }
}

std::string Utils::generateSessionToken() {
    Poco::UUIDGenerator& generator = Poco::UUIDGenerator::defaultGenerator();
    return generator.createRandom().toString();
}

std::string Utils::generateShareToken() {
    Poco::UUIDGenerator& generator = Poco::UUIDGenerator::defaultGenerator();
    return generator.createRandom().toString();
}

std::string Utils::generateUniqueFilename(const std::string& originalName) {
    Poco::DateTime now;
    std::stringstream ss;
    ss << now.timestamp().epochMicroseconds() << "_" << originalName;
    return ss.str();
}

bool Utils::createDirectory(const std::string& path) {
    try {
        Poco::File dir(path);
        if (!dir.exists()) {
            dir.createDirectories();
        }
        return true;
    }
    catch (...) {
        return false;
    }
}
