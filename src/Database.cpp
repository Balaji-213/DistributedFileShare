#include "Database.h"
#include <Poco/Data/SessionFactory.h>
#include <iostream>

std::unique_ptr<Database> Database::instance = nullptr;

Database& Database::getInstance() {
    if (!instance) {
        instance = std::unique_ptr<Database>(new Database());
    }
    return *instance;
}

bool Database::initialize() {
    try {
        // Register PostgreSQL connector
        Poco::Data::PostgreSQL::Connector::registerConnector();
        
        // Connection string for YugabyteDB
        connectionString = "host=127.0.1.1 port=5433 dbname=fileshare user=yugabyte";
        
        // Test connection
        auto session = getSession();
        return true;
    }
    catch (const Poco::Exception& ex) {
        std::cerr << "Database initialization failed: " << ex.displayText() << std::endl;
        return false;
    }
}

Poco::Data::Session Database::getSession() {
    return Poco::Data::Session("PostgreSQL", connectionString);
}
