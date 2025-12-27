#include "libstemmer.h"

#include "server.hpp"
#include "log.hpp"
#include "connector.hpp"
#include "boolean_index.hpp"

const char *MONGODB_URI = "mongodb://root:example@localhost:27017";
const char *MONGODB_DB = "scraper";
const char *MONGODB_COLLECTION = "scraps";

const uint64_t MAX_RESPONSE_COUNT = 10;

const int SERVER_PORT = 9999;

boolean_index::BooleanIndex<std::string> INDEX = boolean_index::BooleanIndex<std::string>(10);

int main()
{
    setup_logger();
    setup_connector(MONGODB_URI, MONGODB_DB, MONGODB_COLLECTION);
    setup_boolean_index(INDEX, collection);

    INDEX.print_statistics();
    INDEX.print_index();

    MinimalAsyncServer server(SERVER_PORT, MAX_RESPONSE_COUNT, INDEX);
    server.run();

    return 0;
}