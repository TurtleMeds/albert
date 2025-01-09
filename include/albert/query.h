// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <QObject>
#include <QString>
#include <albert/export.h>
#include <albert/util.h>
#include <memory>
#include <ranges>
#include <vector>

class QueryEngine;
namespace albert
{
class Item;
class Extension;
class QueryHandler;
class Query;
class QueryExecution;

class ALBERT_EXPORT ResultItem
{
public:
    const Extension &extension;
    std::shared_ptr<Item> item;
};

///
/// Common query object.
///
class ALBERT_EXPORT Query : public QObject
{
    Q_OBJECT

public:

    const uint id;

    /// The trigger that dispatched this handler.
    const QString trigger;

    /// Query string _excluding_ the trigger.
    const QString string;

    const QString synopsis;

    ~Query() override;

    /// Type conversion to QString
    /// Syntactic sugar for context conversions
    operator QString() const;

    bool triggered() const;

    bool global() const;

    const bool &valid() const;

    bool active() const;

    bool canFetchMore() const;

    void fetchMore();

    void cancel();

    /// Returns the matches.
    const std::vector<ResultItem> &matches() const;

    /// Returns the fallbacks.
    const std::vector<ResultItem> &fallbacks() const;

    /// Executes a match action.
    bool activateMatch(uint item, uint action = {});

    /// Executes a fallback action.
    bool activateFallback(uint item, uint action = {});

    ///
    /// Add `item` to the results.
    /// Use range add if you can to avoid UI flicker.
    ///
    void add(auto &&item)
    {
        if (!valid())
            return;

        using T = std::decay_t<decltype(item)>;
        if constexpr (auto l = getLock(); std::same_as<T, ResultItem>)
            buffer().emplace_back(std::forward<decltype(item)>(item));
        // Otherwise assume shared_ptr<Item> or derived
        // else if constexpr (std::is_base_of_v<Item, typename T::element_type>)
        //     buffer().emplace_back(handler, std::forward<decltype(item)>(item));
        else
            static_assert(false, "Query::add: value type not supported.");

        invokeCollectResults();
    }

    ///
    /// Add `items` to the results.
    ///
    void add(std::ranges::range auto &&items)
    {
        if (!valid() || items.empty())
            return;

        {
            auto l = getLock();

            auto &b = buffer();
            b.reserve(b.size() + items.size());

            using T = std::ranges::range_value_t<decltype(items)>;
            for (auto&& item : items)
                if constexpr (std::same_as<T, ResultItem>)
                    b.emplace_back(forward_like<decltype(items)>(item));
                // Otherwise assume shared_ptr<Item> or derived
                // else if constexpr (std::is_base_of_v<Item, typename T::element_type>)
                //     b.emplace_back(handler, forward_like<decltype(items)>(item));
                else
                    static_assert(false, "Query::add: value type not supported.");
        }

        invokeCollectResults();
    }


signals:

    /// Emitted before `count` matches are added to the matches.
    void matchesAboutToBeAdded(uint count);

    /// Emitted after matches have been added to the matches.
    void matchesAdded();

    /// Emitted when the query has been invalidated.
    void invalidated();

    /// Emitted when query processing started or finished.
    void activeChanged(bool active);

protected:


private:

    Query(std::unique_ptr<QueryExecution> execution,
          std::vector<ResultItem> &&fallbacks,
          const QString &trigger,
          const QString &string,
          const QString &synopsis);

    std::lock_guard<std::mutex> getLock();
    std::vector<ResultItem> &buffer();
    void invokeCollectResults();
    void collectResults();

    class Private;
    std::unique_ptr<Private> d;
    friend class ::QueryEngine;
    friend class QueryHandler;
};
}
