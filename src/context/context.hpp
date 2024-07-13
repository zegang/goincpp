// Copyright 2024 The Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef GOINCPP_CONTEXT_CONTEXT_HPP
#define GOINCPP_CONTEXT_CONTEXT_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <atomic>
#include <mutex>
#include <unordered_set>
#include <optional>
#include <any>
#include <cstring>
#include <cassert>

#include "../errors/errors.hpp"
#include "../runtime/chan.hpp"
#include "../time/timer.hpp"
#include "../reflect/type.hpp"


namespace goincpp {
namespace context {

static std::string
deadlineString(const std::chrono::system_clock::time_point& deadline) {
    auto timeT = std::chrono::system_clock::to_time_t(deadline);
    std::tm tm = *std::localtime(&timeT); // Convert to local time

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S"); // Format the time
    return oss.str();
}

static std::string
timeUntilString(const std::chrono::system_clock::time_point& deadline) {
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(deadline - now);

    if (duration.count() < 0) {
        return "Deadline has passed";
    }

    // Convert duration to hours, minutes, seconds
    int hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
    int minutes = std::chrono::duration_cast<std::chrono::minutes>(duration % std::chrono::hours(1)).count();
    int seconds = duration.count() % 60;

    std::ostringstream oss;
    oss << hours << "h " << minutes << "m " << seconds << "s"; // Format the duration
    return oss.str();
}

using UnbufferedChannel = goincpp::runtime::UnbufferedChannel;
using Error = goincpp::errors::Error;
using ErrorString = goincpp::errors::ErrorString;

// Forward declarations
class DeadlineExceededError;
class Stringer;
class Context;
class EmptyCtx;
class BackgroundCtx;
class TodoCtx;
class Canceler;
class CancelCtx;
class WithoutCancelCtx;
class ValueCtx;

extern std::string stringify(const std::any& v);
extern std::string contextName(std::shared_ptr<Context> c);
extern std::string contextName(const Context* c);
extern Error cause(std::shared_ptr<Context> c);

// Canceled is the error returned by [Context.Err] when the context is canceled.
extern Error canceledError;

class DeadlineExceededError : public ErrorString {
public:
    DeadlineExceededError(const std::string& msg) : ErrorString(msg) {}
    bool timeout() { return true; }
    bool temporary() { return true; }
};

// DeadlineExceeded is the error returned by [Context.Err] when the context's
// deadline passes.
extern Error deadlineExceededError;

// Interface for stringer types
class Stringer {
public:
    virtual std::string string() const { return ""; }
    virtual ~Stringer() = default;
};

// A Context carries a deadline, a cancellation signal, and other values across
// API boundaries.
//
// Context's methods may be called by multiple goroutines simultaneously.
class Context {
public:
    virtual ~Context() = default;

    // Deadline returns the time when work done on behalf of this context
	// should be canceled. Deadline returns ok==false when no deadline is
	// set. Successive calls to Deadline return the same results.
	virtual std::optional<std::chrono::time_point<std::chrono::system_clock>> deadline() const = 0;

    // Done returns a channel that's closed when work done on behalf of this
    // context should be canceled. Done may return nil if this context can
	// never be canceled. Successive calls to Done return the same value.
	// The close of the Done channel may happen asynchronously,
	// after the cancel function returns.
    virtual std::shared_ptr<UnbufferedChannel> done() = 0;

    // If Done is not yet closed, Err returns nil.
	// If Done is closed, Err returns a non-nil error explaining why:
	// Canceled if the context was canceled
	// or DeadlineExceeded if the context's deadline passed.
	// After Err returns a non-nil error, successive calls to Err return the same error.
    virtual Error err() = 0;

