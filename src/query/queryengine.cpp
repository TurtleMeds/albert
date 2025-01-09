// Copyright (c) 2022-2024 Manuel Schneider

#include "albert.h"
#include "session.h"
#include "extensionregistry.h"
#include "fallbackhandler.h"
#include "globalqueryhandler.h"
#include "logging.h"
#include "query.h"
#include "queryengine.h"
#include "queryexecution.h"
#include "triggerqueryhandler.h"
#include "queryhandler.h"
#include "queryhandlerprivate.hpp"
#include "usagedatabase.h"
#include <QCoreApplication>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QMessageBox>
#include <QSettings>
#include <ranges>
using namespace albert;
using namespace std;
static const char *CFG_GLOBAL_HANDLER_ENABLED = "global_handler_enabled";
static const char *CFG_FALLBACK_ORDER = "fallback_order";
static const char *CFG_FALLBACK_EXTENSION = "extension";
static const char *CFG_FALLBACK_ITEM = "fallback";
static const uint pad = 8;


Q_LOGGING_CATEGORY(lcg, "albert.query.global")

QueryEngine::QueryEngine(ExtensionRegistry &registry) : registry_(registry)
{
    UsageHistory::initialize();
    loadFallbackOrder();
    connect(&registry, &ExtensionRegistry::added, this, &QueryEngine::onExtensionAdded);
    connect(&registry, &ExtensionRegistry::removed, this, &QueryEngine::onExtensionRemoved);
}

unique_ptr<Session> QueryEngine::createSession() { return make_unique<Session>(*this); }

unique_ptr<Query> QueryEngine::query(const QString &query_string)
{
    for (const auto &[trigger, handler] : active_triggers_)
        if (query_string.startsWith(trigger))
            return unique_ptr<Query>(new Query(*handler, fallbacks(query_string),
                                               trigger, query_string.mid(trigger.size())));

    // make_unique is not a friend
    return unique_ptr<Query>(new Query(global_query, fallbacks(query_string), {}, query_string));
}

// -------------------------------------------------------------------------------------------------
// Trigger handlers

const map<QString, QueryHandler *> &QueryEngine::queryHandlers() const { return query_handlers_; }

const map<QString, QueryHandler *> &QueryEngine::activeTriggerHandlers() const
{ return active_triggers_; }

void QueryEngine::updateActiveTriggers()
{
    active_triggers_.clear();
    for (const auto&[id, h] : query_handlers_)
        if (const auto&[it, success] = active_triggers_.emplace(h->trigger(), h); !success)
            WARN << QString("Trigger '%1' of '%2' already registered for '%3'.")
                        .arg(h->trigger(), id, it->second->id());
}

void QueryEngine::setTrigger(const QString &id, const QString& t)
{
    auto *h = query_handlers_.at(id);  // callers responsibility
    if (h->allowTriggerRemap() && h->trigger() != t)
    {
        h->setTrigger(t);
        updateActiveTriggers();
    }
}

void QueryEngine::setFuzzy(const QString &id, bool f)
{
    query_handlers_.at(id)->setFuzzy(f);  // callers responsibility
}

// -------------------------------------------------------------------------------------------------
// Global handlers

map<QString, GlobalQueryHandler*> QueryEngine::globalQueryHandlers()
{
    map<QString, GlobalQueryHandler*> handlers;
    for (const auto &[id, h] : global_query_handlers_)
        handlers.emplace(id, h.handler);
    return handlers;
}

bool QueryEngine::isEnabled(const QString &id) const
{ return global_query_handlers_.at(id).enabled; }

void QueryEngine::setEnabled(const QString &id, bool e)
{
    auto &h = global_query_handlers_.at(id);

    if (h.enabled != e)
    {
        settings()->setValue(QString("%1/%2").arg(id, CFG_GLOBAL_HANDLER_ENABLED), e);
        h.enabled = e;
    }
}

// -------------------------------------------------------------------------------------------------
// Fallback handlers

map<QString, FallbackHandler*> QueryEngine::fallbackHandlers()
{ return fallback_handlers_; }

