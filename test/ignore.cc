#include <gtest/gtest.h>

#include "scientist.hh"

TEST(Ignore, IgnoresIfReturnsFalse)
{
    bool published = false;
    int res = Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { return 1;});
        e.Ignore([]() { return true; });
        e.Publish([&](const Observation<int>& o)
        {
            published = true;
            ASSERT_TRUE(o.Success());
        });
    });

    ASSERT_TRUE(published);
    ASSERT_EQ(42, res);
}

TEST(Ignore, DoesNotIgnore)
{
    bool published = false;
    int res = Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { return 1;});
        e.Ignore([]() { return false; });
        e.Publish([&](const Observation<int>& o)
        {
            published = true;
            ASSERT_FALSE(o.Success());
        });
    });

    ASSERT_TRUE(published);
    ASSERT_EQ(42, res);
}

TEST(Ignore, IgnoresIfPredicateThrows)
{
    bool published = false;
    int res = Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { return 1;});
        e.Ignore([]() { throw ""; return false; });
        e.Publish([&](const Observation<int>& o)
        {
            published = true;
            ASSERT_TRUE(o.Success());
        });
    });

    ASSERT_TRUE(published);
    ASSERT_EQ(42, res);
}
