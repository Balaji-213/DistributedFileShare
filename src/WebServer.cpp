#include "WebServer.h"
#include "User.h" 
#include "FileManager.h"
#include "Database.h"
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/URI.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Parser.h>
#include <Poco/StreamCopier.h>
#include <Poco/Data/Statement.h>
// Remove the problematic include: #include <Poco/Data/Keywords.h>
#include <iostream>
#include <sstream>

using namespace Poco::Net;
using namespace Poco::JSON;
// Remove the problematic using: using namespace Poco::Data::Keywords;

void FileShareRequestHandler::handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
    Poco::URI uri(request.getURI());
    std::string path = uri.getPath();
    
    std::cout << "Request: " << request.getMethod() << " " << path << std::endl;

    // ✅ CRITICAL: Set CORS headers for ALL requests
    setCORSHeaders(response);
    
    // ✅ CRITICAL: Handle OPTIONS preflight requests
    if (request.getMethod() == "OPTIONS") {
        std::cout << "Handling OPTIONS preflight request for: " << path << std::endl;
        response.setStatus(HTTPResponse::HTTP_OK);
        response.setContentLength(0);
        response.send();
        return;
    }
    
    try {
        if (path == "/register" && request.getMethod() == "POST") {
            handleRegister(request, response);
        }
        else if (path == "/login" && request.getMethod() == "POST") {
            handleLogin(request, response);
        }
        else if (path == "/upload" && request.getMethod() == "POST") {
            handleUpload(request, response);
        }
        else if (path.find("/download/") == 0 && request.getMethod() == "GET") {
            handleDownload(request, response);
        }
        else if (path == "/share" && request.getMethod() == "POST") {
            handleShare(request, response);
        }
        else if (path == "/files" && request.getMethod() == "GET") {
            handleList(request, response);
        }
        else if (path.find("/shared/") == 0 && request.getMethod() == "GET") { 
            handleSharedFileAccess(request, response);                          
        } 
        else if (path == "/shared-with-me" && request.getMethod() == "GET") {
            handleSharedWithMe(request, response);
        }                                                                      // ← ADD THIS LINE
        else {
            sendErrorResponse(response, "Not Found", 404);
        }
    }
    catch (const std::exception& ex) {
        sendErrorResponse(response, ex.what(), 500);
    }
}

void FileShareRequestHandler::handleRegister(HTTPServerRequest& request, HTTPServerResponse& response) {
    std::string body;
    Poco::StreamCopier::copyToString(request.stream(), body);
    
    Parser parser;
    auto result = parser.parse(body);
    Object::Ptr object = result.extract<Object::Ptr>();
    
    std::string username = object->getValue<std::string>("username");
    std::string password = object->getValue<std::string>("password");
    std::string email = object->optValue<std::string>("email", "");

    // ✅ PRE-VALIDATION: Check for existing username
    if (isUsernameExists(username)) {
        sendErrorResponse(response, "Username already exists. Please choose a different username.", 409);
        return;
    }
    
    if (User::registerUser(username, password, email)) {
        // After successful registration, get the user ID
        int userId = 0;
        try {
            auto session = Database::getInstance().getSession();
            std::string user = username;
            
            Poco::Data::Statement select(session);
            select << "SELECT user_id FROM users WHERE username = $1",
                Poco::Data::Keywords::use(user), Poco::Data::Keywords::into(userId), Poco::Data::Keywords::limit(1);
            select.execute();
            
            std::cout << "User '" << username << "' registered with ID: " << userId << std::endl;
        }
        catch (const Poco::Exception& ex) {
            std::cerr << "Failed to retrieve user ID: " << ex.displayText() << std::endl;
        }
        
        Object response_obj;
        response_obj.set("success", true);
        response_obj.set("message", "User registered successfully");
        if (userId > 0) {
            response_obj.set("user_id", userId);
        }
        
        std::stringstream ss;
        response_obj.stringify(ss);
        sendJSONResponse(response, ss.str());
    } else {
        sendErrorResponse(response, "Registration failed");
    }
}

