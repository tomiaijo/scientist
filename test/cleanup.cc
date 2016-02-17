#include <gtest/gtest.h>

#include "scientist.hh"

TEST(Cleanup, Cleanup)
{
    bool published = false;
    Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { return 1;});
        e.Publish([&](const Observation<int>& o)
        {
            published = true;
            ASSERT_FALSE(o.Success());
            ASSERT_EQ(o.ControlResult(), 84);
            ASSERT_EQ(o.CandidateResult(), 2);
        });
        e.Cleanup([](const int& value ) { return 2 * value; });
    });

    ASSERT_TRUE(published);
}

struct Data
{
    int Field;
};

TEST(Cleanup, CleanupClass)
{
    bool published = false;
    Scientist<Data, int>::Science("test", [&](ExperimentInterface<Data, int>& e)
    {
        e.Use([]() { return Data { 42 };});
        e.Try([]() { return Data { 1 };});
        e.Publish([&](const Observation<int>& o)
        {
            published = true;
            ASSERT_FALSE(o.Success());
            ASSERT_EQ(o.ControlResult(), 42);
            ASSERT_EQ(o.CandidateResult(), 1);

        });
        e.Cleanup([](const Data& d ) { return d.Field; });
        e.Compare([]( const Data& a, const Data& b) { return a.Field == b.Field;});
    });

    ASSERT_TRUE(published);
}

TEST(Cleanup, CleanupRequiredButMissingDoesNotCrash)
{
    bool published = false;
    Scientist<Data, int>::Science("test", [&](ExperimentInterface<Data, int>& e)
    {
        e.Use([]() { return Data { 42 };});
        e.Try([]() { return Data { 1 };});
        e.Publish([&](const Observation<int>& o)
        {
            published = true;
            ASSERT_FALSE(o.Success());
            ASSERT_EQ(o.ControlResult(), 0 /* default value for int */);
            ASSERT_EQ(o.CandidateResult(), 0 /* default value for int */);
        });
        e.Compare([]( const Data& a, const Data& b) { return a.Field == b.Field;});
    });

    ASSERT_TRUE(published);
}