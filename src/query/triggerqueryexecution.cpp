// Copyright (c) 2022-2024 Manuel Schneider

#include "logging.h"
#include "queryengine.h"
#include "triggerqueryexecution.h"
#include "triggerqueryhandler.h"
#include "usagedatabase.h"
#include <QtConcurrent>
using namespace albert;
using namespace std::chrono;
using namespace std;
static const uint pad = 8;

uint TriggerQueryExecution::query_count = 0;

TriggerQueryExecution::TriggerQueryExecution(std::vector<albert::ResultItem> fallbacks,
                                             albert::TriggerQueryHandler &handler,
                                             const QString &trigger,
                                             const QString &string) :
    query_id(query_count++),
    handler_(handler),
    synopsis_(handler.synopsis(string)),
    trigger_(trigger),
    string_(string),
    valid_(true),
    can_fetch_more_(true),
    fallbacks_(::move(fallbacks))
{
    connect(&future_watcher_, &decltype(future_watcher_)::finished,
            this, [this]{ emit stateChanged(false); });

    connect(&future_watcher_, &decltype(future_watcher_)::started,
            this, [this]{ emit stateChanged(true); });
}

TriggerQueryExecution::~TriggerQueryExecution()
{
    if (future_watcher_.isRunning())
    {
        WARN << QString("Busy wait on query: #%1").arg(query_id);
        invalidate();
    }
    DEBG << QString("Query deleted. [#%1 '%2']").arg(query_id).arg(string_);
}

QString TriggerQueryExecution::synopsis() const { return synopsis_; }

QString TriggerQueryExecution::trigger() const { return trigger_; }

QString TriggerQueryExecution::string() const { return string_; }

bool TriggerQueryExecution::isValid() const { return valid_; }

bool TriggerQueryExecution::isProcessing() const { return future_watcher_.isRunning(); }

bool TriggerQueryExecution::canFetchMore() const
{ return isValid() && !isProcessing() && can_fetch_more_; }

void TriggerQueryExecution::fetchMore()
{
    if (!canFetchMore())
        return;

    can_fetch_more_ = false;

    auto map = [this] {
        try
        {
            auto t = system_clock::now();
            handler_.handleTriggerQuery(*this);
            DEBG << QStringLiteral(cblue " %L1 ms│%L2 pc│ FETCH [%6] #%3 '%4'>'%5' ")
                        .arg(duration_cast<milliseconds>(system_clock::now() - t).count(), pad)
                        .arg(matches_.size()+results_buffer_.size(), pad)
                        .arg(query_id)
                        .arg(trigger_, string_, handler_.id());
        }
        catch (const exception &e)
        {
            WARN << QString("QueryHandler '%1' threw exception:\n").arg(handler_.id())
                 << e.what();
        }
        catch (...)
        {
            CRIT << QString("QueryHandler '%1' threw unknown exception.").arg(handler_.id());
        }
    };

    future_watcher_.setFuture(QtConcurrent::run(map));
}

void TriggerQueryExecution::cancel() { valid_ = false; }

void TriggerQueryExecution::invalidate()
{
    valid_ = false;  // cancel
    future_watcher_.disconnect();
    future_watcher_.waitForFinished();
    matches_.clear();
    fallbacks_.clear();
    results_buffer_.clear();
}

void TriggerQueryExecution::setCanFetchMore() { can_fetch_more_ = true; }

QueryState *TriggerQueryExecution::state() const { return state_.get(); }

QueryState *TriggerQueryExecution::setState(unique_ptr<QueryState> state)
{ return (state_ = ::move(state)).get(); }

void TriggerQueryExecution::add(const shared_ptr<Item> &item)
{
    if (valid_)
    {
        lock_guard lock(results_buffer_mutex_);
        results_buffer_.emplace_back(&handler_, item);
        invokeCollectResults();
    }
}

void TriggerQueryExecution::add(shared_ptr<Item> &&item)
{
    if (valid_)
    {
        lock_guard lock(results_buffer_mutex_);
        results_buffer_.emplace_back(&handler_, ::move(item));
        invokeCollectResults();
    }
}

void TriggerQueryExecution::add(const vector<shared_ptr<Item>> &items)
{
    if (valid_)
    {
        lock_guard lock(results_buffer_mutex_);
        results_buffer_.reserve(results_buffer_.size() + items.size());
        for (const auto &item : items)
            results_buffer_.emplace_back(&handler_, item);
        invokeCollectResults();
    }
}

void TriggerQueryExecution::add(vector<shared_ptr<Item>> &&items)
{
    if (valid_)
    {
        lock_guard lock(results_buffer_mutex_);
        results_buffer_.reserve(results_buffer_.size() + items.size());
        for (const auto &item : items)
            results_buffer_.emplace_back(&handler_, ::move(item));
        invokeCollectResults();
    }
}

const vector<ResultItem> &TriggerQueryExecution::matches() const { return matches_; }

const vector<ResultItem> &TriggerQueryExecution::fallbacks() const { return fallbacks_; }

static bool activate(const vector<ResultItem> result_items, QueryExecution *q, uint iidx, uint aidx)
{
    try {
        auto &[e, i] = result_items.at(iidx);
        try {
            auto a = i->actions().at(aidx);

            INFO << QString("Activating action %1>%2>%3 (%4>%5>%6) ")
                    .arg(e->id(), i->id(), a.id, e->name(), i->text(), a.text);

            // Order is cumbersome here
            UsageHistory::addActivation(q->string(), e->id(), i->id(), a.id);

            // May delete the query, due to hide()
            // Notes to self:
            // - QTimer::singleShot(0, this, [a]{ a.function(); });
            //   Disconnects on query deletion.

            a.function(); // May delete the query, due to hide()
            return true;
        }
        catch (const out_of_range&) {
            WARN << "Activated action index is invalid:" << aidx;
        }
    } catch (const out_of_range&) {
        WARN << "Activated item index is invalid:" << iidx;
    }
    return false;
}

bool TriggerQueryExecution::activateMatch(uint i, uint a) { return activate(matches_, this, i, a); }

bool TriggerQueryExecution::activateFallback(uint i, uint a) { return activate(fallbacks_, this, i, a); }

void TriggerQueryExecution::invokeCollectResults()
{
    QMetaObject::invokeMethod(this, &TriggerQueryExecution::collectResults, Qt::AutoConnection);
}

void TriggerQueryExecution::collectResults()
{
    if (!valid_)
        return;

    // Rationale:
    // Queued signals from other threads may fire multple times which
    // messes up the frontend state machines. So we collect the results in
    // the main thread using a buffer.

    lock_guard lock(results_buffer_mutex_);

    if (!results_buffer_.empty())
    {
        emit matchesAboutToBeAdded(results_buffer_.size());

        matches_.insert(matches_.end(),
                        make_move_iterator(results_buffer_.begin()),
                        make_move_iterator(results_buffer_.end()));

        emit matchesAdded();

        results_buffer_.clear();
    }
}
