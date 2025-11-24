#include "server.hpp"
#include "log.hpp"
#include "connector.hpp"

const char *MONGODB_URI = "mongodb://root:example@localhost:27017";
const char *MONGODB_DB = "scraper";
const char *MONGODB_COLLECTION = "scraps";

const uint64_t MAX_RESPONSE_COUNT = 10;

const int SERVER_PORT = 9999;

int main()
{
    setup_logger();
    setup_connector(MONGODB_URI, MONGODB_DB, MONGODB_COLLECTION);

    MinimalAsyncServer server(SERVER_PORT, MAX_RESPONSE_COUNT);
    server.run();

    return 0;
}