    // Value returns the value associated with this context for key, or nil
	// if no value is associated with key. Successive calls to Value with
	// the same key returns the same result.
	//
	// Use context values only for request-scoped data that transits
	// processes and API boundaries, not for passing optional parameters to
	// functions.
	//
    virtual std::optional<std::any> value(const void* key) = 0;
};

// An emptyCtx is never canceled, has no values, and has no deadline.
// It is the common base of backgroundCtx and todoCtx.
class EmptyCtx : public Context {
public:
    virtual std::optional<std::chrono::time_point<std::chrono::system_clock>>
    deadline() const override {
        return {};
    }
    virtual std::shared_ptr<UnbufferedChannel> done() override { return nullptr; }
    virtual Error err() override { return nullptr; }
    virtual std::optional<std::any> value(const void* key) override { return {}; }
};

class BackgroundCtx : public EmptyCtx, public Stringer {
public:
    std::string string() const override {
        return "context.Background";
    }
};

class TodoCtx : public EmptyCtx, public Stringer {
public:
    std::string string() const override {
        return "context.TODO";
    }
};

// Background returns a non-nil, empty [Context]. It is never canceled, has no
// values, and has no deadline. It is typically used by the main function,
// initialization, and tests, and as the top-level Context for incoming
// requests.
extern std::shared_ptr<Context> background();

// TODO returns a non-nil, empty [Context]. Code should use context.TODO when
// it's unclear which Context to use or it is not yet available (because the
// surrounding function has not yet been extended to accept a Context
// parameter).
extern std::shared_ptr<Context> todo();

// A CancelFunc tells an operation to abandon its work.
// A CancelFunc does not wait for the work to stop.
// A CancelFunc may be called by multiple goroutines simultaneously.
// After the first call, subsequent calls to a CancelFunc do nothing.
using CancelFunc = std::function<void()>;
// WithCancel returns a copy of parent with a new Done channel. The returned
// context's Done channel is closed when the returned cancel function is called
// or when the parent context's Done channel is closed, whichever happens first.
//
// Canceling this context releases resources associated with it, so code should
// call cancel as soon as the operations running in this [Context] complete.
extern std::pair<std::shared_ptr<CancelCtx>, CancelFunc> withCancel(std::shared_ptr<Context> parent);

// A CancelCauseFunc behaves like a [CancelFunc] but additionally sets the cancellation cause.
// This cause can be retrieved by calling [Cause] on the canceled Context or on
// any of its derived Contexts.
//
// If the context has already been canceled, CancelCauseFunc does not set the cause.
// For example, if childContext is derived from parentContext:
//   - if parentContext is canceled with cause1 before childContext is canceled with cause2,
//     then Cause(parentContext) == Cause(childContext) == cause1
//   - if childContext is canceled with cause2 before parentContext is canceled with cause1,
//     then Cause(parentContext) == cause1 and Cause(childContext) == cause2
using CancelCauseFunc = std::function<void(Error cause)>;
// WithCancelCause behaves like [WithCancel] but returns a [CancelCauseFunc] instead of a [CancelFunc].
// Calling cancel with a non-nil error (the "cause") records that error in ctx;
// it can then be retrieved using Cause(ctx).
// Calling cancel with nil sets the cause to Canceled.
//
// Example use:
//
//	ctx, cancel := context.WithCancelCause(parent)
//	cancel(myError)
//	ctx.Err() // returns context.Canceled
//	context.Cause(ctx) // returns myError
extern std::pair<std::shared_ptr<CancelCtx>, CancelCauseFunc> withCancelCause(std::shared_ptr<Context> parent);

// Cause returns a non-nil error explaining why c was canceled.
// The first cancellation of c or one of its parents sets the cause.
// If that cancellation happened via a call to CancelCauseFunc(err),
// then [Cause] returns err.
// Otherwise Cause(c) returns the same value as c.Err().
// Cause returns nil if c has not been canceled yet.
Error cause(std::shared_ptr<Context> c);

// A canceler is a context type that can be canceled directly. The
// implementations are *cancelCtx and *timerCtx.
class Canceler : public virtual Context {
public:
    virtual ~Canceler() = default;
	virtual void cancel(bool removeFromParent, Error err, Error cause) = 0;
};

extern std::shared_ptr<UnbufferedChannel> closedChan;
static inline void init() { closedChan->close(); }

// A cancelCtx can be canceled. When canceled, it also cancels any children
// that implement canceler.
class CancelCtx : public Canceler, public std::enable_shared_from_this<CancelCtx> {
public:
    static int cancelCtxKey;

    CancelCtx() : _done(nullptr), _err(nullptr), _cause(nullptr) {}

    virtual std::optional<std::chrono::time_point<std::chrono::system_clock>>
    deadline() const override {
        return std::nullopt;
    }
    virtual std::shared_ptr<UnbufferedChannel> done() override;
    virtual Error err() override { std::lock_guard<std::mutex> lock(_mu); return _err; }
    virtual std::optional<std::any> value(const void* key) override;

