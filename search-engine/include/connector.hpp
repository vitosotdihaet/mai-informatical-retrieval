#pragma once
#include <cstdint>
#include <iostream>
#include <vector>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

extern mongocxx::instance instance;
extern mongocxx::uri uri;
extern mongocxx::client client;
extern mongocxx::database db;
extern mongocxx::collection collection;

int setup_connector(const char *mongodb_uri, const char *mongodb_db, const char *mongodb_collection);