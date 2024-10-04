#pragma once

#include <coroutine>
#include <span>
#include <optional>
#include <type_traits>

namespace coro {

template <typename T>
class awaiter {
public:
    using result_type = std::invoke_result_t<T>;

    bool await_ready() {
        auto attempt = t();
        if (!attempt && attempt.error().retryable())
            return false;
        result = std::move(attempt);
        return true;
    }

    void await_suspend(auto) const noexcept {}

    result_type await_resume() {
        result_type return_val = result ? std::move(result.value()) : t();
        result.reset();
        return return_val;
    }

    awaiter(T t_): t{std::move(t_)} {}

private:
    T t;
    std::optional<result_type> result;
};

}
