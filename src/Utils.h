#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <random>

class Utils {
public:
    static std::string hashPassword(const std::string& password);
    static std::string generateSessionToken();
    static std::string generateShareToken();
    static std::string generateUniqueFilename(const std::string& originalName);
    static bool createDirectory(const std::string& path);
};

#endif
