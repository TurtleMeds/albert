// Copyright (c) 2023-2024 Manuel Schneider

#include "logging.h"
#include "query.h"
#include "queryengine.h"
#include "usagedatabase.h"
#include <QtConcurrent>
#include <ranges>
using namespace albert;
using namespace std::chrono;
using namespace std;

Q_LOGGING_CATEGORY(timeCat, "albert.query_runtimes")

namespace
{
static uint query_count = 0;
struct ResultRankItem
{
    albert::Extension *extension;
    albert::RankItem rank_item;
    bool operator<(const ResultRankItem &o) const { return rank_item < o.rank_item; }
    bool operator>(const ResultRankItem &o) const { return rank_item > o.rank_item; }
};
}

QueryContext::~QueryContext() = default;


class Query::Private
{
public:
    Query &q;
    QueryEngine &query_engine_;
    const uint query_id;
    const QString trigger_;
    const QString string_;
    TriggerQueryHandler &query_handler_;
    const vector<FallbackHandler*> fallback_handlers_;
    ItemsModel matches_;
    ItemsModel fallbacks_;
    bool valid_;
    bool finished_;
    QFutureWatcher<void> future_watcher_;
    unique_ptr<QueryContext> context_;
    vector<ResultItem> results_buffer_;
    mutex results_buffer_mutex_;


    void run()
    {
        future_watcher_.setFuture(QtConcurrent::run([this]{
            try {
                runFallbackHandlers();
                auto tp = system_clock::now();
                query_handler_.handleTriggerQuery(this);
                qCDebug(timeCat,).noquote()
                    << QStringLiteral("\x1b[38;5;33m│%1 ms│ TRIGGER |%2│ #%3  '%4' '%5' \x1b[0m")
                           .arg(duration_cast<milliseconds>(system_clock::now() - tp).count(), 6)
                           .arg(matches_.rowCount(), 6)
                           .arg(query_id)
                           .arg(trigger_, string_);
            }
            catch (const exception &e) {
                WARN << QString("TriggerQueryHandler '%1' threw exception:\n")
                        .arg(query_handler_.id()) << e.what();
            }
            catch (...){
                CRIT << "Unexpected exception in QueryExecution::run()!";
            }
        }));
    }

    void cancel() { valid_ = false; }

    void runFallbackHandlers()
    {
        if (trigger_.isEmpty() && string_.isEmpty())
            return;

        const auto &o = query_engine_.fallbackOrder();

        vector<ResultRankItem> fallbacks;
        for (auto *handler : fallback_handlers_)
            for (auto item : handler->fallbacks(QString("%1%2").arg(trigger_, string_)))
                if (auto it = o.find(make_pair(handler->id(), item->id())); it == o.end())
                    fallbacks.emplace_back(handler, RankItem(::move(item), 0));
                else
                    fallbacks.emplace_back(handler, RankItem(::move(item), it->second));

        ranges::sort(fallbacks, greater());

        fallbacks_.add(fallbacks | views::transform([](ResultRankItem &fb) {
                           return ResultItem{fb.extension, fb.rank_item.item};
                       }));
    }

    void invokeCollectResults()
    {
        QMetaObject::invokeMethod(this, &Query::Private::collectResults, Qt::QueuedConnection);
    }

    void collectResults()
    {
        // Rationale:
        // Queued signals from other threads may fire multple times which
        // messes up the frontend state machines. So we collect the results in
        // the main thread using a buffer.

        lock_guard lock(results_buffer_mutex_);
        if (!results_buffer_.empty())
        {
            matches_.add(results_buffer_);
            results_buffer_.clear();
        }
    }
};


Query::Query(QueryEngine &e,
             vector<FallbackHandler *> &&fallback_handlers,
             TriggerQueryHandler &query_handler,
             const QString &string,
             const QString &trigger):
    d(new Private{
        .q = *this,
        .query_engine_ = e,
        .query_id = query_count++,
        .trigger_  = ::move(trigger),
        .string_  = ::move(string),
        .query_handler_  = query_handler,
        .fallback_handlers_  = ::move(fallback_handlers),
        .matches_  = this,  // Important for qml ownership determination
        .fallbacks_  = this,  // Important for qml ownership determination
        .valid_  = true,
        .finished_  = false,
        .future_watcher_{},
        .context_{},
        .results_buffer_{},
        .results_buffer_mutex_{}
    })
{
    connect(&d->future_watcher_, &decltype(d->future_watcher_)::finished,
            this, &Query::finished);
}

Query::~Query()
{
    d->cancel();

    if (!isFinished())
    {
        WARN << QString("Busy wait on query: #%1").arg(d->query_id);
        // there may be some queued collectResults calls
        QCoreApplication::processEvents();
        d->future_watcher_.waitForFinished();
    }

    DEBG << QString("Query deleted. [#%1 '%2']").arg(d->query_id).arg(string());
}

void Query::preAdd()
{

}

void Query::postAdd()
{

}

void Query::collectResults() { d->collectResults(); }

QString Query::synopsis() const { return d->query_handler_.synopsis(); }

QString Query::trigger() const { return d->trigger_; }

QString Query::string() const { return d->string_; }

const bool &Query::isValid() const { return d->valid_; }

bool Query::isFinished() const { return d->future_watcher_.isFinished(); }

bool Query::isTriggered() const { return !d->trigger_.isEmpty(); }

QAbstractListModel *Query::matches() { return &d->matches_; }

QAbstractListModel *Query::fallbacks()  { return &d->fallbacks_; }

QueryContext *Query::context() { return d->context_.get(); }

void Query::setContext(unique_ptr<QueryContext> context) { d->context_ = ::move(context); }

static void activate(const vector<ResultItem> result_items, Query *q, uint iidx, uint aidx)
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
        }
        catch (const out_of_range&) {
            WARN << "Activated action index is invalid:" << aidx;
        }
    } catch (const out_of_range&) {
        WARN << "Activated item index is invalid:" << iidx;
    }
}

void Query::activateMatch(uint i, uint a)
{ activate(d->matches_.items, this, i, a); }

void Query::activateFallback(uint i, uint a)
{ activate(d->fallbacks_.items, this, i, a); }



// void Query::add(const shared_ptr<Item> &item)
// {
//     unique_lock lock(results_buffer_mutex_);

//     results_buffer_.emplace_back(query_handler_, item);

//     if (valid_)
//         invokeCollectResults();
// }

// void Query::add(shared_ptr<Item> &&item)
// {
//     unique_lock lock(results_buffer_mutex_);

//     results_buffer_.emplace_back(query_handler_, ::move(item));

//     if (valid_)
//         invokeCollectResults();
// }

// void Query::add(const vector<shared_ptr<Item>> &items)
// {
//     unique_lock lock(results_buffer_mutex_);

//     for (const auto &item : items)
//         results_buffer_.emplace_back(query_handler_, item);

//     if (valid_)
//         invokeCollectResults();
// }

// void Query::add(vector<shared_ptr<Item>> &&items)
// {
//     unique_lock lock(results_buffer_mutex_);

//     for (auto &item : items)
//         results_buffer_.emplace_back(query_handler_, ::move(item));

//     if (valid_)
//         invokeCollectResults();
// }






















































































