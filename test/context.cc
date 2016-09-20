#include <gtest/gtest.h>

#include "scientist.hh"


TEST(Context, FindsValueForKey)
{
    bool published = false;
    Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { return 0;});
        e.Context("key", "value");
        e.Publish([&](const Observation<int>& o)
        {
            published = true;
            std::pair<bool, std::string> c = o.Context("key");

            ASSERT_TRUE(c.first);
            ASSERT_EQ("value", c.second);
        });
    });

    ASSERT_TRUE(published);
}

TEST(Context, NonExistingKey)
{
    bool published = false;
    Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { return 0;});
        e.Publish([&](const Observation<int>& o)
        {
              published = true;
              std::pair<bool, const std::string&> c = o.Context("nonexisting");

              ASSERT_FALSE(c.first);
        });
    });

    ASSERT_TRUE(published);
}

TEST(Context, ReturnsKeys)
{
    bool published = false;
    Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { return 0;});
        e.Context("key1", "value");
        e.Context("key2", "value");
        e.Publish([&](const Observation<int>& o)
        {
            published = true;

            std::vector<std::string> keys = o.ContextKeys();
            std::sort(keys.begin(), keys.end());

            ASSERT_EQ(2, keys.size());
            ASSERT_EQ("key1", keys.front());
            ASSERT_EQ("key2", keys.back());
        });
    });

    ASSERT_TRUE(published);
}