void FileShareRequestHandler::handleLogin(HTTPServerRequest& request, HTTPServerResponse& response) {
    std::string body;
    Poco::StreamCopier::copyToString(request.stream(), body);
    
    Parser parser;
    auto result = parser.parse(body);
    Object::Ptr object = result.extract<Object::Ptr>();
    
    std::string username = object->getValue<std::string>("username");
    std::string password = object->getValue<std::string>("password");
    
    if (User::authenticateUser(username, password)) {
        // Get user ID
        auto session = Database::getInstance().getSession();
        int userId = 0;
        std::string user = username;
        
        Poco::Data::Statement select(session);
        select << "SELECT user_id FROM users WHERE username = $1",
            Poco::Data::Keywords::use(user), Poco::Data::Keywords::into(userId), Poco::Data::Keywords::limit(1);
        select.execute();
        
        std::string sessionToken = User::createSession(userId);
        
        Object response_obj;
        response_obj.set("success", true);
        response_obj.set("session_token", sessionToken);
        response_obj.set("user_id", userId);
        
        std::stringstream ss;
        response_obj.stringify(ss);
        sendJSONResponse(response, ss.str());
    } else {
        sendErrorResponse(response, "Authentication failed", 401);
    }
}

void FileShareRequestHandler::handleUpload(HTTPServerRequest& request, HTTPServerResponse& response) {
    int userId;
    if (!authenticateRequest(request, userId)) {
        sendErrorResponse(response, "Unauthorized", 401);
        return;
    }
    
    std::string contentType = request.getContentType();
    std::string filename = request.get("X-Filename", "uploaded_file");
    
    std::string content;
    Poco::StreamCopier::copyToString(request.stream(), content);
    
    int fileId = FileManager::uploadFile(filename, content, contentType, userId);
    
    if (fileId > 0) {
        Object response_obj;
        response_obj.set("success", true);
        response_obj.set("file_id", fileId);
        response_obj.set("message", "File uploaded successfully");
        
        std::stringstream ss;
        response_obj.stringify(ss);
        sendJSONResponse(response, ss.str());
    } else {
        sendErrorResponse(response, "Upload failed");
    }
}

void FileShareRequestHandler::handleDownload(HTTPServerRequest& request, HTTPServerResponse& response) {
    Poco::URI uri(request.getURI());
    std::string path = uri.getPath();
    
    // Extract file ID from path /download/{id}
    std::string fileIdStr = path.substr(10); // Remove "/download/"
    int fileId = std::stoi(fileIdStr);
    
    int userId = 0;
    authenticateRequest(request, userId); // Optional for public files
    
    std::string content;
    FileInfo info;
    
    if (FileManager::downloadFile(fileId, userId, content, info)) {
        response.setContentType(info.contentType);
        response.setContentLength(content.length());
        response.set("Content-Disposition", "attachment; filename=\"" + info.originalFilename + "\"");
        
        std::ostream& out = response.send();
        out << content;
    } else {
        sendErrorResponse(response, "File not found or access denied", 404);
    }
}

void FileShareRequestHandler::handleShare(HTTPServerRequest& request, HTTPServerResponse& response) {
    int userId;
    if (!authenticateRequest(request, userId)) {
        sendErrorResponse(response, "Unauthorized", 401);
        return;
    }
    
    std::string body;
    Poco::StreamCopier::copyToString(request.stream(), body);
    
    Parser parser;
    auto result = parser.parse(body);
    Object::Ptr object = result.extract<Object::Ptr>();
    
    int fileId = object->getValue<int>("file_id");
    std::string expiryHours = object->optValue<std::string>("expiry_hours", "24");

    // NEW: Get optional shared_with_user_id
    int sharedWithUserId = 0;
    if (object->has("shared_with_user_id")) {
        sharedWithUserId = object->getValue<int>("shared_with_user_id");
    }
    
    std::string shareToken = FileManager::shareFile(fileId, userId, sharedWithUserId, expiryHours);
    
    if (!shareToken.empty()) {
        Object response_obj;
        response_obj.set("success", true);
        response_obj.set("share_token", shareToken);
        response_obj.set("share_url", "http://localhost:8080/shared/" + shareToken);
        
        std::stringstream ss;
        response_obj.stringify(ss);
        sendJSONResponse(response, ss.str());
    } else {
        sendErrorResponse(response, "Share generation failed");
    }
}

