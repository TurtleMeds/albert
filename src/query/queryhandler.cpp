// Copyright (c) 2023-2024 Manuel Schneider

#include "queryhandler.h"
#include "queryhandlerprivate.hpp"
using namespace albert;
using namespace std;

QueryHandler::QueryHandler() : d(std::make_unique<QueryHandlerPrivate>(*this)) {}

QueryHandler::~QueryHandler() = default;

bool QueryHandler::allowTriggerRemap() const { return true; }

QString QueryHandler::defaultTrigger() const { return QString("%1 ").arg(id()); }

QString QueryHandler::trigger() const { return d->trigger(); }

void QueryHandler::setTrigger(const QString &) {}

bool QueryHandler::supportsFuzzyMatching() const { return false; }

bool QueryHandler::fuzzy() const { return d->fuzzy(); }

void QueryHandler::setFuzzy(bool) { }

QString QueryHandler::synopsis(const QString &) const { return {}; }

// -------------------------------------------------------------------------------------------------

#include <QFutureWatcher>
#include <QtConcurrent>

class ThreadedQueryExecution::Private
{
public:
    Query &query;
    ThreadedQueryHandler &handler;
};

ThreadedQueryExecution::ThreadedQueryExecution(const Query &q, ThreadedQueryHandler &h) :
    query(q), d(std::make_unique<Private>(h))
{
}

ThreadedQueryExecution::~ThreadedQueryExecution() { d->watcher.waitForFinished(); }

void ThreadedQueryExecution::fetchMore()
{
    auto *w = new QFutureWatcher<void>(&d->query);

    connect(w, &QFutureWatcher<void>::finished, [this](){
        emit d->query.activeChanged(true);
    });

    w->setFuture(QtConcurrent::run([this] {
        try
        {
            d->handler.handle(*this);
        }
        catch (const exception &e)
        {
            WARN << QString("QueryHandler '%1' threw exception:\n")
                        .arg(d->handler.id()) << e.what();
        }
        catch (...)
        {
            WARN << QString("QueryHandler '%1' threw unknown exception.")
                        .arg(d->handler.id());
        }
    }));
}

void ThreadedQueryExecution::cancel() {}

QueryExecution *ThreadedQueryHandler::createQueryExecution(Query &query)
{
    return new ThreadedQueryExecution(query, *this);
}


// -------------------------------------------------------------------------------------------------