    // cancel closes c.done, cancels each of c's children, and, if
    // removeFromParent is true, removes c from its parent's children.
    // cancel sets c.cause to cause if this is the first time c is canceled.
    virtual void cancel(bool removeFromParent, Error err, Error cause) override;

    // propagateCancel arranges for child to be canceled when parent is.
    // It sets the parent context of cancelCtx.
    void propagateCancel(std::shared_ptr<Context> parent, std::shared_ptr<Canceler> child);

    Error cause() const { std::lock_guard<std::mutex> lock(_mu); return _cause; }
    std::unordered_set<std::shared_ptr<Canceler>>& children() {
        std::lock_guard<std::mutex> lock(_mu); return _children;
    }
    std::mutex& mu() const { return _mu; }

    std::shared_ptr<Context> parent() const { return _parent; }

private:
    std::shared_ptr<Context> _parent;
    mutable std::mutex _mu;
    std::atomic< std::shared_ptr< UnbufferedChannel > > _done;
    std::unordered_set<std::shared_ptr<Canceler>> _children;
    Error _err;
    Error _cause;
};

// WithoutCancel returns a copy of parent that is not canceled when parent is canceled.
// The returned context returns no Deadline or Err, and its Done channel is nil.
// Calling [Cause] on the returned context returns nil.
extern std::shared_ptr<Context> withoutCancel(std::shared_ptr<Context> parent);

class WithoutCancelCtx : public Context, public Stringer,
    public std::enable_shared_from_this<WithoutCancelCtx> {
public:
    WithoutCancelCtx(std::shared_ptr<Context> parent) : _parent(parent) {}

    virtual std::optional<std::chrono::time_point<std::chrono::system_clock>>
    deadline() const override {
        return std::nullopt;
    }
    virtual std::shared_ptr<UnbufferedChannel> done() { return nullptr; }
    virtual Error err() override { return nullptr; }
    virtual std::optional<std::any> value(const void* key) override;
    virtual std::string string() const override {
        return contextName(_parent) + ".WithoutCancel";
    }

    std::shared_ptr<Context> parent() const { return _parent; }

private:
    std::shared_ptr<Context> _parent;
};


// WithDeadlineCause behaves like [WithDeadline] but also sets the cause of the
// returned Context when the deadline is exceeded. The returned [CancelFunc] does
// not set the cause.
extern std::pair<std::shared_ptr<Context>, CancelFunc> withDeadlineCause(std::shared_ptr<Context> parent,
    std::chrono::time_point<std::chrono::system_clock> d, Error cause);

// WithDeadline returns a copy of the parent context with the deadline adjusted
// to be no later than d. If the parent's deadline is already earlier than d,
// WithDeadline(parent, d) is semantically equivalent to parent. The returned
// [Context.Done] channel is closed when the deadline expires, when the returned
// cancel function is called, or when the parent context's Done channel is
// closed, whichever happens first.
//
// Canceling this context releases resources associated with it, so code should
// call cancel as soon as the operations running in this [Context] complete.
extern std::pair<std::shared_ptr<Context>, CancelFunc> withDeadline(std::shared_ptr<Context> parent,
    std::chrono::time_point<std::chrono::system_clock> d);

// A timerCtx carries a timer and a deadline. It embeds a cancelCtx to
// implement Done and Err. It implements cancel by stopping its timer then
// delegating to cancelCtx.cancel.
class TimerCtx : public CancelCtx, public Stringer {
public:
    TimerCtx(std::chrono::time_point<std::chrono::system_clock> deadline) : _deadline(deadline) {}

    virtual std::optional<std::chrono::time_point<std::chrono::system_clock>>
    deadline() const override {
        return _deadline;
    }

    virtual std::string string() const override {
        return contextName(parent()) + ".WithDeadline(" +
            deadlineString(_deadline) + " [" + timeUntilString(_deadline) + "])";
    }

    virtual void cancel(bool removeFromParent, Error err, Error cause) override {
        CancelCtx::cancel(false, err, cause);
        if (removeFromParent) {
            // Remove this timerCtx from its parent cancelCtx's children.
            // removeChild(parent(), this);
        }
        mu().lock();
        if (_timer.isRunning()) {
            _timer.stop();
        }
        mu().unlock();
    }

