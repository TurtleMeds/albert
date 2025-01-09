// Copyright (c) 2023-2024 Manuel Schneider

#include "globalqueryhandler.h"
#include "query.h"
#include "usagedatabase.h"
#include <ranges>
using namespace albert;
using namespace std;

GlobalQueryHandler::~GlobalQueryHandler() = default;

void GlobalQueryHandler::applyUsageScore(std::vector<RankItem> &rank_items) const
{ UsageHistory::applyScores(id(), rank_items); }

void GlobalQueryHandler::handle(ThreadedQueryExecution &exec)
{
    auto rank_items = handleGlobalQuery(exec.query);
    applyUsageScore(rank_items);
    ranges::sort(rank_items, std::greater());
    exec.query.add(rank_items | views::transform(&RankItem::item));
}

vector<shared_ptr<Item>> GlobalQueryHandler::handleEmptyQuery(const Query &) { return {}; }
