#include "FileManager.h"
#include "Database.h"
#include "Utils.h"
#include <Poco/Data/Statement.h>
#include <Poco/Exception.h>
#include <Poco/DateTime.h>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <fstream>
#include <iostream>

using namespace Poco::Data::Keywords;

std::string FileManager::getUploadsDirectory() {
    return "./uploads/";
}

bool FileManager::saveFileToDisk(const std::string& filename, const std::string& content) {
    try {
        std::string fullPath = getUploadsDirectory() + filename;
        std::ofstream file(fullPath, std::ios::binary);
        if (!file) return false;
        
        file.write(content.c_str(), content.length());
        file.close();
        return true;
    }
    catch (...) {
        return false;
    }
}

bool FileManager::loadFileFromDisk(const std::string& filename, std::string& content) {
    try {
        std::string fullPath = getUploadsDirectory() + filename;
        std::ifstream file(fullPath, std::ios::binary);
        if (!file) return false;
        
        file.seekg(0, std::ios::end);
        content.reserve(file.tellg());
        file.seekg(0, std::ios::beg);
        
        content.assign((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
        file.close();
        return true;
    }
    catch (...) {
        return false;
    }
}

int FileManager::uploadFile(const std::string& originalFilename, const std::string& content, 
                           const std::string& contentType, int ownerId) {
    try {
        // Generate unique filename
        std::string uniqueFilename = Utils::generateUniqueFilename(originalFilename);
        
        // Save to disk
        if (!saveFileToDisk(uniqueFilename, content)) {
            return -1;
        }
        
        // Save metadata to database
        auto session = Database::getInstance().getSession();
        std::string filePath = getUploadsDirectory() + uniqueFilename;
        long fileSize = content.length();
        
        // Create non-const variables for binding
        std::string fname = uniqueFilename;
        std::string origName = originalFilename;
        std::string fpath = filePath;
        std::string ctype = contentType;
        
        Poco::Data::Statement insert(session);
        insert << "INSERT INTO files (filename, original_filename, file_path, file_size, content_type, owner_id) "
                  "VALUES ($1, $2, $3, $4, $5, $6)",
            use(fname), use(origName), use(fpath), use(fileSize), 
            use(ctype), use(ownerId);
        insert.execute();
        
        // Get the file_id
        int fileId = 0;
        Poco::Data::Statement getId(session);
        getId << "SELECT file_id FROM files WHERE filename = $1 AND owner_id = $2 ORDER BY file_id DESC LIMIT 1",  // ← Fixed: $1, $2 instead of ?
            use(fname), use(ownerId), into(fileId);
        getId.execute();
        
        std::cout << "File uploaded successfully with ID: " << fileId << std::endl;
        return fileId;
    }
    catch (const Poco::Exception& ex) {
        std::cerr << "File upload failed: " << ex.displayText() << std::endl;
        return -1;
    }
}

bool FileManager::downloadFile(int fileId, int requesterId, std::string& content, FileInfo& info) {
    try {
        auto session = Database::getInstance().getSession();
        
        // Get file metadata
        std::string filename, originalFilename, contentType, uploadDate;
        int ownerId;
        long fileSize;
        bool isPublic;
        
        Poco::Data::Statement select(session);
        select << "SELECT filename, original_filename, file_size, content_type, owner_id, upload_date, is_public "
                  "FROM files WHERE file_id = $1",
            use(fileId), into(filename), into(originalFilename), into(fileSize), 
            into(contentType), into(ownerId), into(uploadDate), into(isPublic), limit(1);
        select.execute();
        
        if (filename.empty()) return false;
        
        // Check access permissions
        bool hasAccess = false;
        
        if (ownerId == requesterId) {
            // User owns the file
            hasAccess = true;
        }
        else if (isPublic) {
            // File is public
            hasAccess = true;
        }
        else if (requesterId == 0) {
            // Shared access - already validated by accessSharedFile
            hasAccess = true;  // ✅ THIS IS THE KEY FIX
        }
        else if (requesterId > 0) {
            // Check if file is specifically shared with this user
            int shareCount = 0;
            Poco::Data::Statement shareCheck(session);
            shareCheck << "SELECT COUNT(*) FROM file_shares WHERE file_id = $1 AND shared_with = $2 AND (expires_at IS NULL OR expires_at > CURRENT_TIMESTAMP)",
                use(fileId), use(requesterId), into(shareCount);
            shareCheck.execute();
            hasAccess = (shareCount > 0);
        }
        
        if (!hasAccess) return false;
        
        // Load file content
        if (!loadFileFromDisk(filename, content)) return false;
        
        // Fill file info
        info.fileId = fileId;
        info.filename = filename;
        info.originalFilename = originalFilename;
        info.fileSize = fileSize;
        info.contentType = contentType;
        info.ownerId = ownerId;
        info.uploadDate = uploadDate;
        info.isPublic = isPublic;
        
        return true;
    }
    catch (const Poco::Exception& ex) {
        std::cerr << "File download failed: " << ex.displayText() << std::endl;
        return false;
    }
}


std::vector<FileInfo> FileManager::getUserFiles(int userId) {
    std::vector<FileInfo> files;
    
    try {
        auto session = Database::getInstance().getSession();
        
        // Use individual variables instead of RecordSet
        std::vector<int> fileIds;
        std::vector<std::string> filenames;
        std::vector<std::string> originalFilenames;
        std::vector<long> fileSizes;
        std::vector<std::string> contentTypes;
        std::vector<int> ownerIds;
        std::vector<std::string> uploadDates;
        std::vector<bool> isPublicFlags;
        
        Poco::Data::Statement select(session);
        select << "SELECT file_id, filename, original_filename, file_size, content_type, owner_id, upload_date, is_public "
                  "FROM files WHERE owner_id = $1 ORDER BY upload_date DESC",  // ← Fixed: $1 instead of ?
            use(userId), 
            into(fileIds), into(filenames), into(originalFilenames), into(fileSizes),
            into(contentTypes), into(ownerIds), into(uploadDates), into(isPublicFlags);
        select.execute();
        
        // Create FileInfo objects from vectors
        for (size_t i = 0; i < fileIds.size(); ++i) {
            FileInfo info;
            info.fileId = fileIds[i];
            info.filename = filenames[i];
            info.originalFilename = originalFilenames[i];
            info.fileSize = fileSizes[i];
            info.contentType = contentTypes[i];
            info.ownerId = ownerIds[i];
            info.uploadDate = uploadDates[i];
            info.isPublic = isPublicFlags[i];
            files.push_back(info);
        }
    }
    catch (const Poco::Exception& ex) {
        std::cerr << "Get user files failed: " << ex.displayText() << std::endl;
    }
    
    return files;
}

std::string FileManager::shareFile(int fileId, int ownerId, int sharedWithUserId, const std::string& expiryHours) {
    try {
        auto session = Database::getInstance().getSession();
        
        // Verify file ownership
        int actualOwnerId = 0;
        Poco::Data::Statement ownerCheck(session);
        ownerCheck << "SELECT owner_id FROM files WHERE file_id = $1",
            use(fileId), into(actualOwnerId), limit(1);
        ownerCheck.execute();
        
        if (actualOwnerId != ownerId) return "";
        
        // ✅ CORRECT: Check for existing share of THIS SPECIFIC FILE to THIS SPECIFIC USER
        if (sharedWithUserId > 0) {
            std::string existingToken;
            Poco::Data::Statement existingCheck(session);
            existingCheck << "SELECT share_token FROM file_shares "
                            "WHERE file_id = $1 AND shared_with = $2 "  // ← Both conditions ensure file-specific check
                            "AND (expires_at IS NULL OR expires_at > CURRENT_TIMESTAMP) "
                            "LIMIT 1",
                use(fileId), use(sharedWithUserId), into(existingToken);
            existingCheck.execute();
            
            if (!existingToken.empty()) {
                std::cout << "File " << fileId << " already shared with user " << sharedWithUserId 
                         << ". Returning existing token." << std::endl;
                return existingToken;  // Return existing share token for THIS file
            }
        }
        
        // Generate new share token (no existing share of THIS file to THIS user)
        std::string shareToken = Utils::generateShareToken();
        
        // Calculate expiry
        Poco::DateTime expiry;
        expiry += Poco::Timespan(0, std::stoi(expiryHours), 0, 0, 0);
        
        // Insert new share record
        Poco::Data::Statement insert(session);
        if (sharedWithUserId > 0) {
            insert << "INSERT INTO file_shares (file_id, shared_by, shared_with, share_token, expires_at) "
                      "VALUES ($1, $2, $3, $4, $5)",
                use(fileId), use(ownerId), use(sharedWithUserId), use(shareToken), use(expiry);
        } else {
            insert << "INSERT INTO file_shares (file_id, shared_by, share_token, expires_at) "
                      "VALUES ($1, $2, $3, $4)",
                use(fileId), use(ownerId), use(shareToken), use(expiry);
        }
        insert.execute();
        
        std::cout << "Created new share for file " << fileId << " to user " << sharedWithUserId << std::endl;
        return shareToken;
    }
    catch (const Poco::Exception& ex) {
        std::cerr << "File sharing failed: " << ex.displayText() << std::endl;
        return "";
    }
}


bool FileManager::accessSharedFile(const std::string& shareToken, 
                                   int requesterId,           // ← NEW PARAMETER
                                   std::string& content, 
                                   FileInfo& info) {
    try {
        auto session = Database::getInstance().getSession();
        
        // Get file info from share token
        int fileId = 0;          // Initialize to 0
        int sharedWith = 0;      // ← NEW: Get shared_with column
        std::string token = shareToken;
        
        Poco::Data::Statement select(session);
        select << "SELECT fs.file_id, COALESCE(fs.shared_with, 0) "
                  "FROM file_shares fs WHERE fs.share_token = $1 AND "
                  "(fs.expires_at IS NULL OR fs.expires_at > CURRENT_TIMESTAMP)",
            use(token), into(fileId), into(sharedWith), limit(1);  // ← Get both values
        select.execute();
        
        if (fileId == 0) return false;  // No such token or expired
        
        // ← NEW: Check user-specific sharing permissions
        if (sharedWith > 0 && sharedWith != requesterId) {
            std::cerr << "Share is private and not accessible to user " << requesterId << std::endl;
            return false;
        }
        
        // Get file details and content
        return downloadFile(fileId, requesterId, content, info);  // ← Pass actual requesterId
    }
    catch (const Poco::Exception& ex) {
        std::cerr << "Shared file access failed: " << ex.displayText() << std::endl;
        return false;
    }
}


bool FileManager::deleteFile(int fileId, int ownerId) {
    try {
        auto session = Database::getInstance().getSession();
        
        // Get filename before deletion
        std::string filename;
        Poco::Data::Statement getFile(session);
        getFile << "SELECT filename FROM files WHERE file_id = $1 AND owner_id = $2",  // ← Fixed: $1, $2 instead of ?
            use(fileId), use(ownerId), into(filename), limit(1);
        getFile.execute();
        
        if (filename.empty()) return false;
        
        // Delete from database
        Poco::Data::Statement deleteStmt(session);
        deleteStmt << "DELETE FROM files WHERE file_id = $1 AND owner_id = $2",  // ← Fixed: $1, $2 instead of ?
            use(fileId), use(ownerId);
        deleteStmt.execute();
        
        // Delete file from disk
        std::string fullPath = getUploadsDirectory() + filename;
        Poco::File file(fullPath);
        if (file.exists()) {
            file.remove();
        }
        
        return true;
    }
    catch (const Poco::Exception& ex) {
        std::cerr << "File deletion failed: " << ex.displayText() << std::endl;
        return false;
    }
}

bool FileManager::setFilePublic(int fileId, int ownerId, bool isPublic) {
    try {
        auto session = Database::getInstance().getSession();
        
        Poco::Data::Statement update(session);
        update << "UPDATE files SET is_public = $1 WHERE file_id = $2 AND owner_id = $3",  // ← Fixed: $1, $2, $3 instead of ?
            use(isPublic), use(fileId), use(ownerId);
        update.execute();
        
        return true;
    }
    catch (const Poco::Exception& ex) {
        std::cerr << "Set file public failed: " << ex.displayText() << std::endl;
        return false;
    }
}
