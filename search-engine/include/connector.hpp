#pragma once

#include <cstdint>
#include <iostream>
#include <vector>
#include <sstream>
#include <cctype>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "libstemmer.h"

#include "log.hpp"
#include "boolean_index.hpp"

extern struct sb_stemmer *RU_STEMMER;
extern struct sb_stemmer *EN_STEMMER;

extern mongocxx::instance instance;
extern mongocxx::uri uri;
extern mongocxx::client client;
extern mongocxx::database db;
extern mongocxx::collection collection;

int setup_connector(const char *mongodb_uri, const char *mongodb_db, const char *mongodb_collection);

std::vector<std::string> tokenize_and_stem(const std::string &text);

template <typename DocId = std::string>
bool setup_boolean_index(boolean_index::BooleanIndex<DocId> &index, mongocxx::collection collection)
{
    try
    {
        int count = 0;
        int bad_count = 0;
        auto cursor = collection.find({});

        using clock = std::chrono::high_resolution_clock;

        std::chrono::duration<double> time_to_build_index;

        for (auto &&doc : cursor)
        {
            auto source_elem = doc["source"];
            auto value_elem = doc["value"];

            if (!source_elem || source_elem.type() != bsoncxx::type::k_string)
            {
                bad_count += 1;
                if (bad_count % 1000 == 0)
                {
                    spdlog::warn("{} bad documents", bad_count);
                }
                spdlog::debug("document missing valid 'source' field: {}", bsoncxx::to_json(doc));
                continue;
            }

            if (!value_elem || value_elem.type() != bsoncxx::type::k_string)
            {
                bad_count += 1;
                if (bad_count % 1000 == 0)
                {
                    spdlog::warn("{} bad documents", bad_count);
                }
                spdlog::debug("document missing valid 'value' field: {}", bsoncxx::to_json(doc));
                continue;
            }

            DocId doc_id = std::string{source_elem.get_string().value};
            spdlog::debug("adding doc with doc_id = {} to the boolean index", doc_id);

            std::string value = std::string{value_elem.get_string().value};
            auto start = clock::now();
            std::vector<std::string> terms = tokenize_and_stem(value);
            auto end = clock::now();

            time_to_build_index += end - start;

            index.add_document(doc_id, terms);
            count++;
            if (count % 10000 == 0)
            {
                spdlog::info("added {} documents", count);
                spdlog::info("have been building index for {}s", std::chrono::duration_cast<std::chrono::seconds>(time_to_build_index).count());
            }
        }

        spdlog::info("built index in {}s", std::chrono::duration_cast<std::chrono::seconds>(time_to_build_index).count());
    }
    catch (const std::exception &e)
    {
        spdlog::error("setting up boolean index failed with error {}", e.what());
        return false;
    }

    return true;
}