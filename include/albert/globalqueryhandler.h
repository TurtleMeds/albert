// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <albert/rankitem.h>
#include <albert/triggerqueryhandler.h>
#include <memory>
#include <vector>

namespace albert
{

///
/// Abstract global query handler.
///
/// A functional query handler returning scored items. Applicable for the
/// global search. Use this if you want your results show up in the global
/// search. Implements TriggeredQueryHandler.
///
/// @note Do _not_ use this for long running tasks!
///
class ALBERT_EXPORT GlobalQueryHandler : public albert::TriggerQueryHandler
{
public:
    /// The query handling function.
    /// The match score should make sense and often (if not always) be the
    /// fraction matched chars (legth of query string / length of item title).
    /// @return A list of match items. Empty query should return all items with
    /// a score of 0.
    /// @note Executed in a worker thread.
    virtual std::vector<RankItem> handleGlobalQuery(const Query &) = 0;

    /// The empty query handling function.
    /// Empty patterns match everything. For triggered queries this is desired.
    /// For global queries it may quickly result in long running queries on show.
    /// Since a lot of global query handlers relay the handleTriggerQuery to
    /// handleGlobalQuery it is not possible to have both. This function allows
    /// extensions to handle empty global queries differently, while still
    /// yielding all items using the trigger handler.
    virtual std::vector<std::shared_ptr<Item>> handleEmptyQuery(const Query*);

    /// Takes rank items and modifies the score according to the users usage.
    /// Use this if you want to reuse your global results in the trigger handler.
    void applyUsageScore(std::vector<RankItem>*) const;

    /// Implements pure virtual handleTriggerQuery(…).
    /// Calls handleGlobalQuery, applyUsageScore, sort and adds the items.
    /// @note Reimplement if the handler should have custom triggered behavior,
    /// but think twice if this is necessary. It may break user expectation.
    /// @see handleTriggerQuery and rankItems
    void handleTriggerQuery(Query &) override;

protected:

    ~GlobalQueryHandler() override;

};

}
