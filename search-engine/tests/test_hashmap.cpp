#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include "hashmap.hpp"

using namespace hashmap;

TEST(HashMapTest, BasicInsertAndFind)
{
    HashMap<int, std::string> map;

    EXPECT_TRUE(map.insert(1, "one"));
    EXPECT_TRUE(map.insert(2, "two"));
    EXPECT_TRUE(map.insert(3, "three"));

    EXPECT_EQ(map.size(), 3);

    auto *value1 = map.find(1);
    ASSERT_NE(value1, nullptr);
    EXPECT_EQ(*value1, "one");

    auto *value2 = map.find(2);
    ASSERT_NE(value2, nullptr);
    EXPECT_EQ(*value2, "two");

    EXPECT_EQ(map.find(4), nullptr);
}

TEST(HashMapTest, DuplicateInsert)
{
    HashMap<int, std::string> map;

    EXPECT_TRUE(map.insert(1, "one"));
    EXPECT_FALSE(map.insert(1, "uno")); // Should return false for duplicate

    auto *value = map.find(1);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, "uno"); // Value should be updated
    EXPECT_EQ(map.size(), 1);
}

TEST(HashMapTest, EraseOperations)
{
    HashMap<int, std::string> map;

    map.insert(1, "one");
    map.insert(2, "two");
    map.insert(3, "three");

    EXPECT_TRUE(map.erase(2));
    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map.find(2), nullptr);

    EXPECT_FALSE(map.erase(4)); // Non-existent key
    EXPECT_EQ(map.size(), 2);
}

TEST(HashMapTest, OperatorBracket)
{
    HashMap<int, int> map;

    map[1] = 100;
    map[2] = 200;
    map[1] = 300; // Update existing

    EXPECT_EQ(map[1], 300);
    EXPECT_EQ(map[2], 200);
    EXPECT_EQ(map[3], 0); // Creates default value

    EXPECT_EQ(map.size(), 3);
}

TEST(HashMapTest, AtMethod)
{
    HashMap<int, std::string> map;

    map.insert(1, "one");

    EXPECT_EQ(map.at(1), "one");
    EXPECT_THROW(map.at(2), std::out_of_range);
}

TEST(HashMapTest, ContainsMethod)
{
    HashMap<int, std::string> map;

    map.insert(1, "one");
    map.insert(2, "two");

    EXPECT_TRUE(map.contains(1));
    EXPECT_TRUE(map.contains(2));
    EXPECT_FALSE(map.contains(3));
}

TEST(HashMapTest, ClearAndEmpty)
{
    HashMap<int, std::string> map;

    EXPECT_TRUE(map.empty());

    map.insert(1, "one");
    map.insert(2, "two");

    EXPECT_FALSE(map.empty());
    EXPECT_EQ(map.size(), 2);

    map.clear();

    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);
    EXPECT_EQ(map.find(1), nullptr);
}

TEST(HashMapTest, InitializerList)
{
    HashMap<int, std::string> map = {
        {1, "one"},
        {2, "two"},
        {3, "three"}};

    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map.at(1), "one");
    EXPECT_EQ(map.at(2), "two");
    EXPECT_EQ(map.at(3), "three");
}

TEST(HashMapTest, Iterator)
{
    HashMap<int, std::string> map;

    map.insert(1, "one");
    map.insert(2, "two");
    map.insert(3, "three");

    std::unordered_map<int, std::string> expected = {
        {1, "one"},
        {2, "two"},
        {3, "three"}};

    std::unordered_map<int, std::string> actual;
    for (const auto &kv : map)
    {
        actual[kv.first] = kv.second;
    }

    EXPECT_EQ(actual, expected);
}

TEST(HashMapTest, ConstIterator)
{
    HashMap<int, std::string> map;
    map.insert(1, "one");
    map.insert(2, "two");

    const auto &const_map = map;

    size_t count = 0;
    for (auto it = const_map.cbegin(); it != const_map.cend(); ++it)
    {
        ++count;
    }

    EXPECT_EQ(count, 2);
}

