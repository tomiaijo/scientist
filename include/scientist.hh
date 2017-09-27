#ifndef SCIENTIST_HH
#define SCIENTIST_HH

#include <algorithm>
#include <chrono>
#include <exception>
#include <iostream>
#include <list>
#include <random>
#include <thread>
#include <tuple>
#include <unordered_map>

template <class T>
using Operation = std::function<T()>;

template <class T>
using Compare = std::function<bool(const T&, const T&)>;

using Predicate = std::function<bool()>;

template <class T>
class Observation;
template <class T>
using Publisher = std::function<void(const Observation<T>&)>;

template <class T, class U>
using Transform = std::function<U(const T&)>;

using Setup = std::function<void()>;

template <class T>
class Observation
{
public:
    Observation(std::string name, bool success, std::unordered_map<std::string, std::string> context,
                std::tuple<T, std::chrono::nanoseconds, std::exception_ptr> control,
                std::vector<std::tuple<T, std::chrono::nanoseconds, std::exception_ptr>> candidates) :
            name_(std::move(name)), success_(success), context_(context),
            control_(control),
            candidates_(candidates)
    {
    }

    const std::string& Name() const { return name_; }
    bool Success() const { return success_; }

    std::chrono::nanoseconds ControlDuration() const { return std::get<1>(control_); }
    std::exception_ptr ControlException() const { return std::get<2>(control_); }
    T ControlResult() const { return std::get<0>(control_); }

    std::size_t NumberOfCandidates() const
    {
        return candidates_.size();
    }

    std::vector<std::chrono::nanoseconds> CandidateDurations() const
    {
        std::vector<std::chrono::nanoseconds> result;

        result.reserve(candidates_.size());

        for (const auto &candidate : candidates_)
        {
            result.push_back(std::get<1>(candidate));
        }

        return result;
    }

    std::chrono::nanoseconds CandidateDuration(const std::size_t index = 0) const
    {
        std::chrono::nanoseconds result;

        if (index < candidates_.size())
        {
            result = std::get<1>(candidates_.at(index));
        }

        return result;
    }

    std::vector<std::exception_ptr> CandidateExceptions() const
    {
        std::vector<std::exception_ptr> result;

        result.reserve(candidates_.size());

        for (const auto &candidate : candidates_)
        {
            result.push_back(std::get<2>(candidate));
        }

        return result;
    }

    std::exception_ptr CandidateException(const std::size_t index = 0) const
    {
        std::exception_ptr result;

        if (index < candidates_.size())
        {
            result = std::get<2>(candidates_.at(index));
        }

        return result;
    }

    std::vector<T> CandidateResults() const
    {
        std::vector<T> result;

        result.reserve(candidates_.size());

        for (const auto &candidate : candidates_)
        {
            result.push_back(std::get<0>(candidate));
        }

        return result;
    }

    T CandidateResult(const std::size_t index = 0) const
    {
        T result;

        if (index < candidates_.size())
        {
            result = std::get<0>(candidates_.at(index));
        }

        return result;
    }

    std::vector<std::string> ContextKeys() const
    {
        std::vector<std::string> keys;

        for (const auto& p : context_)
        {
            keys.push_back(p.first);
        }

        return keys;
    }

    std::pair<bool, std::string> Context(std::string key) const
    {
        std::unordered_map<std::string, std::string>::const_iterator ret = context_.find(key);

        std::pair<bool, std::string> result(false, std::string());

        if (ret != context_.cend())
        {
            result = std::make_pair(true, ret->second);
        }

        return result;
    }

private:
    std::string name_;
    bool success_;
    std::unordered_map<std::string, std::string> context_;

    std::tuple<T, std::chrono::nanoseconds, std::exception_ptr> control_;

    std::vector<std::tuple<T, std::chrono::nanoseconds, std::exception_ptr>> candidates_;
};

template<class T>
struct has_operator_equal_impl
{
    template<class U>
    static auto test(U*) -> decltype(std::declval<U>() == std::declval<U>());
    template<typename>
    static auto test(...) -> std::false_type;

    using type = typename std::is_same<bool, decltype(test<T>(0))>::type;
};

template<class T>
struct has_operator_equal : has_operator_equal_impl<T>::type {};

template <class T, class U>
class Experiment
{
public:
    using Measurement = std::tuple<T, std::chrono::nanoseconds, std::exception_ptr>;

    Experiment(std::string name, std::unordered_map<std::string, std::string> context,
               std::list<::Setup> setups,
               Operation<T> control, std::vector<Operation<T>> candidates,
               std::list<Predicate> ignorePredicates,
               std::list<Predicate> runIfPredicates, std::list<Publisher<U>> publishers,
               std::list<Publisher<U>> asyncPublishers, Transform<T,U> cleanup,
               Compare<T> compare) :
            name_(name), context_(context), setups_(setups), control_(control), candidates_(candidates),
            ignorePredicates_(ignorePredicates), runIfPredicates_(runIfPredicates),
            publishers_(publishers), asyncPublishers_(asyncPublishers),
            compare_(compare), cleanup_(cleanup)
    {
    }

