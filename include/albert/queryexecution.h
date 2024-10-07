// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <QObject>
#include <QString>
#include <albert/export.h>
#include <vector>


namespace albert
{

class Item;
class Extension;


class ALBERT_EXPORT ResultItem
{
public:
    albert::Extension *extension;  ///< The extension that provided `item`.
    std::shared_ptr<albert::Item> item;  ///< The item.
};


///
/// Common query object.
///
class ALBERT_EXPORT QueryExecution : public QObject
{
    Q_OBJECT

public:
    /// Returns the synopsis of this query.
    [[nodiscard]] Q_INVOKABLE virtual QString synopsis() const = 0;

    /// Returns the trigger of this query.
    [[nodiscard]] Q_INVOKABLE virtual QString trigger() const = 0;

    /// Returns the query string _excluding_ the trigger.
    [[nodiscard]] Q_INVOKABLE virtual QString string() const = 0;

    /// Returns `true` if the query has been cancelled or invalidated.
    [[nodiscard]] Q_INVOKABLE virtual bool isValid() const = 0;

    /// Returns `true` if this query is currently processing.
    [[nodiscard]] Q_INVOKABLE virtual bool isProcessing() const = 0;

    /// Returns `true` if this query can fetch more results.
    [[nodiscard]] Q_INVOKABLE virtual bool canFetchMore() const = 0;

    /// Fetches more results if canFetchMore() is `true`.
    Q_INVOKABLE virtual void fetchMore() = 0;

    /// Cancels processing and invalidates query.
    Q_INVOKABLE virtual void cancel() = 0;

    /// Returns the matches.
    [[nodiscard]] Q_INVOKABLE virtual const std::vector<ResultItem> &matches() const = 0;

    /// Returns the fallbacks.
    [[nodiscard]] Q_INVOKABLE virtual const std::vector<ResultItem> &fallbacks() const = 0;

    /// Activates a match action.
    [[nodiscard]] Q_INVOKABLE virtual bool activateMatch(uint item, uint action = {}) = 0;

    /// Activates a fallback action.
    [[nodiscard]] Q_INVOKABLE virtual bool activateFallback(uint item, uint action = {}) = 0;

signals:

    /// This signal is emitted before `count` matches are added to the matches().
    void matchesAboutToBeAdded(uint count);

    /// This signal is emitted after matches have been added to the matches().
    void matchesAdded();

    /// This signal is emitted when the query has been invalidated.
    void invalidated();

    /// This signal is emitted when the query handler started/finished processing the query.
    void stateChanged(bool processing);
};

}


