// Copyright (c) 2023-2024 Manuel Schneider

#include "globalqueryhandler.h"
#include "usagedatabase.h"
#include <ranges>
using namespace albert;
using namespace std;

GlobalQueryHandler::~GlobalQueryHandler() = default;

void GlobalQueryHandler::applyUsageScore(std::vector<RankItem> &items) const
{
    UsageHistory::applyScores(id(), items);
}

void GlobalQueryHandler::handleTriggerQuery(TriggerQuery &query)
{
    struct State : public QueryState {
        vector<RankItem> rank_items;
    } *s = query.state<State>();

    if (!s)
    {
        s = query.setState(make_unique<State>());
        s->rank_items = handleGlobalQuery(query);
        applyUsageScore(s->rank_items);
    }

    // See also GlobalQueryExecution::fetchMore

    // Partial sort the items incrementally in reverse order (for cheap "pop_n")
    const uint max_chunk_size = 10;
    auto reverse_view = s->rank_items | views::reverse;
    auto chunk_view = reverse_view | views::take(max_chunk_size);
    ranges::partial_sort(reverse_view, chunk_view.end(), greater());

    // Add the chunk
    auto item_view = chunk_view | views::transform(&RankItem::item);
    query.add({make_move_iterator(begin(item_view)), make_move_iterator(end(item_view))});

    // Cheap pop_n
    s->rank_items.erase(s->rank_items.end() - chunk_view.size(), s->rank_items.end());

    if (!s->rank_items.empty())
        query.setCanFetchMore();
}

vector<shared_ptr<Item>> GlobalQueryHandler::handleDisabledEmptyGlobalQuery() { return {}; }