TEST(HashMapTest, Rehashing)
{
    HashMap<int, int> map(4); // Start with small capacity

    // Insert enough elements to trigger rehash
    for (int i = 0; i < 10; ++i)
    {
        map.insert(i, i * 10);
    }

    EXPECT_EQ(map.size(), 10);
    EXPECT_GT(map.bucket_count(), 4); // Should have rehashed

    for (int i = 0; i < 10; ++i)
    {
        EXPECT_TRUE(map.contains(i));
        EXPECT_EQ(*map.find(i), i * 10);
    }

    EXPECT_LT(map.load_factor(), 0.75f); // Should be below max load factor
}

TEST(HashMapTest, StringKeys)
{
    HashMap<std::string, int> map;

    map.insert("apple", 1);
    map.insert("banana", 2);
    map.insert("cherry", 3);

    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(*map.find("apple"), 1);
    EXPECT_EQ(*map.find("banana"), 2);
    EXPECT_EQ(*map.find("cherry"), 3);

    EXPECT_TRUE(map.erase("banana"));
    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map.find("banana"), nullptr);
}

TEST(HashMapTest, CustomHashFunction)
{
    struct Point
    {
        int x, y;

        bool operator==(const Point &other) const
        {
            return x == other.x && y == other.y;
        }
    };

    struct PointHash
    {
        size_t operator()(const Point &p) const
        {
            return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
        }
    };

    HashMap<Point, std::string, PointHash> map;

    Point p1{1, 2};
    Point p2{3, 4};

    map.insert(p1, "point one");
    map.insert(p2, "point two");

    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(*map.find(p1), "point one");
    EXPECT_EQ(*map.find(p2), "point two");
}

TEST(HashMapTest, LoadFactorAndBucketStats)
{
    HashMap<int, int> map(8);

    for (int i = 0; i < 6; ++i)
    {
        map.insert(i, i);
    }

    float lf = map.load_factor();
    EXPECT_GT(lf, 0.0f);
    EXPECT_LT(lf, 1.0f);

    EXPECT_GT(map.bucket_count(), 0);

    // Can't predict exact bucket sizes, but should be non-negative
    for (size_t i = 0; i < map.bucket_count(); ++i)
    {
        EXPECT_GE(map.bucket_size(i), 0);
    }
}

TEST(HashMapTest, StressTest)
{
    HashMap<int, int> map;
    const int count = 10000;

    for (int i = 0; i < count; ++i)
    {
        map.insert(i, i * 2);
    }

    EXPECT_EQ(map.size(), count);

    for (int i = 0; i < count; ++i)
    {
        EXPECT_TRUE(map.contains(i));
        EXPECT_EQ(*map.find(i), i * 2);
    }

    for (int i = 0; i < count; i += 2)
    {
        map.erase(i);
    }

    EXPECT_EQ(map.size(), count / 2);

    for (int i = 0; i < count; ++i)
    {
        if (i % 2 == 0)
        {
            EXPECT_FALSE(map.contains(i));
        }
        else
        {
            EXPECT_TRUE(map.contains(i));
        }
    }
}

TEST(HashMapTest, MoveOperations)
{
    HashMap<int, std::string> map1;
    map1.insert(1, "one");
    map1.insert(2, "two");

    // Test move constructor
    HashMap<int, std::string> map2(std::move(map1));
    EXPECT_EQ(map2.size(), 2);
    EXPECT_TRUE(map1.empty());

    // Test move assignment
    HashMap<int, std::string> map3;
    map3 = std::move(map2);
    EXPECT_EQ(map3.size(), 2);
    EXPECT_TRUE(map2.empty());

    EXPECT_EQ(*map3.find(1), "one");
    EXPECT_EQ(*map3.find(2), "two");
}

// Main function for Google Test
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}