void FileShareRequestHandler::handleSharedFileAccess(HTTPServerRequest& request, HTTPServerResponse& response) {
    Poco::URI uri(request.getURI());
    std::string path = uri.getPath();
    
    // Extract share token from path /shared/{token}
    std::string shareToken = path.substr(8); // Remove "/shared/"
    
    if (shareToken.empty()) {
        sendErrorResponse(response, "Share token is required", 400);
        return;
    }

    int userId = 0;  // 0 means anonymous access
    authenticateRequest(request, userId);  // This sets userId if auth succeeds, otherwise stays 0
    
    std::string content;
    FileInfo info;
    
    if (FileManager::accessSharedFile(shareToken, userId, content, info)) {
        response.setContentType(info.contentType);
        response.setContentLength(content.length());
        response.set("Content-Disposition", "attachment; filename=\"" + info.originalFilename + "\"");
        
        std::ostream& out = response.send();
        out << content;
    } else {
        if (userId == 0) {
            // Might be a private share requiring authentication
            sendErrorResponse(response, "Authentication required for this shared file", 401);
        } else {
            sendErrorResponse(response, "Shared file not found, expired, or access denied", 404);
        }
    }
}

void FileShareRequestHandler::handleList(HTTPServerRequest& request, HTTPServerResponse& response) {
    int userId;
    if (!authenticateRequest(request, userId)) {
        sendErrorResponse(response, "Unauthorized", 401);
        return;
    }
    
    auto files = FileManager::getUserFiles(userId);
    
    Array filesArray;
    for (const auto& file : files) {
        Object fileObj;
        fileObj.set("file_id", file.fileId);
        fileObj.set("filename", file.originalFilename);
        fileObj.set("size", file.fileSize);
        fileObj.set("content_type", file.contentType);
        fileObj.set("upload_date", file.uploadDate);
        fileObj.set("is_public", file.isPublic);
        filesArray.add(fileObj);
    }
    
    Object response_obj;
    response_obj.set("success", true);
    response_obj.set("files", filesArray);
    
    std::stringstream ss;
    response_obj.stringify(ss);
    sendJSONResponse(response, ss.str());
}

bool FileShareRequestHandler::authenticateRequest(HTTPServerRequest& request, int& userId) {
    std::string authHeader = request.get("Authorization", "");
    if (authHeader.find("Bearer ") == 0) {
        std::string token = authHeader.substr(7);
        return User::validateSession(token, userId);
    }
    return false;
}

bool FileShareRequestHandler::isUsernameExists(const std::string& username) {
    try {
        auto session = Database::getInstance().getSession();
        int count = 0;
        std::string user = username;
        
        Poco::Data::Statement check(session);
        check << "SELECT COUNT(*) FROM users WHERE username = $1",
            Poco::Data::Keywords::use(user), Poco::Data::Keywords::into(count);
        check.execute();
        
        return count > 0;
    } catch (const Poco::Exception& ex) {
        std::cerr << "Username check failed: " << ex.displayText() << std::endl;
        return false; // Assume not exists on error to allow registration attempt
    }
}

void FileShareRequestHandler::sendJSONResponse(HTTPServerResponse& response, const std::string& json, int status) {
    response.setStatus(static_cast<HTTPResponse::HTTPStatus>(status));
    response.setContentType("application/json");
    response.setContentLength(json.length());
    
    std::ostream& out = response.send();
    out << json;
}

void FileShareRequestHandler::sendErrorResponse(HTTPServerResponse& response, const std::string& error, int status) {
    Object errorObj;
    errorObj.set("success", false);
    errorObj.set("error", error);
    
    std::stringstream ss;
    errorObj.stringify(ss);
    sendJSONResponse(response, ss.str(), status);
}

