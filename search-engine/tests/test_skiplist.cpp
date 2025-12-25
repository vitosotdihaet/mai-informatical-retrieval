#include <gtest/gtest.h>
#include <string>
#include "skiplist.hpp"

using namespace skiplist;

TEST(SkipListTest, IntegerInsertAndSearch)
{
    SkipList<int> skiplist;

    EXPECT_TRUE(skiplist.insert(10));
    EXPECT_TRUE(skiplist.insert(5));
    EXPECT_TRUE(skiplist.insert(15));
    EXPECT_TRUE(skiplist.insert(7));

    EXPECT_TRUE(skiplist.search(10));
    EXPECT_TRUE(skiplist.search(5));
    EXPECT_TRUE(skiplist.search(15));
    EXPECT_TRUE(skiplist.search(7));

    EXPECT_FALSE(skiplist.search(12));
    EXPECT_FALSE(skiplist.search(0));
}

TEST(SkipListTest, IntegerDuplicateInsert)
{
    SkipList<int> skiplist;

    EXPECT_TRUE(skiplist.insert(10));
    EXPECT_FALSE(skiplist.insert(10)); // Duplicate should fail

    EXPECT_EQ(skiplist.size(), 1);
}

TEST(SkipListTest, IntegerRemove)
{
    SkipList<int> skiplist;

    skiplist.insert(10);
    skiplist.insert(5);
    skiplist.insert(15);

    EXPECT_TRUE(skiplist.remove(10));
    EXPECT_FALSE(skiplist.search(10));
    EXPECT_TRUE(skiplist.search(5));
    EXPECT_TRUE(skiplist.search(15));

    EXPECT_EQ(skiplist.size(), 2);
}

TEST(SkipListTest, IntegerMinMax)
{
    SkipList<int> skiplist;

    skiplist.insert(10);
    skiplist.insert(5);
    skiplist.insert(15);
    skiplist.insert(7);
    skiplist.insert(20);

    EXPECT_EQ(skiplist.get_min(), 5);
    EXPECT_EQ(skiplist.get_max(), 20);
}

TEST(SkipListTest, StringOperations)
{
    SkipList<std::string> skiplist;

    EXPECT_TRUE(skiplist.insert("apple"));
    EXPECT_TRUE(skiplist.insert("banana"));
    EXPECT_TRUE(skiplist.insert("cherry"));

    EXPECT_TRUE(skiplist.search("banana"));
    EXPECT_FALSE(skiplist.search("grape"));

    EXPECT_TRUE(skiplist.remove("banana"));
    EXPECT_FALSE(skiplist.search("banana"));
}

TEST(SkipListTest, Iterator)
{
    SkipList<int> skiplist;
    std::vector<int> values = {3, 6, 7, 9, 12, 19, 17, 26, 21, 25};

    for (int val : values)
    {
        skiplist.insert(val);
    }

    std::sort(values.begin(), values.end());
    auto it = values.begin();

    for (auto val : skiplist)
    {
        EXPECT_EQ(val, *it);
        ++it;
    }

    EXPECT_EQ(skiplist.size(), values.size());
}

TEST(SkipListTest, EmptyList)
{
    SkipList<int> skiplist;

    EXPECT_TRUE(skiplist.empty());
    EXPECT_EQ(skiplist.size(), 0);

    EXPECT_THROW(skiplist.get_min(), std::runtime_error);
    EXPECT_THROW(skiplist.get_max(), std::runtime_error);
}

TEST(SkipListTest, LargeOperations)
{
    SkipList<int> skiplist;
    const int count = 1000;

    for (int i = 0; i < count; i++)
    {
        skiplist.insert(i);
    }

    EXPECT_EQ(skiplist.size(), count);

    for (int i = 0; i < count; i++)
    {
        EXPECT_TRUE(skiplist.search(i));
    }

    for (int i = 0; i < count; i += 2)
    {
        skiplist.remove(i);
    }

    EXPECT_EQ(skiplist.size(), count / 2);

    for (int i = 0; i < count; i++)
    {
        if (i % 2 == 0)
        {
            EXPECT_FALSE(skiplist.search(i));
        }
        else
        {
            EXPECT_TRUE(skiplist.search(i));
        }
    }
}

// Main function for Google Test
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}