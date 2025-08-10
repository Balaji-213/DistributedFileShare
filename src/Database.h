#ifndef DATABASE_H
#define DATABASE_H

#include <Poco/Data/Session.h>
#include <Poco/Data/PostgreSQL/Connector.h>
#include <string>
#include <memory>

class Database {
public:
    static Database& getInstance();
    Poco::Data::Session getSession();
    bool initialize();
    
private:
    Database() = default;
    static std::unique_ptr<Database> instance;
    std::string connectionString;
};

#endif
