#pragma once

#include "DirectorDesk/Core/Error.h"

#include <cassert>
#include <utility>

namespace DirectorDesk::Core {

template<typename T>
class Result {
public:
    static Result Ok(T value) {
        Result result;
        result.m_hasValue = true;
        result.m_value = std::move(value);
        return result;
    }

    static Result Fail(Error error) {
        Result result;
        result.m_hasValue = false;
        result.m_error = std::move(error);
        return result;
    }

    [[nodiscard]] bool IsOk() const { return m_hasValue; }
    explicit operator bool() const { return m_hasValue; }

    [[nodiscard]] const T& Value() const {
        assert(m_hasValue && "Result::Value called on failure");
        return m_value;
    }

    [[nodiscard]] T& Value() {
        assert(m_hasValue && "Result::Value called on failure");
        return m_value;
    }

    [[nodiscard]] const Error& GetError() const {
        assert(!m_hasValue && "Result::GetError called on success");
        return m_error;
    }

private:
    bool m_hasValue = false;
    T m_value{};
    Error m_error{};
};

template<>
class Result<void> {
public:
    static Result Ok() {
        Result result;
        result.m_hasValue = true;
        return result;
    }

    static Result Fail(Error error) {
        Result result;
        result.m_hasValue = false;
        result.m_error = std::move(error);
        return result;
    }

    [[nodiscard]] bool IsOk() const { return m_hasValue; }
    explicit operator bool() const { return m_hasValue; }

    [[nodiscard]] const Error& GetError() const {
        assert(!m_hasValue && "Result::GetError called on success");
        return m_error;
    }

private:
    bool m_hasValue = false;
    Error m_error{};
};

} // namespace DirectorDesk::Core
