#include "spdlog/spdlog.h"
#include "log.hpp"

void setup_logger()
{
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [tx %t] %v");
}