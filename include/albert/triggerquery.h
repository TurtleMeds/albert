// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <albert/export.h>
#include <albert/query.h>
#include <memory>
#include <vector>

namespace albert
{
class Item;

///
/// The abstract base class for query state data.
///
/// Subclass to store query state over iterative runs using state() and setState().
///
class ALBERT_EXPORT QueryState
{
public:
    virtual ~QueryState();
};

///
/// The trigger query execution interface class.
///
/// Since the GlobalQuery interface is a true subset of the TriggerQuery interface and
/// implementations of GlobalQueryHandler may be interested in relaying the TriggerQuery to
/// GlobalQueryHandler::handleGlobalQuery() the TriggerQuery inherits the GlobalQuery.
///
class ALBERT_EXPORT TriggerQuery : public Query
{
public:

    ///
    /// Adds `item` to the query results.
    ///
    /// Use batch add methods if you can to avoid expensive locking and UI flicker.
    ///
    virtual void add(const std::shared_ptr<Item> &item) = 0;

    ///
    /// Adds `item` to the query results using move semantics.
    ///
    /// @copydetails add(const std::shared_ptr<Item> &item)
    ///
    virtual void add(std::shared_ptr<Item> &&item) = 0;

    ///
    /// Adds `items` to the query results.
    ///
    virtual void add(const std::vector<std::shared_ptr<Item>> &items) = 0;

    ///
    /// Adds `items` to the query results using move semantics.
    ///
    virtual void add(std::vector<std::shared_ptr<Item>> &&items) = 0;

    ///
    /// Sets the QueryExecution::canFetchMore() flag of the query to `true`.
    ///
    virtual void setCanFetchMore() = 0;

    ///
    /// Returns a weak pointer to the query state.
    ///
    virtual QueryState *state() const = 0;

    ///
    /// Returns a weak pointer to the query state implicitly casted to the requested type `T`.
    /// @note Performs a static_cast. The types you set and get have to be the same.
    ///
    template<typename T>
    inline T *state() const
    {
        return static_cast<T *>(state());
    }

    ///
    /// Sets the query state to `state` and returns a weak pointer to it.
    ///
    virtual QueryState *setState(std::unique_ptr<QueryState> state) = 0;

    ///
    /// Sets the query state to `state` and returns a weak pointer of type `T` to it.
    ///
    template<typename T>
    inline T *setState(std::unique_ptr<T> state)
    {
        return static_cast<T *>(setState(std::unique_ptr<QueryState>(std::move(state))));
    }
};

}  // namespace albert