    T Run() const
    {
        if (!RunCandidate())
            return control_();

        Setup();

        std::tuple<T, Observation<U>> result = MeasureBoth();
        Observation<U> observation = std::get<1>(result);
        T controlResult = std::get<0>(result);

        Publish(observation);

        if (observation.ControlException())
        {
            std::rethrow_exception(observation.ControlException());
        }

        return controlResult;
    }

private:

    void Setup() const
    {
        for (::Setup s: setups_)
        {
            s();
        }
    }

    std::tuple<T, Observation<U>> MeasureBoth() const
    {
        std::size_t index = std::numeric_limits<std::size_t>::min();

        std::vector<std::size_t> indices;

        indices.resize(candidates_.size() + 1);

        std::generate_n(indices.begin(), candidates_.size() + 1, [&index]() { return ++index; });

        std::random_device rd;
        std::mt19937 mt(rd());
        std::shuffle(indices.begin(), indices.end(), mt);

        Measurement control;
        std::vector<Measurement> candidates;

        candidates.resize(candidates_.size());

        for (const auto i : indices)
        {
            if (i == index)
            {
                control = Measure(control_);
            }
            else
            {
                candidates[i] = Measure(candidates_[i]);
            }
        }

        return std::make_tuple(std::get<0>(control), CreateObservation(control, candidates));
    }

    Observation<U> CreateObservation(Measurement control, std::vector<Measurement> candidates) const
    {
        T controlResult = std::get<0>(control);
        bool controlThrew = static_cast<bool>(std::get<2>(control));

        bool success = std::all_of(candidates.cbegin(), candidates.cend(), [=](Measurement measurement)
        {
            T result = std::get<0>(measurement);
            bool threw = static_cast<bool>(std::get<2>(measurement));

            return ((compare_ && compare_(controlResult, result) && (controlThrew == threw)) || Ignored());
        });

        Observation<U> result(name_, success, context_, Cleanup(control), Cleanup(candidates));

        return result;
    }

    // TODO: investigate if this actually necessary
    template <class Q = T>
    typename std::enable_if<std::is_same<Q, U>::value, std::tuple<U, std::chrono::nanoseconds, std::exception_ptr>>::type
    Cleanup(const std::tuple<T, std::chrono::nanoseconds, std::exception_ptr> &value) const
    {
        if (!cleanup_)
        {
            return value;
        }

        std::tuple<U, std::chrono::nanoseconds, std::exception_ptr> result(cleanup_(std::get<0>(value)), std::get<1>(value), std::get<2>(value));

        return result;
    }

    template <class Q = T>
    typename std::enable_if<std::is_same<Q, U>::value == false, std::tuple<U, std::chrono::nanoseconds, std::exception_ptr>>::type
    Cleanup(const std::tuple<T, std::chrono::nanoseconds, std::exception_ptr> &value) const
    {
        if (!cleanup_)
        {
            return std::tuple<U, std::chrono::nanoseconds, std::exception_ptr>(U(), std::get<1>(value), std::get<2>(value));
        }

        std::tuple<U, std::chrono::nanoseconds, std::exception_ptr> result(cleanup_(std::get<0>(value)), std::get<1>(value), std::get<2>(value));

        return result;
    }

    // TODO: investigate if this actually necessary
    template <class Q = T>
    typename std::enable_if<std::is_same<Q, U>::value, std::vector<std::tuple<U, std::chrono::nanoseconds, std::exception_ptr>>>::type
    Cleanup(const std::vector<std::tuple<T, std::chrono::nanoseconds, std::exception_ptr>> &value) const
    {
        std::vector<std::tuple<U, std::chrono::nanoseconds, std::exception_ptr>> result;

        result.resize(value.size());

        if (!cleanup_)
        {
            result = value;
        }
        else
        {
            std::transform(value.begin(), value.end(), result.begin(), [this](std::tuple<T, std::chrono::nanoseconds, std::exception_ptr> item)
                {
                    std::tuple<U, std::chrono::nanoseconds, std::exception_ptr> transformed(cleanup_(std::get<0>(item)), std::get<1>(item), std::get<2>(item));

                    return transformed;
                });
        }

        return result;
    }

    template <class Q = T>
    typename std::enable_if<std::is_same<Q, U>::value == false, std::vector<std::tuple<U, std::chrono::nanoseconds, std::exception_ptr>>>::type
    Cleanup(const std::vector<std::tuple<T, std::chrono::nanoseconds, std::exception_ptr>> &value) const
    {
        std::vector<std::tuple<U, std::chrono::nanoseconds, std::exception_ptr>> result;

        result.resize(value.size());

        std::function<std::tuple<U, std::chrono::nanoseconds, std::exception_ptr>(std::tuple<T, std::chrono::nanoseconds, std::exception_ptr>)> transformer;

        if (!cleanup_)
        {
            transformer = [](std::tuple<T, std::chrono::nanoseconds, std::exception_ptr> item)
                {
                    std::tuple<U, std::chrono::nanoseconds, std::exception_ptr> transformed(U(), std::get<1>(item), std::get<2>(item));

                    return transformed;
                };
        }
        else
        {
            transformer = [this](std::tuple<T, std::chrono::nanoseconds, std::exception_ptr> item)
                {
                    std::tuple<U, std::chrono::nanoseconds, std::exception_ptr> transformed(cleanup_(std::get<0>(item)), std::get<1>(item), std::get<2>(item));

                    return transformed;
                };
        }

        std::transform(value.begin(), value.end(), result.begin(), transformer);

        return result;
    }

