// SPDX-FileCopyrightText: 2024 Manuel Schneider

#pragma once
#include <QDebug>
#include <QString>
#include <chrono>
#include <albert/logging.h>


// Private API

struct TimeIt
{
    QString name;
    std::chrono::system_clock::time_point start;

    [[nodiscard]] TimeIt(const QString &name = {}):
        name(name),
        start(std::chrono::system_clock::now())
    {}

    ~TimeIt()
    {
        auto end = std::chrono::system_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        CRIT << QString(ccyan "%L1 µs | %2").arg(dur, 8).arg(name);
    }
};


// TODO remove with c++23
template<class T, class U>
constexpr auto&& forward_like(U&& x) noexcept
{
    constexpr bool is_adding_const = std::is_const_v<std::remove_reference_t<T>>;
    if constexpr (std::is_lvalue_reference_v<T&&>)
    {
        if constexpr (is_adding_const)
            return std::as_const(x);
        else
            return static_cast<U&>(x);
    }
    else
    {
        if constexpr (is_adding_const)
            return std::move(std::as_const(x));
        else
            return std::move(x);
    }
}
