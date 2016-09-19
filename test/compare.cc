#include <gtest/gtest.h>

#include "scientist.hh"


TEST(Compare, CustomComparison)
{
    bool published = false;
    Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { return 1;});
        e.Publish([&](const Observation<int>& o)
        {
            published = true;
            ASSERT_TRUE(o.Success());
        });
        e.Compare([](const int&, const int&) { return true; });
    });

    ASSERT_TRUE(published);
}

struct Data
{
    int Field;
};

TEST(Compare, CustomComparisonForClass)
{
    bool published = false;
    Scientist<Data>::Science("test", [&](ExperimentInterface<Data>& e)
    {
        e.Use([]() { return Data { 1 };});
        e.Try([]() { return Data { 1 };});
        e.Publish([&](const Observation<Data>& o)
                  {
                      published = true;
                      ASSERT_TRUE(o.Success());
                  });
        e.Compare([](const Data& a, const Data& b) { return a.Field == b.Field; });
    });

    ASSERT_TRUE(published);
}

TEST(Compare, CustomComparisonRequiredButMissing)
{
    // Observation.Success must be false if comparison is missing
    bool published = false;
    Scientist<Data>::Science("test", [&](ExperimentInterface<Data>& e)
    {
        e.Use([]() { return Data { 1 };});
        e.Try([]() { return Data { 1 };});
        e.Publish([&](const Observation<Data>& o)
                  {
                      published = true;
                      ASSERT_FALSE(o.Success());
                  });
    });

    ASSERT_TRUE(published);
}
