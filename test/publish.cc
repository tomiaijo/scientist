#include <gtest/gtest.h>

#include "scientist.hh"

TEST(Publish, PublishesObservationFields)
{
    bool published = false;
    int res = Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { return 42;});
        e.Publish([&](const Observation<int>& o)
        {
            published = true;
            ASSERT_EQ("test", o.Name());
            ASSERT_TRUE(o.Success());
            ASSERT_FALSE(o.CandidateException());
            ASSERT_FALSE(o.ControlException());
            ASSERT_GT(o.CandidateDuration().count(), 0);
            ASSERT_GT(o.ControlDuration().count(), 0);
            ASSERT_EQ(o.CandidateResult(), 42);
            ASSERT_EQ(o.ControlResult(), 42);
        });
    });

    ASSERT_TRUE(published);
    ASSERT_EQ(42, res);
}

TEST(Publish, PublishesResultIfExperimentUnsuccessful)
{
    bool published = false;
    int res = Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { return 0;});
        e.Publish([&](const Observation<int>& o)
        {
            published = true;
            ASSERT_FALSE(o.Success());
            ASSERT_EQ(o.CandidateResult(), 0);
            ASSERT_EQ(o.ControlResult(), 42);
        });
    });

    ASSERT_TRUE(published);
    ASSERT_EQ(42, res);
}

TEST(Publish, PublishesObservationIfCandidateThrows)
{
    bool published = false;

    int res = Scientist<int>::Science("", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { throw ""; return 0; });
        e.Publish([&](const Observation<int>& o)
        {
            published = true;
            ASSERT_FALSE(o.Success());
            ASSERT_TRUE(o.CandidateException());
            ASSERT_FALSE(o.ControlException());
            ASSERT_GT(o.CandidateDuration().count(), 0);
            ASSERT_GT(o.ControlDuration().count(), 0);
        });
    });

    ASSERT_TRUE(published);
    ASSERT_EQ(42, res);
}

TEST(Publish, RethrowsControlException)
{
    ASSERT_THROW(Scientist<int>::Science("", [](ExperimentInterface<int>& e)
    {
        e.Use([]() { throw std::string(); return 42;});
        e.Try([]() { return 0; });
    }), std::string);
}

TEST(Publish, SuccessIfBothThrow)
{
    bool published = false;
    ASSERT_ANY_THROW(Scientist<int>::Science("", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { throw ""; return 0; });
        e.Try([]() { throw ""; return 42; });
        e.Publish([&](const Observation<int>& o)
        {
            published = true;
            ASSERT_TRUE(o.Success());
        });
    }));

    ASSERT_TRUE(published);
}

TEST(Publish, MultiplePublishers)
{
    bool aPublished = false;
    bool bPublished = false;
    Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { return 42;});
        e.Publish([&](const Observation<int>& o)
        {
            aPublished = true;
        });
        e.Publish([&](const Observation<int>& o)
        {
            bPublished = true;
        });
    });

    ASSERT_TRUE(aPublished);
    ASSERT_TRUE(bPublished);
}

TEST(Publish, AsyncPublishes)
{
    bool published = false;
    Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { return 42;});
        e.PublishAsync([&](const Observation<int>& o)
        {
            published = true;
        });
    });

    while (!published);
    ASSERT_TRUE(published);
}

TEST(Publish, AsyncPublishDoesNotBlock)
{
    bool block = true;
    int value = Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { return 42;});
        e.PublishAsync([&](const Observation<int>& o)
        {
            while(block);
        });
    });

    ASSERT_EQ(42, value);
    block = false;
}