map<pair<QString,QString>,int> QueryEngine::fallbackOrder() const
{ return fallback_order_; }

void QueryEngine::setFallbackOrder(map<pair<QString,QString>,int> order)
{
    fallback_order_ = order;
    saveFallbackOrder();
}

std::vector<ResultItem> QueryEngine::fallbacks(const QString &query_string) const
{
    vector<pair<Extension*,RankItem>> fallbacks;

    for (auto &[id, h] : fallback_handlers_)
    {
        for (auto i : h->fallbacks(query_string))
        {
            auto it = fallback_order_.find(make_pair(id, i->id()));
            fallbacks.emplace_back(h, RankItem(::move(i),
                                               it == fallback_order_.end() ? 0 : it->second));
        }
    }

    ranges::sort(fallbacks, greater(), &pair<Extension*,RankItem>::second);

    auto v = fallbacks | views::transform([](auto &x){ return ResultItem(*x.first, x.second.item); });

    return {v.begin(), v.end()};
}

// -------------------------------------------------------------------------------------------------

void QueryEngine::onExtensionAdded(albert::Extension *e)
{
    if (auto *h = dynamic_cast<QueryHandler*>(e))
    {
        h->d->loadSettings();
        query_handlers_.emplace(h->id(), h);
        updateActiveTriggers();
        emit queryHandlersChanged();
    }

    if (auto *h = dynamic_cast<GlobalQueryHandler*>(e))
    {
        auto en = settings()->value(QString("%1/%2")
                                        .arg(h->id(), CFG_GLOBAL_HANDLER_ENABLED),
                                            true).toBool();
        global_query_handlers_.emplace(piecewise_construct,
                                       forward_as_tuple(h->id()),
                                       forward_as_tuple(h, en));
        emit globalQueryHandlersChanged();
    }

    if (auto *h = dynamic_cast<FallbackHandler*>(e))
    {
        fallback_handlers_.emplace(h->id(), h);
        emit fallbackHandlersChanged();
    }
}

void QueryEngine::onExtensionRemoved(albert::Extension *e)
{
    if (auto *th = dynamic_cast<QueryHandler*>(e))
    {
        query_handlers_.erase(th->id());
        updateActiveTriggers();
        emit queryHandlersChanged();
    }

    if (auto *gh = dynamic_cast<GlobalQueryHandler*>(e))
    {
        global_query_handlers_.erase(gh->id());
        emit globalQueryHandlersChanged();
    }

    if (auto *fh = dynamic_cast<FallbackHandler*>(e))
    {
        fallback_handlers_.erase(fh->id());
        emit fallbackHandlersChanged();
    }
}

void QueryEngine::saveFallbackOrder() const
{
    // Invert to ordered list
    vector<pair<QString, QString>> o;
    for (const auto &[pair, prio] : fallback_order_)
        o.emplace_back(pair.first, pair.second);
    sort(begin(o), end(o), [&](const auto &a, const auto &b)
         { return fallback_order_.at(a) > fallback_order_.at(b); });

    // Save to settings
    auto s = settings();
    s->beginWriteArray(CFG_FALLBACK_ORDER);
    for (int i = 0; i < (int)o.size(); ++i)
    {
        s->setArrayIndex(i);
        s->setValue(CFG_FALLBACK_EXTENSION, o.at(i).first);
        s->setValue(CFG_FALLBACK_ITEM, o.at(i).second);
    }
    s->endArray();
}

void QueryEngine::loadFallbackOrder()
{
    // Load from settings
    vector<pair<QString, QString>> o;
    auto s = settings();
    int size = s->beginReadArray(CFG_FALLBACK_ORDER);
    for (int i = 0; i < size; ++i)
    {
        s->setArrayIndex(i);
        o.emplace_back(s->value(CFG_FALLBACK_EXTENSION).toString(),
                       s->value(CFG_FALLBACK_ITEM).toString());
    }
    s->endArray();

    // Create order map
    fallback_order_.clear();
    uint rank = 1;
    for (auto it = o.rbegin(); it != o.rend(); ++it, ++rank)
        fallback_order_.emplace(*it, rank);
}


