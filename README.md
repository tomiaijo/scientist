# Scientist

A C++ library for carefully refactoring critical paths. This is a port of Github's Ruby [Scientist library](http://githubengineering.com/scientist/).

# Usage

Currently it follows closely the original API (and the [.NET version](https://github.com/Haacked/Scientist.net)).

Here is how a simple experiment is defined in C++:

```cpp
#include <scientist.hh>

int res = Scientist<int>::Science("do-stuff", [](ExperimentInterface<int>& e)
{
    e.Use([]() { return do_stuff_legacy();});
    e.Try([]() { return do_stuff();});
});

```

Scientist has most of the features of the Ruby library:

- It decides whether or not to run the `Try` function
- Randomizes the order of `Try`and `Use` functions
- Measures the durations of both functions
- Compares the results
- Swallows exceptions from `Try` function
- Publishes all observations

# Installation

Currently there is only a [single header](include/scientist.hh) that is required. Copy `scientist.hh` somewhere and point the compiler at the location.

# Experiments

Experiments are described using the following interface:

```cpp
template <class T, class U = T>
class ExperimentInterface
{
public:
    virtual void Use(Operation<T> control) = 0;
    virtual void Try(Operation<T> candidate) = 0;
    virtual void Ignore(Predicate ignore) = 0;
    virtual void RunIf(Predicate runIf) = 0;
    virtual void Publish(Publisher<U> publisher) = 0;
    virtual void PublishAsync(Publisher<U> publisher) = 0;
    virtual void Compare(Compare<T> compare) = 0;
    virtual void Cleanup(Transform<T,U> cleanup) = 0;
    virtual void Context(std::string key, std::string value) = 0;
};

using Operation = std::function<T()>;
using Compare = std::function<bool(const T&, const T&)>;
using Predicate = std::function<bool()>;
using Publisher = std::function<void(const Observation<T>&)>;
using Transform = std::function<U(const T&)>;
```

Template parameter `T` denotes the result type of the operations, and `U` the possible cleaned result type (See [Cleanup](#control-the-stored-results))

# Observations

For each experiment run, Scientist returns an observation in the following form:

```cpp
template <class T>
class Observation
{
    const std::string& Name() const;
    bool Success() const;

    std::chrono::nanoseconds ControlDuration() const;
    std::exception_ptr ControlException() const;
    T ControlResult() const;

    std::chrono::nanoseconds CandidateDuration() const;
    std::exception_ptr CandidateException() const;
    T CandidateResult() const;

    std::list<std::string> ContextKeys() const;
    std::pair<bool, const std::string&> Context(std::string key) const;
};

```

An experiment is successful if:

- Control and candidate return identical results (according to the equal operator or given custom comparator)
- They both throw an exception
- `Try` is ignored (See [Ignore](#ignore-known-issues))

To gather observations, register `Publish` function:

```cpp
int res = Scientist<int>::Science("", [](ExperimentInterface<int>& e)
{
    ...
    e.Publish([](const Observation<int>& o)
    {
       ...
    }
});

```

There can be a number of registered publishers. 
All `Publish` functions are executed before the control result is returned. 
There exists a asynchronous version, `PublishAsync`, for long running operations.

See [publish tests](test/publish.cc) for more examples.

# Comparison

You can specify a custom comparison function:

```cpp
int res = Scientist<int>::Science("", [](ExperimentInterface<int>& e)
{
    ...
    e.Compare([](const int&, const int&) { ... });

});

```

Comparison function is required if the result type does not have an equal operator. 
Otherwise, all experiments fail silently.

See [comparison tests](test/compare.cc) for more examples.

# Control the stored results

The `Observation` contains results from both operations, which might not be ideal in all cases. 
You can register a `Cleanup` function, which maps the returned data into desired form.
This cleaned value is available in observations.

```cpp
struct Data
{
    int Field;
};

Scientist<Data, int>::Science("", [](ExperimentInterface<Data, int>& e)
{
    e.Use([]() { return Data { 0 };});
    e.Try([]() { return Data { 1 };});
    e.Publish([&](const Observation<int>& o)
    {
        ...

    });
    e.Cleanup([](const Data& d ) { return d.Field; });
    e.Compare([]( const Data& a, const Data& b) { return a.Field == b.Field;});
});
```
The second template parameter denotes the type of the cleaned value (`int` in this case).

See [cleanup tests](test/cleanup.cc) for more examples.
 
# Ignore known issues

You can ignore some test results with `Ignore` function.
Experiment is ignored if any `Ignore` function returns `true` or throws an exception. 
Exceptions are swallowed.

```cpp
int res = Scientist<int>::Science("", [&](ExperimentInterface<int>& e)
{
    e.Use([]() { return 1;});
    e.Try([]() { return 1;});
    e.Ignore([]() { return true; });
});
```

See [Ignore tests](test/ignore.cc) for more examples.


# Disable experiments

Experiments can be disabled with `RunIf` function.
If at least one of them (or there is none) return `true`, the experiment is run.
If an exception is caught from any `RunIf` function, the experiment is not run.

```cpp
int res = Scientist<int>::Science("", [&](ExperimentInterface<int>& e)
{
    e.Use([]() { return 42;});
    e.Try([]() { return 1;});
    e.RunIf([]() { return true; });
});
```

See [RunIf tests](test/run_if.cc) for more examples.

# Context

You can add contextual information to observations as string key-value pairs.
Writing same key multiple times overwrites the previous value.
This information can be queried from `Observation` as shown below:

```cpp
Scientist<int>::Science("", [](ExperimentInterface<int>& e)
{
    e.Use([]() { return 42;});
    e.Try([]() { return 0;});
    e.Context("key1", "value");
    e.Context("key2", "value");
    e.Publish([](const Observation<int>& o)
    {
        for (std::string key: o.ContextKeys())
        {
            std::pair<bool, std::string> value = o.Context(key);

            if (value.first)
                std::cout << key << " : " << value.second << std::endl;
        }
    });
});
```

The first value (`bool`) of the returned pair from `Observation::Context` is `true` if the requested key was found.

# Exceptions

Exceptions from both `Try` and `Use` functions are caught and stored in the `Observation`. 
Exceptions from `Use` function are rethrown after all `Publish` functions have been called.

Exceptions can be rethrown with `std::rethrow_exception` and handled accordingly.

# Tests

Tests are written with Google Test. 
To run tests, install CMake and run the following commands:

```bash
cd path/to/scientist
git submodule update --init
mkdir build
cd build
cmake ..
make tests
./tests
```

# TODO

- [ ] Come up with a better name
- [ ] Clean up the SFINAE magic if possible
- [ ] Finalize the API
- [ ] Allow more than one `Try` function
- [ ] Define an interface for publishers and register them separately (See [IResultPublisher](https://github.com/Haacked/Scientist.net/blob/master/src/Scientist/IResultPublisher.cs) in Scientist.NET)
- [x] Add context to experiments (See [Ruby version](https://github.com/github/scientist/blob/master/README.md#adding-context))
