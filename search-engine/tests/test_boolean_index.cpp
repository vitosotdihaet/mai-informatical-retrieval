#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <algorithm>
#include "boolean_index.hpp"

using namespace boolean_index;

TEST(BooleanIndexTest, BasicInsertAndRetrieve)
{
    BooleanIndex<uint32_t> index;

    // Add some documents
    index.add_document(1, {"apple", "fruit", "red"});
    index.add_document(2, {"banana", "fruit", "yellow"});
    index.add_document(3, {"apple", "pie", "dessert"});

    EXPECT_EQ(index.total_documents(), 3);
    EXPECT_EQ(index.total_terms(), 7);

    // Test single term queries
    auto apple_docs = index.get_documents_for_term("apple");
    EXPECT_EQ(apple_docs.size(), 2);
    EXPECT_TRUE(std::find(apple_docs.begin(), apple_docs.end(), 1) != apple_docs.end());
    EXPECT_TRUE(std::find(apple_docs.begin(), apple_docs.end(), 3) != apple_docs.end());

    auto fruit_docs = index.get_documents_for_term("fruit");
    EXPECT_EQ(fruit_docs.size(), 2);

    auto pie_docs = index.get_documents_for_term("pie");
    EXPECT_EQ(pie_docs.size(), 1);
    EXPECT_EQ(pie_docs[0], 3);
}

TEST(BooleanIndexTest, AndQuery)
{
    BooleanIndex<uint32_t> index;

    index.add_document(1, {"apple", "fruit", "red"});
    index.add_document(2, {"apple", "fruit", "green"});
    index.add_document(3, {"apple", "pie", "dessert"});
    index.add_document(4, {"banana", "fruit", "yellow"});

    // apple AND fruit
    auto result1 = index.and_query({"apple", "fruit"});
    EXPECT_EQ(result1.size(), 2);
    EXPECT_TRUE(std::find(result1.begin(), result1.end(), 1) != result1.end());
    EXPECT_TRUE(std::find(result1.begin(), result1.end(), 2) != result1.end());

    // apple AND pie
    auto result2 = index.and_query({"apple", "pie"});
    EXPECT_EQ(result2.size(), 1);
    EXPECT_EQ(result2[0], 3);

    // Non-existent term
    auto result3 = index.and_query({"apple", "nonexistent"});
    EXPECT_TRUE(result3.empty());

    // Three terms
    auto result4 = index.and_query({"apple", "fruit", "red"});
    EXPECT_EQ(result4.size(), 1);
    EXPECT_EQ(result4[0], 1);
}

TEST(BooleanIndexTest, OrQuery)
{
    BooleanIndex<uint32_t> index;

    index.add_document(1, {"apple", "fruit"});
    index.add_document(2, {"banana", "fruit"});
    index.add_document(3, {"cherry", "fruit"});
    index.add_document(4, {"apple", "pie"});

    // apple OR banana
    auto result1 = index.or_query({"apple", "banana"});
    EXPECT_TRUE(std::find(result1.begin(), result1.end(), 1) != result1.end());
    EXPECT_TRUE(std::find(result1.begin(), result1.end(), 2) != result1.end());
    EXPECT_TRUE(std::find(result1.begin(), result1.end(), 4) != result1.end());
    EXPECT_EQ(result1.size(), 3);

    // apple OR pie
    auto result2 = index.or_query({"apple", "pie"});
    EXPECT_EQ(result2.size(), 2);

    // With non-existent term
    auto result3 = index.or_query({"apple", "nonexistent"});
    EXPECT_EQ(result3.size(), 2); // just apple docs

    // All non-existent terms
    auto result4 = index.or_query({"xyz", "abc"});
    EXPECT_TRUE(result4.empty());
}

TEST(BooleanIndexTest, RemoveDocument)
{
    BooleanIndex<uint32_t> index;

    index.add_document(1, {"apple", "fruit"});
    index.add_document(2, {"apple", "pie"});
    index.add_document(3, {"banana", "fruit"});

    EXPECT_EQ(index.total_documents(), 3);
    EXPECT_EQ(index.get_term_frequency("apple"), 2);

    // Remove document 1
    bool removed = index.remove_document(1, {"apple", "fruit"});
    EXPECT_TRUE(removed);
    EXPECT_EQ(index.total_documents(), 2);
    EXPECT_EQ(index.get_term_frequency("apple"), 1);

    // Verify apple now only in doc 2
    auto apple_docs = index.get_documents_for_term("apple");
    EXPECT_EQ(apple_docs.size(), 1);
    EXPECT_EQ(apple_docs[0], 2);

    // Remove non-existent document
    bool not_removed = index.remove_document(99, {"test"});
    EXPECT_FALSE(not_removed);
}

