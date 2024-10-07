// Copyright (c) 2022-2024 Manuel Schneider

#include "globalqueryexecution.h"
#include "globalqueryhandler.h"
#include "logging.h"
#include <QtConcurrent>
#include <ranges>
using namespace albert;
using namespace std::chrono;
using namespace std;
static const uint pad = 8;

GlobalQueryExecution::GlobalQueryExecution(std::vector<albert::ResultItem> fallbacks,
                                           std::vector<albert::GlobalQueryHandler *> handlers,
                                           const QString &string) :
    TriggerQueryExecution(::move(fallbacks), *this, {},  string),
    handlers_(::move(handlers))
{}

QString GlobalQueryExecution::id() const { return QStringLiteral("globalsearch"); }

QString GlobalQueryExecution::name() const { return {}; }  // never used

QString GlobalQueryExecution::description() const { return {}; }  // never used

void GlobalQueryExecution::handleTriggerQuery(TriggerQuery&)
{
    struct State : public QueryState
    {
        vector<pair<Extension *, RankItem>> result_rank_items;
    };

    auto *s = static_cast<State*>(state_.get());

    if (!s)
    {
        state_ = make_unique<State>();
        s = static_cast<State*>(state_.get());

        mutex rank_items_mutex;  // 6.4 Still no move semantics in QtConcurrent

        struct Diagnositcs
        {
            uint runtime = 0;
            uint scoring = 0;
            uint count = 0;
            optional<std::string> exception;
        };

        function<Diagnositcs(GlobalQueryHandler*)> map
            = [&, this](GlobalQueryHandler *handler) -> Diagnositcs
        {
            // blocking map is not interruptible. end cancelled runs fast.
            if (!isValid())
                return { .exception = "Cancelled." };

            try {
                Diagnositcs a;

                auto t = system_clock::now();
                vector<RankItem> results;
                if (string_.isEmpty())
                    for (auto &item : handler->handleDisabledEmptyGlobalQuery())
                        results.emplace_back(::move(item), 0);
                else
                    results = handler->handleGlobalQuery(*this);

                a.runtime = duration_cast<microseconds>(system_clock::now() - t).count();

                t = system_clock::now();
                handler->applyUsageScore(results);
                a.scoring = duration_cast<microseconds>(system_clock::now() - t).count();

                a.count = results.size();

                // makes no sense to time this, biased due to lock
                {
                    lock_guard<mutex> lock(rank_items_mutex);
                    s->result_rank_items.reserve(s->result_rank_items.size() + results.size());
                    for (auto &rank_item : results)
                        s->result_rank_items.emplace_back(handler, ::move(rank_item));
                }

                return a;
            }
            catch (const exception &e) {
                return { .exception = e.what() };
            }
            catch (...) {
                return { .exception = "Unknown exception." };
            }
        };

        auto t = system_clock::now();
        auto r = QtConcurrent::blockingMapped(handlers_, map);
        auto d_h = duration_cast<milliseconds>(system_clock::now() - t).count();

        // Print diagnostics
        DEBG << QStringLiteral(cblue "┬─ Handling ┬── Scoring ┬──── Count ┬ GLOBAL QUERY #%1 '%2'")
                    .arg(query_id)
                    .arg(string_);

        for (uint i = 0; i < handlers_.size(); ++i)
            if (r[i].exception.has_value())
                DEBG << QStringLiteral(cred "│%L1 µs│%L2 µs│%L3 pc│ %4 : %5")
                            .arg(r[i].runtime, pad)
                            .arg(r[i].scoring, pad)
                            .arg(r[i].count, pad)
                            .arg(handlers_[i]->id(), QString::fromStdString(*r[i].exception));
            else
                DEBG << QStringLiteral(cblue "│%L1 µs│%L2 µs│%L3 pc│ %4")
                            .arg(r[i].runtime, pad)
                            .arg(r[i].scoring, pad)
                            .arg(r[i].count, pad)
                            .arg(handlers_[i]->id());

        DEBG << QStringLiteral(cblue "╰%L1 ms│           │%L2 pc│ TOTAL")
                    .arg(d_h, pad)
                    .arg(s->result_rank_items.size(), pad);
    }

    // See also GlobalQueryHandler::handleTriggerQuery

    // Partial sort the items incrementally in reverse order (for cheap "pop_n")
    const uint max_chunk_size = 10;
    auto reverse_view = s->result_rank_items | views::reverse;
    auto chunk_view = reverse_view | views::take(max_chunk_size);
    ranges::partial_sort(reverse_view,
                         chunk_view.end(),
                         greater{},
                         &pair<Extension *, RankItem>::second);

    // // Debug sections for score problems
    // for (const auto &[e, ri] : chunk_view)
    //     DEBG << QStringLiteral(cmagenta"SCORE: %1 %2").arg(ri.score, 4, 'g', 2, '0' ).arg(ri.item->id());

    // Add the chunk
    {
        lock_guard lock(results_buffer_mutex_);
        results_buffer_.reserve(results_buffer_.size() + chunk_view.size());
        for (auto &[extension, rank_item] : chunk_view)
            results_buffer_.emplace_back(extension, ::move(rank_item.item));
    }

    invokeCollectResults();

    // Cheap pop_n
    s->result_rank_items.resize(s->result_rank_items.size() - chunk_view.size());

    if (!s->result_rank_items.empty())
        setCanFetchMore();
}
