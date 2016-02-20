#include <gtest/gtest.h>

#include "scientist.hh"


TEST(BeforeRun, RunsSetup)
{
    bool setupCalled = false;
    int res = Scientist<int>::Science("", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { return 0;});
        e.BeforeRun([&]() { setupCalled = true; });
    });

    ASSERT_TRUE(setupCalled);
}

TEST(BeforeRun, RunSetupsInOrder)
{
    bool firstSetupCalled = false;
    bool secondSetupCalled = false;

    int res = Scientist<int>::Science("", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { return 0;});
        e.BeforeRun([&]() { ASSERT_FALSE(secondSetupCalled); firstSetupCalled = true; });
        e.BeforeRun([&]() { ASSERT_TRUE(firstSetupCalled); secondSetupCalled = true; });

    });

    ASSERT_TRUE(firstSetupCalled);
    ASSERT_TRUE(secondSetupCalled);
}

TEST(BeforeRun, DoesNotRunSetupIfExperimentIsDisabled)
{
    bool setupCalled = false;
    int res = Scientist<int>::Science("", [&](ExperimentInterface<int>& e)
    {
        e.Use([]() { return 42;});
        e.Try([]() { return 0;});
        e.RunIf([]() { return false; });
        e.BeforeRun([&]() { setupCalled = true; });
    });

    ASSERT_FALSE(setupCalled);
}
