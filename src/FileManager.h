#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include <vector>

struct FileInfo {
    int fileId;
    std::string filename;
    std::string originalFilename;
    long fileSize;
    std::string contentType;
    int ownerId;
    std::string uploadDate;
    bool isPublic;
};

class FileManager {
public:
    static int uploadFile(const std::string& filename, const std::string& content, 
                         const std::string& contentType, int ownerId);
    static bool downloadFile(int fileId, int requesterId, std::string& content, FileInfo& info);
    static bool deleteFile(int fileId, int ownerId);
    static std::vector<FileInfo> getUserFiles(int userId);
    static std::string shareFile(int fileId, int ownerId, int sharedWithUserId = 0, 
                                const std::string& expiryHours = "24");
    static bool accessSharedFile(const std::string& shareToken, int requesterId, std::string& content, FileInfo& info);
    static bool setFilePublic(int fileId, int ownerId, bool isPublic);
    
private:
    static std::string getUploadsDirectory();
    static bool saveFileToDisk(const std::string& filename, const std::string& content);
    static bool loadFileFromDisk(const std::string& filename, std::string& content);
};

#endif
