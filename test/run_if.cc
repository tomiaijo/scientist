#include <gtest/gtest.h>

#include "scientist.hh"

TEST(RunIf, DoesNotRunCandidate)
{
    bool candidateRun = false;
    bool published = false;
    int res = Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([&]() { candidateRun = true; return 1;});
        e.RunIf([]() { return false; });
        e.Publish([&](const Observation<int>& o)
        {
            published = true;
        });
    });

    ASSERT_FALSE(published);
    ASSERT_FALSE(candidateRun);
    ASSERT_EQ(42, res);
}

TEST(RunIf, DoesNotRunCandidateIfAnyFalse)
{
    bool candidateRun = false;
    bool published = false;
    int res = Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([&]() { candidateRun = true; return 1;});
        e.RunIf([]() { return true; });
        e.RunIf([]() { return false; });
        e.Publish([&](const Observation<int>& o)
        {
            published = true;
        });
    });

    ASSERT_FALSE(published);
    ASSERT_FALSE(candidateRun);
    ASSERT_EQ(42, res);
}

TEST(RunIf, DoesNotRunCandidateIfPredicateThrows)
{
    bool candidateRun = false;
    bool published = false;
    int res = Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([&]() { candidateRun = true; return 1;});
        e.RunIf([]() { return true; });
        e.RunIf([]() { throw ""; return true; });
        e.Publish([&](const Observation<int>& o)
        {
            published = true;
        });
    });

    ASSERT_FALSE(published);
    ASSERT_FALSE(candidateRun);
    ASSERT_EQ(42, res);
}
