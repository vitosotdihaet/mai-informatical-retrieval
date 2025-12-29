#include "connector.hpp"

mongocxx::instance instance;
mongocxx::uri uri;
mongocxx::client client;
mongocxx::database db;
mongocxx::collection collection;

struct sb_stemmer *RU_STEMMER = sb_stemmer_new("ru", NULL);
struct sb_stemmer *EN_STEMMER = sb_stemmer_new("en", NULL);

int setup_connector(const char *mongodb_uri, const char *mongodb_db, const char *mongodb_collection)
{
    uri = mongocxx::uri(mongodb_uri);
    client = mongocxx::client(uri);
    db = client[mongodb_db];
    collection = db[mongodb_collection];

    int64_t result = collection.count_documents({});
    return 0;
}

inline bool is_cyrillic(unsigned char c)
{
    // UTF-8 Cyrillic: 0xD0 0xD1 prefixes
    return c == 0xD0 || c == 0xD1;
}

inline size_t utf8_char_len(unsigned char c)
{
    if ((c & 0x80) == 0x00)
        return 1; // 0xxxxxxx
    if ((c & 0xE0) == 0xC0)
        return 2; // 110xxxxx
    if ((c & 0xF0) == 0xE0)
        return 3; // 1110xxxx
    if ((c & 0xF8) == 0xF0)
        return 4; // 11110xxx
    return 1;     // invalid
}

inline bool is_russian_token(const std::string &token)
{
    for (unsigned char c : token)
    {
        if (c >= 0x80)
            return true;
    }
    return false;
}

inline std::string normalize_text_utf8(const std::string &input)
{
    std::string out;
    out.reserve(input.size());

    for (size_t i = 0; i < input.size();)
    {
        unsigned char c = input[i];

        // ASCII
        if (c < 0x80)
        {
            if (std::isalnum(c))
            {
                out.push_back(std::tolower(c));
            }
            else
            {
                out.push_back(' ');
            }
            ++i;
        }
        // Cyrillic letter (2-byte UTF-8)
        else if (is_cyrillic(c) && i + 1 < input.size())
        {
            unsigned char b1 = static_cast<unsigned char>(input[i]);
            unsigned char b2 = static_cast<unsigned char>(input[i + 1]);

            // А–Я
            if (b1 == 0xD0 && b2 >= 0x90 && b2 <= 0xAF)
            {
                b2 += 0x20; // lowercase
            }

            out.push_back(static_cast<char>(b1));
            out.push_back(static_cast<char>(b2));
            i += 2;
        }
        // Any other UTF-8 symbol (punctuation, quotes, etc.)
        else
        {
            size_t len = utf8_char_len(c);
            out.push_back(' ');
            i += len;
        }
    }

    return out;
}

std::vector<std::string> tokenize_and_stem(const std::string &text)
{
    std::vector<std::string> terms;
    std::string token;

    auto flush = [&](void)
    {
        if (token.empty())
            return;

        sb_stemmer *stemmer = is_russian_token(token) ? RU_STEMMER : EN_STEMMER;

        const sb_symbol *stemmed = sb_stemmer_stem(stemmer, reinterpret_cast<const sb_symbol *>(token.data()), static_cast<int>(token.size()));

        if (stemmed)
        {
            int len = sb_stemmer_length(stemmer);
            if (len > 2)
            {
                terms.emplace_back(reinterpret_cast<const char *>(stemmed), len);
            }
        }

        token.clear();
    };

    for (char c : normalize_text_utf8(text))
    {
        if (c != ' ')
        {
            token.push_back(c);
        }
        else
        {
            flush();
        }
    }
    flush();

    return terms;
}
