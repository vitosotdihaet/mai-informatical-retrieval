#include <cstdint>
#include <iostream>
#include <vector>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "log.hpp"
#include "connector.hpp"

mongocxx::instance instance;
mongocxx::uri uri;
mongocxx::client client;
mongocxx::database db;
mongocxx::collection collection;

int setup_connector(const char *mongodb_uri, const char *mongodb_db, const char *mongodb_collection)
{
    uri = mongocxx::uri(mongodb_uri);
    client = mongocxx::client(uri);
    db = client[mongodb_db];
    collection = db[mongodb_collection];

    int64_t result = collection.count_documents({});
    return 0;
}