    Measurement Measure(Operation<T> f) const
    {
        T result;
        std::exception_ptr exception;
        auto start = std::chrono::steady_clock::now();

        try
        {
            result = f();
        }
        catch(...)
        {
            exception = std::current_exception();
        }

        auto end = std::chrono::steady_clock::now();
        return std::make_tuple(result, std::chrono::nanoseconds(end - start), exception);
    }

    bool Ignored() const
    {
        bool result = true;

        try
        {
            result = std::any_of(ignorePredicates_.begin(), ignorePredicates_.end(), [](Predicate p) { return p(); });
        }
        catch(...)
        {
        }

        return result;
    }

    bool RunCandidate() const
    {
        bool result = false;

        try
        {
            result = std::all_of(runIfPredicates_.begin(), runIfPredicates_.end(), [](Predicate p) { return p(); });
        }
        catch(...)
        {
        }

        return result;

    }

    void Publish(const Observation<U>& observation) const
    {
        for (Publisher<U> p: publishers_)
            p(observation);
        for (Publisher<U> p: asyncPublishers_)
            std::thread(std::bind(p, observation)).detach();
    }

    std::string name_;
    std::unordered_map<std::string, std::string> context_;
    std::list<::Setup> setups_;
    Operation<T> control_;
    std::vector<Operation<T>> candidates_;
    std::list<Predicate> ignorePredicates_;
    std::list<Predicate> runIfPredicates_;
    std::list<Publisher<U>> publishers_;
    std::list<Publisher<U>> asyncPublishers_;
    Compare<T> compare_;
    Transform<T,U> cleanup_;
};

template <class T, class U = T>
class ExperimentInterface
{
public:
    virtual ~ExperimentInterface() {}
    virtual void BeforeRun(Setup setup) = 0;
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

template <class T, class U>
class ExperimentBuilder : public ExperimentInterface<T, U>
{
public:
    ExperimentBuilder(std::string name) : name_(std::move(name)) {}
    virtual ~ExperimentBuilder() {}

    virtual void BeforeRun(Setup setup) override
    {
        setups_.push_back(setup);
    }

    virtual void Use(Operation<T> control) override
    {
        control_ = control;
    }

    virtual void Try(Operation<T> candidate) override
    {
        candidates_.push_back(candidate);
    }

    virtual void Ignore(Predicate ignore) override
    {
        ignorePredicates_.push_back(ignore);
    }

    virtual void RunIf(Predicate runIf) override
    {
        runIfPredicates_.push_back(runIf);
    }

    virtual void Publish(Publisher<U> publisher) override
    {
        publishers_.push_back(publisher);
    }

    virtual void PublishAsync(Publisher<U> publisher) override
    {
        asyncPublishers_.push_back(publisher);
    }

    virtual void Compare(::Compare<T> compare) override
    {
        compare_ = compare;
    }

    virtual void Cleanup(Transform<T,U> cleanup) override
    {
        cleanup_ = cleanup;
    }

    virtual void Context(std::string key, std::string value) override
    {
        context_[key] = value;
    }

    template <class Q = T>
    typename std::enable_if<has_operator_equal<Q>::value, Experiment<T,U>>::type
    Build() const
    {
        return Experiment<T, U>(name_, context_, setups_, control_, candidates_, ignorePredicates_, runIfPredicates_,
                                publishers_, asyncPublishers_, cleanup_, compare_ ? compare_ : std::equal_to<T>());
    }

    template <class Q = T>
    typename std::enable_if<!has_operator_equal<Q>::value, Experiment<T,U>>::type
    Build() const
    {
        return Experiment<T, U>(name_, context_, setups_, control_, candidates_, ignorePredicates_, runIfPredicates_,
                                publishers_, asyncPublishers_, cleanup_, compare_);
    }
private:
    std::string name_;
    std::list<Setup> setups_;
    Operation<T> control_;
    std::vector<Operation<T>> candidates_;
    std::list<Predicate> ignorePredicates_;
    std::list<Predicate> runIfPredicates_;
    std::list<Publisher<U>> publishers_;
    std::list<Publisher<U>> asyncPublishers_;
    Transform<T,U> cleanup_;
    ::Compare<T> compare_;
    std::unordered_map<std::string, std::string> context_;
};

template <class T, class U = T>
class Scientist
{
public:

    static T Science(std::string name, std::function<void (ExperimentInterface<T, U>&)> experimentDefinition)
    {
        ExperimentBuilder<T, U> builder(name);
        experimentDefinition(builder);
        Experiment<T, U> experiment = builder.Build();
        return experiment.Run();
    }
};

#endif //SCIENTIST_HH