    time::Timer& timer() { return _timer; }

private:
    std::chrono::time_point<std::chrono::system_clock> _deadline;
    time::Timer _timer;
};

// WithTimeout returns WithDeadline(parent, time.Now().Add(timeout)).
//
// Canceling this context releases resources associated with it, so code should
// call cancel as soon as the operations running in this [Context] complete:
//
//	func slowOperationWithTimeout(ctx context.Context) (Result, error) {
//		ctx, cancel := context.WithTimeout(ctx, 100*time.Millisecond)
//		defer cancel()  // releases resources if slowOperation completes before timeout elapses
//		return slowOperation(ctx)
//	}
extern std::pair<std::shared_ptr<Context>, CancelFunc> withTimeout(std::shared_ptr<Context> parent,
    std::chrono::steady_clock::duration timeout);

// WithTimeoutCause behaves like [WithTimeout] but also sets the cause of the
// returned Context when the timeout expires. The returned [CancelFunc] does
// not set the cause.
extern std::pair<std::shared_ptr<Context>, CancelFunc> withTimeoutCause(std::shared_ptr<Context> parent,
    std::chrono::steady_clock::duration timeout, Error cause);

// A valueCtx carries a key-value pair. It implements Value for that key and
// delegates all other calls to the embedded Context.
class ValueCtx : public Context, public Stringer {
public:
    ValueCtx(std::shared_ptr<Context> parent, const void* k, size_t k_size, const std::any& v) :
        _parent(parent), _value(v) {
            if (k_size == 0) {
                _key = nullptr;
            } else {
                _key = malloc(k_size);
                assert(_key != nullptr);
                std::memcpy(_key, k, k_size);
            }
        }

    void* key() { return _key; }
    std::any value() { return _value; }

    virtual std::optional<std::chrono::system_clock::time_point>
    deadline() const override {
        return std::nullopt;
    }
    virtual std::shared_ptr<UnbufferedChannel> done() override { return nullptr; }
    virtual Error err() override { return nullptr; }
    virtual std::optional<std::any> value(const void* key) override;
    std::string string() const override {
        return contextName(this) + ".WithValue(" +
            stringify(_key) + ", " + stringify(_value) + ")";
    }

    auto parent() const { return _parent; }

private:
    std::shared_ptr<Context> _parent;
    void* _key;
    std::any _value;
};

// WithValue returns a copy of parent in which the value associated with key is
// val.
//
// Use context Values only for request-scoped data that transits processes and
// APIs, not for passing optional parameters to functions.
//
// The provided key must be comparable and should not be of type
// string or any other built-in type to avoid collisions between
// packages using context. Users of WithValue should define their own
// types for keys. To avoid allocating when assigning to an
// interface{}, context keys often have concrete type
// struct{}. Alternatively, exported context key variables' static
// type should be a pointer or interface.
extern std::shared_ptr<Context> withValue(std::shared_ptr<Context> parent,
    const std::any& key, const std::any& val);

template <typename T>
std::string stringify(const T& v) {
    return typeid(v).name();
}

template <>
std::string stringify<std::shared_ptr<Stringer>>(const std::shared_ptr<Stringer>& v) {
    return v->string();
}

/// @brief 
/// @param v 
/// @return 
template <>
std::string stringify<Stringer>(const Stringer& v) {
    return v.string();
}

template <>
std::string stringify<std::string>(const std::string& v) {
    return v;
}

template <>
std::string stringify<std::nullptr_t>(const std::nullptr_t& v) {
    return "<nil>";
}

template<typename T, typename V>
std::shared_ptr<Context>
withValue(std::shared_ptr<Context> parent, const T& key, const V& val)
{
    if (!parent) {
        throw std::invalid_argument("cannot create context from nil parent");
    }
    if (key == nullptr) {
        throw std::invalid_argument("nil key");
    }
    if (!goincpp::reflect::is_comparable<T>::value) {
        throw std::invalid_argument("key is not comparable");
    }
    return std::make_shared<ValueCtx>(parent, key, val);
}

}
}

#endif // GOINCPP_CONTEXT_CONTEXT_HPP