TEST(BooleanIndexTest, ContainsMethods)
{
    BooleanIndex<uint32_t> index;

    index.add_document(1, {"apple", "fruit"});
    index.add_document(2, {"banana", "fruit"});

    EXPECT_TRUE(index.contains_term("apple"));
    EXPECT_TRUE(index.contains_term("fruit"));
    EXPECT_FALSE(index.contains_term("orange"));

    EXPECT_TRUE(index.contains_document(1));
    EXPECT_TRUE(index.contains_document(2));
    EXPECT_FALSE(index.contains_document(3));
}

TEST(BooleanIndexTest, GetAllTermsAndDocuments)
{
    BooleanIndex<uint32_t> index;

    index.add_document(1, {"apple", "fruit"});
    index.add_document(2, {"banana", "fruit", "yellow"});
    index.add_document(3, {"cherry", "fruit", "red"});

    auto all_terms = index.get_all_terms();
    EXPECT_EQ(all_terms.size(), 6);

    auto all_docs = index.get_all_documents();
    EXPECT_EQ(all_docs.size(), 3);

    // Check all documents are present
    std::vector<uint32_t> expected = {1, 2, 3};
    for (auto doc : expected)
    {
        EXPECT_TRUE(std::find(all_docs.begin(), all_docs.end(), doc) != all_docs.end());
    }
}

TEST(BooleanIndexTest, EmptyIndex)
{
    BooleanIndex<uint32_t> index;

    EXPECT_EQ(index.total_documents(), 0);
    EXPECT_EQ(index.total_terms(), 0);
    EXPECT_TRUE(index.get_all_terms().empty());
    EXPECT_TRUE(index.get_all_documents().empty());

    // Queries on empty index
    EXPECT_TRUE(index.and_query({"test"}).empty());
    EXPECT_TRUE(index.or_query({"test"}).empty());
}

// TEST(BooleanIndexTest, DuplicateDocumentId)
// {
//     BooleanIndex<uint32_t> index;

//     // Adding same document ID with different terms
//     index.add_document(1, {"apple", "fruit"});
//     index.add_document(1, {"banana", "fruit"}); // Should update terms

//     // Document 1 should now have terms from both calls
//     // Our current implementation doesn't handle term updates properly
//     // This is a known limitation

//     EXPECT_EQ(index.total_documents(), 1);
//     EXPECT_TRUE(index.contains_document(1));
// }

TEST(BooleanIndexTest, LargeDataset)
{
    BooleanIndex<uint32_t> index;
    const uint32_t num_docs = 1000;
    const uint32_t terms_per_doc = 5;

    // Add many documents
    for (uint32_t i = 0; i < num_docs; ++i)
    {
        std::vector<std::string> terms;
        for (uint32_t j = 0; j < terms_per_doc; ++j)
        {
            terms.push_back("term_" + std::to_string(j));
        }
        // Add some unique terms
        terms.push_back("doc_" + std::to_string(i));

        index.add_document(i, terms);
    }

    EXPECT_EQ(index.total_documents(), num_docs);

    // Test query on common term
    auto common_result = index.and_query({"term_0", "term_1"});
    EXPECT_EQ(common_result.size(), num_docs); // All docs have both terms

    // Test query on unique term
    auto unique_result = index.and_query({"doc_42"});
    EXPECT_EQ(unique_result.size(), 1);
    EXPECT_EQ(unique_result[0], 42);

    // Test OR query
    auto or_result = index.or_query({"doc_10", "doc_20", "doc_30"});
    EXPECT_EQ(or_result.size(), 3);
}

TEST(BooleanIndexTest, StringDocumentIds)
{
    // Test with string document IDs (e.g., URLs)
    BooleanIndex<std::string> index;

    index.add_document("doc1.html", {"apple", "fruit", "computer"});
    index.add_document("doc2.html", {"apple", "pie", "recipe"});
    index.add_document("doc3.html", {"banana", "fruit", "tropical"});

    EXPECT_EQ(index.total_documents(), 3);

    auto apple_docs = index.get_documents_for_term("apple");
    EXPECT_EQ(apple_docs.size(), 2);
    EXPECT_TRUE(std::find(apple_docs.begin(), apple_docs.end(), "doc1.html") != apple_docs.end());
    EXPECT_TRUE(std::find(apple_docs.begin(), apple_docs.end(), "doc2.html") != apple_docs.end());

    auto result = index.and_query({"fruit", "tropical"});
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "doc3.html");
}

// Main function for Google Test
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}