#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>

class FileShareRequestHandler : public Poco::Net::HTTPRequestHandler {
public:
    void handleRequest(Poco::Net::HTTPServerRequest& request, 
                      Poco::Net::HTTPServerResponse& response) override;

private:
    void setCORSHeaders(Poco::Net::HTTPServerResponse& response); 

    void handleRegister(Poco::Net::HTTPServerRequest& request, 
                       Poco::Net::HTTPServerResponse& response);
    void handleLogin(Poco::Net::HTTPServerRequest& request, 
                    Poco::Net::HTTPServerResponse& response);
    void handleUpload(Poco::Net::HTTPServerRequest& request, 
                     Poco::Net::HTTPServerResponse& response);
    void handleDownload(Poco::Net::HTTPServerRequest& request, 
                       Poco::Net::HTTPServerResponse& response);
    void handleShare(Poco::Net::HTTPServerRequest& request, 
                    Poco::Net::HTTPServerResponse& response);
    void handleList(Poco::Net::HTTPServerRequest& request, 
                   Poco::Net::HTTPServerResponse& response);
    void handleSharedFileAccess(Poco::Net::HTTPServerRequest& request,  
                    Poco::Net::HTTPServerResponse& response); // ← ADD THIS LINE
    void handleSharedWithMe(Poco::Net::HTTPServerRequest& request, 
                           Poco::Net::HTTPServerResponse& response);    // ← ADD THIS LINE
    
    bool isUsernameExists(const std::string& username);
    bool authenticateRequest(Poco::Net::HTTPServerRequest& request, int& userId);
    void sendJSONResponse(Poco::Net::HTTPServerResponse& response, 
                         const std::string& json, int status = 200);
    void sendErrorResponse(Poco::Net::HTTPServerResponse& response, 
                          const std::string& error, int status = 400);
};

class FileShareRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
public:
    Poco::Net::HTTPRequestHandler* createRequestHandler(
        const Poco::Net::HTTPServerRequest& request) override;
};

class WebServer {
public:
    WebServer(int port = 8080);
    ~WebServer();
    void start();
    void stop();

private:
    int serverPort;
    Poco::Net::HTTPServer* httpServer;
};

#endif
