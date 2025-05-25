// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <QRegularExpression>
#include <QString>
#include <albert/export.h>
#include <albert/matchconfig.h>
#include <ranges>
class MatcherPrivate;

namespace albert::util
{

///
/// Augmented match score.
///
/// Some nifty features:
/// - The bool type conversion evaluates to \ref isMatch()
/// - The Score/double type conversion seamlessly uses the \ref score()
///
/// @sa \ref Matcher
///
class ALBERT_EXPORT Match final
{
public:
    using Score = double;

    ///
    /// Constructs an invalid match.
    ///
    Match() : score_(-1.) {}

    ///
    /// Constructs a match with the given `score`.
    ///
    Match(const Score score) : score_(score) {}

    ///
    /// Constructs a #Match with the score of `other`.
    ///
    Match(const Match &o) = default;

    ///
    /// Replaces the score with that of `other`.
    ///
    Match &operator=(const Match &o) = default;

    ///
    /// Returns `true` if this is a match, otherwise returns `false`.
    ///
    inline bool isMatch() const { return score_ >= 0.0; }

    ///
    /// Returns `true` if this is a zero score match, otherwise returns `false`.
    ///
    inline bool isEmptyMatch() const { return qFuzzyCompare(score_, 0.0); }

    ///
    /// Returns `true` if this is a perfect match, otherwise returns `false`.
    ///
    inline bool isExactMatch() const { return qFuzzyCompare(score_, 1.0); }

    ///
    /// Returns the score.
    ///
    inline Score score() const { return score_; }

    ///
    /// Returns `true` if this is a match, otherwise returns `false`.
    ///
    inline explicit operator bool() const { return isMatch(); }

    ///
    /// Returns the score.
    ///
    inline operator Score() const { return score_; }

private:

    Score score_;
};


///
/// Configurable string matcher.
///
/// @sa \ref MatchConfig, \ref Match
///
class ALBERT_EXPORT Matcher final
{
public:
    ///
    /// Constructs a Matcher with the given `string` and match `config`.
    ///
    /// If `config` is not provided, a default constructed config is used.
    ///
    Matcher(const QString &string, MatchConfig config = {});

    ///
    /// Constructs a Matcher with the contents of `other` using move semantics.
    ///
    Matcher(Matcher &&o);

    ///
    /// Replaces the contents with those of `other` using move semantics.
    ///
    Matcher &operator=(Matcher &&o);

    ///
    /// Destructs the Matcher.
    ///
    ~Matcher();

    ///
    /// Returns the string matched against.
    ///
    const QString &string() const;

    ///
    /// Returns a \ref Match for `string`.
    ///
    Match match(const QString &string) const;

    ///
    /// Returns the max \ref Match for the given strings.
    ///
    Match match(QString first, auto... args) const
    { return std::max(match(first), match(args...)); }

    ///
    /// Returns the max \ref Match in the range of `strings`.
    ///
    Match match(std::ranges::range auto &&strings) const
         requires std::same_as<std::ranges::range_value_t<decltype(strings)>, QString>
    {
        if (strings.empty())
            return Match();
        return std::ranges::max(
            strings | std::views::transform([this](const QString &s) { return this->match(s); }));
    }


private:
    class Private;
    std::unique_ptr<Private> d;

};

}