void FileShareRequestHandler::setCORSHeaders(HTTPServerResponse& response) {
    response.set("Access-Control-Allow-Origin", "http://localhost:3000");
    response.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    response.set("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Filename");
    response.set("Access-Control-Allow-Credentials", "true");
    response.set("Access-Control-Max-Age", "86400");
}

void FileShareRequestHandler::handleSharedWithMe(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) {
    int userId;
    if (!authenticateRequest(request, userId)) {
        sendErrorResponse(response, "Unauthorized", 401);
        return;
    }
    
    try {
        auto session = Database::getInstance().getSession();
        
        // Get files shared specifically with this user
        std::vector<int> fileIds;
        std::vector<std::string> filenames;
        std::vector<std::string> originalFilenames;
        std::vector<long> fileSizes;
        std::vector<std::string> contentTypes;
        std::vector<int> ownerIds;
        std::vector<std::string> uploadDates;
        std::vector<std::string> shareTokens;
        std::vector<std::string> sharedByUsers;
        std::vector<std::string> expiryDates;
        
        Poco::Data::Statement select(session);
        select << "SELECT f.file_id, f.filename, f.original_filename, f.file_size, f.content_type, "
                  "f.owner_id, f.upload_date, fs.share_token, u.username, fs.expires_at "
                  "FROM files f "
                  "JOIN file_shares fs ON f.file_id = fs.file_id "
                  "JOIN users u ON fs.shared_by = u.user_id "
                  "WHERE fs.shared_with = $1 AND (fs.expires_at IS NULL OR fs.expires_at > CURRENT_TIMESTAMP) "
                  "ORDER BY fs.created_at DESC",
            Poco::Data::Keywords::use(userId),  // ← Fixed: Fully qualified
            Poco::Data::Keywords::into(fileIds), Poco::Data::Keywords::into(filenames), 
            Poco::Data::Keywords::into(originalFilenames), Poco::Data::Keywords::into(fileSizes),
            Poco::Data::Keywords::into(contentTypes), Poco::Data::Keywords::into(ownerIds), 
            Poco::Data::Keywords::into(uploadDates), Poco::Data::Keywords::into(shareTokens),
            Poco::Data::Keywords::into(sharedByUsers), Poco::Data::Keywords::into(expiryDates);
        select.execute();
        
        Array filesArray;
        for (size_t i = 0; i < fileIds.size(); ++i) {
            Object fileObj;
            fileObj.set("file_id", fileIds[i]);
            fileObj.set("filename", originalFilenames[i]);
            fileObj.set("size", fileSizes[i]);
            fileObj.set("content_type", contentTypes[i]);
            fileObj.set("upload_date", uploadDates[i]);
            fileObj.set("share_token", shareTokens[i]);
            fileObj.set("shared_by", sharedByUsers[i]);
            fileObj.set("expires_at", expiryDates[i]);
            fileObj.set("owner_id", ownerIds[i]);
            filesArray.add(fileObj);
        }
        
        Object response_obj;
        response_obj.set("success", true);
        response_obj.set("shared_files", filesArray);
        
        std::stringstream ss;
        response_obj.stringify(ss);
        sendJSONResponse(response, ss.str());
        
    } catch (const Poco::Exception& ex) {
        sendErrorResponse(response, "Failed to load shared files: " + ex.displayText(), 500);
    }
}

HTTPRequestHandler* FileShareRequestHandlerFactory::createRequestHandler(const HTTPServerRequest& request) {
    return new FileShareRequestHandler();
}

WebServer::WebServer(int port) : serverPort(port), httpServer(nullptr) {}

WebServer::~WebServer() {
    stop();
}

void WebServer::start() {
    ServerSocket serverSocket(serverPort);
    HTTPServerParams* params = new HTTPServerParams();
    params->setMaxThreads(16);
    
    httpServer = new HTTPServer(new FileShareRequestHandlerFactory(), serverSocket, params);
    httpServer->start();
    
    std::cout << "File sharing server started on port " << serverPort << std::endl;
}

void WebServer::stop() {
    if (httpServer) {
        httpServer->stop();
        delete httpServer;
        httpServer = nullptr;
    }
}
