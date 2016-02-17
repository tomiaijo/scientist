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

template <class T>
class Observation
{
public:
    Observation(std::string name, bool success,
                std::chrono::nanoseconds controlDuration, std::exception_ptr controlException, T controlResult,
                std::chrono::nanoseconds candidateDuration, std::exception_ptr candidateException, T candidateResult) :
            name_(std::move(name)), success_(success),
            controlDuration_(controlDuration), controlException_(controlException), controlResult_(controlResult),
            candidateDuration_(candidateDuration), candidateException_(candidateException), candidateResult_(candidateResult)
    {}

    const std::string& Name() const { return name_; }
    bool Success() const { return success_; }

    std::chrono::nanoseconds ControlDuration() const { return controlDuration_; }
    std::exception_ptr ControlException() const { return controlException_; }
    T ControlResult() const { return controlResult_; }

    std::chrono::nanoseconds CandidateDuration() const { return candidateDuration_; }
    std::exception_ptr CandidateException() const { return candidateException_; }
    T CandidateResult() const { return candidateResult_; }

private:
    std::string name_;
    bool success_;
    std::chrono::nanoseconds controlDuration_;
    std::exception_ptr controlException_;
    T controlResult_;

    std::chrono::nanoseconds candidateDuration_;
    std::exception_ptr candidateException_;
    T candidateResult_;
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
    Experiment(std::string name, Operation<T> control, Operation<T> candidate,
               std::list<Predicate> ignorePredicates,
               std::list<Predicate> runIfPredicates, std::list<Publisher<U>> publishers,
               std::list<Publisher<U>> asyncPublishers, Transform<T,U> cleanup,
               Compare<T> compare) :
            name_(name), control_(control), candidate_(candidate),
            ignorePredicates_(ignorePredicates), runIfPredicates_(runIfPredicates),
            publishers_(publishers), asyncPublishers_(asyncPublishers),
            cleanup_(cleanup), compare_(compare)
    {

    }

    T Run() const
    {
        if (!RunCandidate())
            return control_();

        T controlResult;
        T candidateResult;
        std::chrono::nanoseconds controlDuration;
        std::chrono::nanoseconds candidateDuration;
        std::exception_ptr controlException;
        std::exception_ptr candidateException;

        if (RunControlFirst())
        {
            std::tie(controlResult, controlDuration, controlException) = Measure(control_);
            std::tie(candidateResult, candidateDuration, candidateException) = Measure(candidate_);

        } else
        {
            std::tie(candidateResult, candidateDuration, candidateException) = Measure(candidate_);
            std::tie(controlResult, controlDuration, controlException) = Measure(control_);
        }

        bool controlThrew = static_cast<bool>(controlException);
        bool candidateThrew = static_cast<bool>(candidateException);

        bool success = (compare_ && compare_(controlResult, candidateResult) && controlThrew == candidateThrew) || Ignored();

        Observation<U> observation(name_, success, controlDuration, controlException, Cleanup(controlResult),
                                candidateDuration, candidateException, Cleanup(candidateResult));
        Publish(observation);

        if (controlException)
            std::rethrow_exception(controlException);

        return controlResult;
    }

private:
    // TODO: investigate if this actually necessary
    template <class Q = T>
    typename std::enable_if<std::is_same<Q, U>::value, U>::type
    Cleanup(const T& value) const
    {
        if (!cleanup_)
            return value;
        return cleanup_(value);
    }

    template <class Q = T>
    typename std::enable_if<!std::is_same<Q, U>::value, U>::type
    Cleanup(const T& value) const
    {
        if (!cleanup_)
            return U();
        return cleanup_(value);
    }

    bool RunControlFirst() const
    {
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<> dist(0, 1);

        return dist(mt) == 0;
    }

    std::tuple<T, std::chrono::nanoseconds, std::exception_ptr> Measure(Operation<T> f) const
    {
        auto start = std::chrono::steady_clock::now();
        T result;
        std::exception_ptr exception;
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
    };

    bool Ignored() const
    {
        try
        {
            return std::any_of(ignorePredicates_.begin(), ignorePredicates_.end(), [](Predicate p) { return p(); });
        }
        catch(...)
        {
            return true;
        }
    }

    bool RunCandidate() const
    {
        try
        {
            return std::all_of(runIfPredicates_.begin(), runIfPredicates_.end(), [](Predicate p) { return p(); });
        }
        catch(...)
        {
            return false;
        }

    }

    void Publish(const Observation<U>& observation) const
    {
        for (Publisher<U> p: publishers_)
            p(observation);
        for (Publisher<U> p: asyncPublishers_)
            std::thread(std::bind(p, observation)).detach();
    }

    std::string name_;
    Operation<T> control_;
    Operation<T> candidate_;
    Compare<T> compare_;
    std::list<Predicate> ignorePredicates_;
    std::list<Predicate> runIfPredicates_;
    std::list<Publisher<U>> publishers_;
    std::list<Publisher<U>> asyncPublishers_;
    Transform<T,U> cleanup_;
};

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
};

template <class T, class U>
class ExperimentBuilder : public ExperimentInterface<T, U>
{
public:
    ExperimentBuilder(std::string name) : name_(std::move(name)) {}

    virtual void Use(Operation<T> control) override
    {
        control_ = control;
    }

    virtual void Try(Operation<T> candidate) override
    {
        candidate_ = candidate;
    }

    virtual void Ignore(Predicate ignore)
    {
        ignorePredicates_.push_back(ignore);
    }

    virtual void RunIf(Predicate runIf)
    {
        runIfPredicates_.push_back(runIf);
    }

    virtual void Publish(Publisher<U> publisher)
    {
        publishers_.push_back(publisher);
    }

    virtual void PublishAsync(Publisher<U> publisher)
    {
        asyncPublishers_.push_back(publisher);
    }

    virtual void Compare(::Compare<T> compare)
    {
        compare_ = compare;
    }

    virtual void Cleanup(Transform<T,U> cleanup)
    {
        cleanup_ = cleanup;
    }

    template <class Q = T>
    typename std::enable_if<has_operator_equal<Q>::value, Experiment<T,U>>::type
    Build() const
    {
        return Experiment<T, U>(name_, control_, candidate_, ignorePredicates_, runIfPredicates_,
                                publishers_, asyncPublishers_, cleanup_, compare_ ? compare_ : std::equal_to<T>());
    }

    template <class Q = T>
    typename std::enable_if<!has_operator_equal<Q>::value, Experiment<T,U>>::type
    Build() const
    {
        return Experiment<T, U>(name_, control_, candidate_, ignorePredicates_, runIfPredicates_,
                                publishers_, asyncPublishers_, cleanup_, compare_);
    }
private:
    std::string name_;
    Operation<T> control_;
    Operation<T> candidate_;
    std::list<Predicate> ignorePredicates_;
    std::list<Predicate> runIfPredicates_;
    std::list<Publisher<U>> publishers_;
    std::list<Publisher<U>> asyncPublishers_;
    Transform<T,U> cleanup_;
    ::Compare<T> compare_;
};

template <class T, class U = T>
class Scientist
{
public:

    static T Science(std::string name, std::function<void(ExperimentInterface<T, U>&)> experimentDefinition)
    {
        ExperimentBuilder<T, U> builder(name);
        experimentDefinition(builder);
        Experiment<T, U> experiment = builder.Build();
        return experiment.Run();
    }
};

#endif //SCIENTIST_HH
