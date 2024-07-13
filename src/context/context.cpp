// Copyright 2024 The Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "context.hpp"
#include <exception>

namespace goincpp {
namespace context {

// Forward declarations
static std::pair<std::shared_ptr<CancelCtx>, bool> parentCancelCtx(std::shared_ptr<Context> parent);
static std::optional<std::any> value(std::shared_ptr<Context> c, const void* key);
static void removeChild(std::shared_ptr<Context> parent, std::shared_ptr<Canceler> child);

std::string
stringify(const std::any& v) {
    if (v.type() == typeid(std::shared_ptr<Stringer>)) {
        return std::any_cast<std::shared_ptr<Stringer>>(v)->string();
    } else if (v.type() == typeid(Stringer)) {
        return std::any_cast<Stringer>(v).string();
    } else if (v.type() == typeid(std::string)) {
        return std::any_cast<std::string>(v);
    } else if (v.type() == typeid(nullptr_t)) {
        return "<nil>";
    }
    return typeid(v).name(); // Fallback to type name
}

std::string
contextName(std::shared_ptr<Context> c) {
    if (auto s = dynamic_cast<Stringer*>(c.get())) {
        return s->string();
    }
    return typeid(c.get()).name();
}

std::string
contextName(const Context* c) {
    if (auto s = dynamic_cast<const Stringer*>(c)) {
        return s->string();
    }
    return typeid(c).name();
}

Error
cause(std::shared_ptr<Context> c) {
    auto cc = c->value((void*)&CancelCtx::cancelCtxKey);
    if (cc.has_value() && cc.value().type() == typeid(std::shared_ptr<CancelCtx>)) {
        return std::any_cast<std::shared_ptr<CancelCtx>>(cc.value())->cause();
    }
    return c->err();
}

// parentCancelCtx returns the underlying *cancelCtx for parent.
// It does this by looking up parent.Value(&cancelCtxKey) to find
// the innermost enclosing *cancelCtx and then checking whether
// parent.Done() matches that *cancelCtx. (If not, the *cancelCtx
// has been wrapped in a custom implementation providing a
// different done channel, in which case we should not bypass it.)
static std::pair<std::shared_ptr<CancelCtx>, bool>
parentCancelCtx(std::shared_ptr<Context> parent) {
    auto done = parent->done();
    if (done == closedChan || done == nullptr) {
        return { nullptr, false };
    }

    auto p = parent->value((void*)&CancelCtx::cancelCtxKey);
    if (!p) {
        return { nullptr, false };
    }

    std::any pV = p.value();
    if (pV.type() != typeid(std::shared_ptr<CancelCtx>)) {
        return { nullptr, false };
    }

    auto pCanelCtx = std::any_cast<std::shared_ptr<CancelCtx>>(pV);
    auto pDone = pCanelCtx->done();
    if (pDone != done) {
        return { nullptr, false };
    }
    return { pCanelCtx, true };
}

static void
removeChild(std::shared_ptr<Context> parent, std::shared_ptr<Canceler> child) {
    auto [p, ok] = parentCancelCtx(parent);
    if (!ok) {
        return;
    }

    std::lock_guard<std::mutex> lock(p->mu());
    if (!p->children().empty()) {
        p->children().erase(child);
    }
}

Error canceledError = goincpp::errors::newError("context canceled");
Error deadlineExceededError = goincpp::errors::newError<DeadlineExceededError>("context deadline exceeded");

std::shared_ptr<UnbufferedChannel> closedChan = UnbufferedChannel::make();

int CancelCtx::cancelCtxKey = 0;

std::shared_ptr<UnbufferedChannel>
CancelCtx::done() {
    auto d = _done.load();
    if (d != nullptr) {
        return d;
    }
    std::lock_guard<std::mutex> lock(_mu);
    d = _done.load();
    if (d == nullptr) {
        _done.store(UnbufferedChannel::make());
    }
    return _done.load();
}

std::optional<std::any>
CancelCtx::value(const void* key) {
    if (key == (void*)&CancelCtx::cancelCtxKey) {
        return std::optional<std::any>(std::any(shared_from_this()));
    }
    return context::value(_parent, key);
}

// propagateCancel arranges for child to be canceled when parent is.
// It sets the parent context of cancelCtx.
void
CancelCtx::propagateCancel(std::shared_ptr<Context> parent,
                           std::shared_ptr<Canceler> child) {
    _parent = parent;

    auto done = parent->done();
    if (done == nullptr) {
        return; // parent is never canceled
    }

    if (done->select()) {
        // parent is already canceled
        child->cancel(false, parent->err(), context::cause(parent));
        return;
    }

    auto [p, ok] = parentCancelCtx(parent);
    if (ok) {
        // parent is a *cancelCtx, or derives from one.
        p->mu().lock();
        if (p->err() != nullptr) {
            // parent has already been canceled
            child->cancel(false, p->err(), p->cause());
        } else {
            p->children().clear();
        }
        p->mu().unlock();
        return;
    }
}

void
CancelCtx::cancel(bool removeFromParent, Error err, Error cause) {
    if (err == nullptr) {
        throw std::invalid_argument("context: internal error: missing cancel error");
    }
    if (cause == nullptr) {
        cause = err;
    }

    do {
        std::lock_guard<std::mutex> lock(_mu);
        if (_err) {
            return; // already canceled
        }

        _err = err;
        _cause = cause;

        auto d = _done.load();
        if (d == nullptr) {
            _done.store(closedChan);
        } else {
            d->close();
        }

        for (auto& child : _children) {
            // Cancel child cancelers if any
            child->cancel(false, err, cause);
        }
        _children.clear();
    } while(0);

    if (removeFromParent) {
        removeChild(_parent, shared_from_this());
    }
}

std::optional<std::any>
WithoutCancelCtx::value(const void* key) {
    return context::value(shared_from_this(), key);
}

std::optional<std::any>
ValueCtx::value(const void* key) {
    if (key == _key) {
        return { _value };
    }
    return context::value(_parent, key);
}

//
//  WithXXX Wrappers
//

std::shared_ptr<Context> background() {
    return std::make_shared<BackgroundCtx>();
}

std::shared_ptr<Context> todo() {
    return std::make_shared<TodoCtx>();
}

static std::shared_ptr<CancelCtx>
_withCancel(std::shared_ptr<Context> parent) {
    if (parent == nullptr) {
        throw std::invalid_argument("cannot create context from nil parent");
    }
    auto c = std::make_shared<CancelCtx>();
    c->propagateCancel(parent, c);
    return c;
}

std::pair<std::shared_ptr<CancelCtx>, CancelFunc>
withCancel(std::shared_ptr<Context> parent) {
    auto c = _withCancel(parent);
    return { c, [c]() { c->cancel(true, canceledError, nullptr); } };
}

std::pair<std::shared_ptr<CancelCtx>, CancelCauseFunc>
withCancelCause(std::shared_ptr<Context> parent) {
    auto c = _withCancel(parent);
    return {c, [c](Error cause) { c->cancel(true, canceledError, cause); } };
}

std::shared_ptr<Context>
withoutCancel(std::shared_ptr<Context> parent) {
    if (parent == nullptr) {
         throw std::invalid_argument("cannot create context from nil parent");
    }
    return std::make_shared<WithoutCancelCtx>(parent);
}

std::pair<std::shared_ptr<Context>, CancelFunc>
withDeadlineCause(std::shared_ptr<Context> parent,
                  std::chrono::system_clock::time_point d,
                  Error cause) {
    if (parent == nullptr) {
        throw std::invalid_argument("cannot create context from nil parent");
    }
    auto pD = parent->deadline();
    if (pD.has_value() && std::chrono::system_clock::now() < d) {
        // The current deadline is already sooner than the new one.
        return withCancel(parent);
    }
    auto c = std::make_shared<TimerCtx>(d);
    c->propagateCancel(parent, c);
    std::lock_guard<std::mutex> lock(c->mu());
    if (c->err() == nullptr) {
        c->timer().start(time::util(d),
                         [c, cause]() {
                            c->cancel(true, deadlineExceededError, cause);
                         });
    }
    return {c, [c] () { c->cancel(true, canceledError, nullptr); } };
}

std::pair<std::shared_ptr<Context>, CancelFunc>
withDeadline(std::shared_ptr<Context> parent,
             std::chrono::system_clock::time_point  d) {
	return withDeadlineCause(parent, d, nullptr);
}

std::pair<std::shared_ptr<Context>, CancelFunc>
withTimeout(std::shared_ptr<Context> parent, std::chrono::system_clock::duration timeout) {
    return withDeadline(parent, std::chrono::system_clock::now() + timeout);
}

std::pair<std::shared_ptr<Context>, CancelFunc>
withTimeoutCause(std::shared_ptr<Context> parent, std::chrono::system_clock::duration timeout, Error cause) {
    return withDeadlineCause(parent, std::chrono::system_clock::now() + timeout, cause);
}

std::shared_ptr<Context>
withValue(std::shared_ptr<Context> parent, const void* key, size_t ksize, const std::any& val) {
    if (!parent) {
        throw std::invalid_argument("cannot create context from nil parent");
    }
    if (key == nullptr) {
        throw std::invalid_argument("nil key");
    }
    // if (!(goincpp::reflect::is_comparable< (key.type()) >::value) ) {
    //     throw std::invalid_argument("key is not comparable");
    // }
    return std::make_shared<ValueCtx>(parent, key, ksize, val);
}

static std::optional<std::any>
value(std::shared_ptr<Context> c, const void* key) {
    while (c) {
        if (auto ctx = dynamic_cast<ValueCtx*>(c.get())) {
            if (key == ctx->key()) {
                return ctx->value();
            }
            c = ctx->parent();
        } else if (auto ctx = dynamic_cast<CancelCtx*>(c.get())) {
            if (key == (void*)&CancelCtx::cancelCtxKey) {
                return c;
            }
            c = ctx->parent();
        } else if (auto ctx = dynamic_cast<WithoutCancelCtx*>(c.get())) {
            if (key == (void*)&CancelCtx::cancelCtxKey) {
                // This implements Cause(ctx) == nil
				// when ctx is created using WithoutCancel.
                return std::nullopt;
            }
            c = ctx->parent();
        } else if (auto ctx = dynamic_cast<TimerCtx*>(c.get())) {
            if (key == (void*)&CancelCtx::cancelCtxKey) {
                return c;
            }
            c = ctx->parent();
        } else if (dynamic_cast<BackgroundCtx*>(c.get()) ||
                   dynamic_cast<TodoCtx*>(c.get())) {
            return std::nullopt;
        } else {
            return c->value(key);
        }
    }

    return std::nullopt;
}

}
}