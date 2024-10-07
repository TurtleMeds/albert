// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <albert/rankitem.h>
#include <albert/triggerqueryhandler.h>
#include <vector>

namespace albert
{
class RankItem;

///
/// Abstract global query handler extension.
///
/// Extensions of this type are used by the global query execution to provide results for global,
/// untriggered queries. Note that this class implements TriggerQueryHandler.
///
/// @note Do **not** use this for long running tasks. Global query handlers should return fast.
///
/// @note handleTriggerQuery(), handleGlobalQuery() and specialItems() are executed in threads. Keep
/// thread safety in mind!
///
class ALBERT_EXPORT GlobalQueryHandler : public albert::TriggerQueryHandler
{
public:
    ///
    /// Modifies the score of `items` according to the users usage in place.
    ///
    void applyUsageScore(std::vector<RankItem> &items) const;

    ///
    /// Adds the scored and sorted results of handleGlobalQuery() to `query`.
    ///
    /// If you plan to reimplement this function to get custom behavior for triggered queries, do
    /// not forget to document this in the settings widget (According to the principle of least
    /// surprise).
    ///
    void handleTriggerQuery(TriggerQuery &query) override;

    ///
    /// Returns items that match `query`.
    ///
    /// The empty string matches any string. Implementations of handleGlobalQuery() should therefore
    /// return all available items on empty queries. However the user may choose to alter this
    /// behavior such that the empty global query is not executed. Despite this configuration there
    /// may be items of interest for the users. Such items should be returned by the
    /// handleDisabledEmptyGlobalQuery() function.
    ///
    virtual std::vector<RankItem> handleGlobalQuery(const Query &query) = 0;

    ///
    /// Returns items that should appear despite disabled empty query execution.
    ///
    /// The base class implementation returns an empty list.
    ///
    virtual std::vector<std::shared_ptr<Item>> handleDisabledEmptyGlobalQuery();

protected:
    ///
    /// Destructs the global query handler.
    ///
    ~GlobalQueryHandler() override;
};

}  // namespace albert
