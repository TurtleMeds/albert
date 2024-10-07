// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <QRegularExpression>
#include <QString>
#include <albert/export.h>
#include <albert/matchconfig.h>

namespace albert
{

///
/// The Match class.
///
/// Utility class encapsulating and augmenting the match score.
///
/// Some nifty features:
/// - The bool type conversion evaluates to isMatch()
/// - The Score/double type conversion seamlessly uses the score
///
class ALBERT_EXPORT Match final
{
public:
    using Score = double;

    ///
    /// Constructs a Match with the given `score`.
    ///
    Match(const Score score) noexcept : score_(score) {}

    ///
    /// Constructs a Match with the score of `other`.
    ///
    Match(const Match &other) noexcept = default;

    ///
    /// Replaces the score with that of `other`.
    ///
    Match &operator=(const Match &other) noexcept = default;

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
    /// Converts the match to `bool` using isMatch().
    ///
    inline operator bool() const { return isMatch(); }

    ///
    /// Converts the match to `Score` using score().
    ///
    inline explicit operator Score() const { return score_; }

private:

    Score score_;
};


///
/// A utility class that provides configurable string matching.
///
/// @sa MatchConfig
///
class ALBERT_EXPORT Matcher final
{
public:

    ///
    /// Constructs a Matcher with the given `string` and match `config`.
    ///
    /// If `config` is not provided, a default constructed MatchConfig is used.
    ///
    Matcher(const QString &string, MatchConfig config = {}) noexcept;

    ///
    /// Constructs a Matcher with the contents of `other` using move semantics.
    ///
    Matcher(Matcher &&o) noexcept;

    ///
    /// Replaces the contents with those of `other` using move semantics.
    ///
    Matcher &operator=(Matcher &&o) noexcept;

    ///
    /// Destructs the Matcher.
    ///
    ~Matcher();

    ///
    /// Returns the string matched against.
    ///
    const QString &string() const;

    ///
    /// Returns a Match for the string `string` and the string of this Matcher.
    ///
    Match match(const QString &string) const;

private:

    class Private;
    std::unique_ptr<Private> d;

};

}