// -------------------------------------------------------------------------------------------------


#include "usagedatabase.h"

using namespace std::chrono;

struct GlobalQueryExecution : public QueryExecution
{
    struct ExecutionData
    {
        vector<RankItem> results;
        uint runtime = 0;
        uint scoring = 0;
    };

    vector<GlobalQueryHandler*> handlers;
    unique_ptr<QFutureWatcher<void>> watcher;
    vector<RankItem> results;
    time_point<system_clock> tp_start;

    GlobalQueryExecution(Query &q, vector<GlobalQueryHandler*> h) :
        QueryExecution(q), handlers(::move(h)) { }

    ExecutionData runGlobalHandler(GlobalQueryHandler *h) const
    {
        ExecutionData d;

        try {
            auto t = system_clock::now();
            if (query.string.isEmpty())
                for (auto &item : h->handleEmptyQuery(query))
                    d.results.emplace_back(::move(item), 0);
            else
                d.results = h->handleGlobalQuery(query);
            d.runtime = duration_cast<chrono::milliseconds>(system_clock::now()-t).count();

            t = system_clock::now();
            h->applyUsageScore(d.results);
            d.scoring = duration_cast<chrono::milliseconds>(system_clock::now()-t).count();

        }
        catch (const exception &e) {
            WARN << QString("GlobalQueryHandler '%1' threw exception:\n")
            .arg(h->id()) << e.what();
        }
        catch (...) {
            WARN << QString("GlobalQueryHandler '%1' threw unknown exception:\n")
            .arg(h->id());
        }

        return d;
    }

    void fetchMore() override
    {
        if (handlers.empty())
        {
            emit finished();
            return;
        }

        else if (!watcher)  // On first execution actually query the handlers
        {
            watcher = make_unique<decltype(watcher)::element_type>();

            connect(watcher.get(), &QFutureWatcher<void>::finished,
                    this, &QueryExecution::finished);


            auto map = [&](GlobalQueryHandler *h){ return runGlobalHandler(h); };

            auto reduce = [](vector<ExecutionData> &result, const ExecutionData &r){
                result.emplace_back(::move(r));
            };

            auto then = [this](vector<ExecutionData> && r){
                for (auto &d : r)
                    ranges::move(d.results, back_inserter(results));

                qCDebug(lcg,).noquote()
                    << QStringLiteral(cblue "┬─ Handling ┬── Scoring ┬──── Count ┬ GLOBAL QUERY #%1 '%2'")
                           .arg(query.id)
                           .arg(query.string);

                for (uint i = 0; i < r.size(); ++i)
                    qCDebug(lcg,).noquote()
                        << QStringLiteral(cblue "│%L1 µs│%L2 µs│%L3 pc│ %4")
                               .arg(r[i].runtime, pad)
                               .arg(r[i].scoring, pad)
                               .arg(r[i].results.size(), pad)
                               .arg(handlers[i]->id());

                auto duration = duration_cast<chrono::milliseconds>(system_clock::now()-tp_start).count();
                qCDebug(lcg,).noquote()
                    << QStringLiteral(cblue "╰%L1 ms│           │%L2 pc│ TOTAL")
                           .arg(duration, pad)
                           .arg(results.size(), pad);
            };


            tp_start = system_clock::now();
            watcher->setFuture(
                QtConcurrent::mappedReduced(handlers, map, reduce, QtConcurrent::UnorderedReduce)
                .then(then)
            );
        }

        else
        {
            ranges::sort(results, std::greater());
            query.add(::move(results) | views::transform(&RankItem::item));
            emit finished();
        }
    }

    void cancel() override
    {
        if (watcher)
            watcher->cancel();
    }
};

GlobalQuery::GlobalQuery(std::map<QString, GQH> &g) : global_handlers(g) {}

QString GlobalQuery::id() const
{ return QStringLiteral("globalquery"); }

QString GlobalQuery::name() const
{ return QStringLiteral("Global query"); }

QString GlobalQuery::description() const
{ return QStringLiteral("Runs a bunch of global query handlers"); }

QueryExecution *GlobalQuery::createQueryExecution(Query &q)
{

}
