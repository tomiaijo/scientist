#include <gtest/gtest.h>

#include "scientist.hh"

TEST(MultipleCandidates, RunsAllCandidates)
{
    std::size_t ran = 0x0;
    int res = Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42; });
        e.Try([&]() { ran |= 0x1; return 42; });
        e.Try([&]() { ran |= 0x2; return 42; });
        e.Try([&]() { ran |= 0x4; return 42; });
        e.Publish([&](const Observation<int>& o)
        {
            ASSERT_TRUE(o.Success());
        });
    });

    ASSERT_EQ(0x7, ran);
    ASSERT_EQ(42, res);
}

TEST(MultipleCandidates, ReportsSuccessWhenAllCandidatesReturnCorrectAnswer)
{
    std::size_t number = 0;
    int res = Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42; });
        e.Try([]() { return 42; });
        e.Try([]() { return 42; });
        e.Try([]() { return 42; });
        e.Publish([&](const Observation<int>& o)
        {
            number = o.NumberOfCandidates();
            ASSERT_TRUE(o.Success());
        });
    });

    ASSERT_EQ(3, number);
    ASSERT_EQ(42, res);
}

TEST(MultipleCandidates, ReportsFailureIfOneCandidateReturnsWrongAnswer)
{
    std::size_t number = 0;
    int res = Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42; });
        e.Try([]() { return 42; });
        e.Try([]() { return 42; });
        e.Try([]() { return 41; });
        e.Publish([&](const Observation<int>& o)
        {
            number = o.NumberOfCandidates();

            ASSERT_FALSE(o.Success());
        });
    });

    ASSERT_EQ(3, number);
    ASSERT_EQ(42, res);
}

TEST(MultipleCandidates, CandidateAnswersInOrderOfCandidateAddition)
{
    std::size_t number = 0;
    int res = Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42; });
        e.Try([]() { return 1; });
        e.Try([]() { return 2; });
        e.Try([]() { return 3; });
        e.Publish([&](const Observation<int>& o)
        {
            number = o.NumberOfCandidates();

            ASSERT_EQ(1, o.CandidateResult(0));
            ASSERT_EQ(2, o.CandidateResult(1));
            ASSERT_EQ(3, o.CandidateResult(2));

            ASSERT_FALSE(o.Success());
        });
    });

    ASSERT_EQ(3, number);
    ASSERT_EQ(42, res);
}

TEST(MultipleCandidates, ReportsFailureIfOneCandidateThrows)
{
    std::size_t number = 0;
    int res = Scientist<int>::Science("test", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42; });
        e.Try([]() { return 42; });
        e.Try([]() { throw std::exception(); return 42; });
        e.Try([]() { return 42; });
        e.Publish([&](const Observation<int>& o)
        {
            number = o.NumberOfCandidates();
            ASSERT_FALSE(o.Success());
        });
    });

    ASSERT_EQ(3, number);
    ASSERT_EQ(42, res);
}
