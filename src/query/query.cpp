// Copyright (c) 2023-2024 Manuel Schneider

#include "query.h"
#include "item.h"
#include "queryhandler.h"
#include "usagedatabase.h"
using namespace albert;
using namespace std;
using namespace std::chrono;

Q_LOGGING_CATEGORY(lcq, "albert.query")

class Query::Private
{
public:
    static uint query_count;
    Query &query;
    unique_ptr<QueryExecution> execution;
    vector<ResultItem> matches;
    vector<ResultItem> fallbacks;
    bool valid = true;
    bool active = true;
    bool canFetchMore = true;
    mutex buffer_mutex;
    vector<ResultItem> buffer;
    time_point<system_clock> tp_fetch_begin;

    void setActive(bool v)
    {
        if (v)
            tp_fetch_begin = system_clock::now();
        else
        {
            auto dur = duration_cast<milliseconds>(system_clock::now() - tp_fetch_begin).count();
            qCDebug(lcq,).noquote()
                << QStringLiteral(cmagenta " #%1 │%2 ms│ '%3' '%4'" creset)
                       .arg(query.id)
                       .arg(dur, 6)
                       .arg(query.trigger, query.string);
        }

        active = v;
        emit query.activeChanged(v);
    }

    void fetchMore() {
        setActive(true);
        if (canFetchMore)
        {
            canFetchMore = false;  // see also collectResults
            execution->fetchMore();
        }
    }

    void cancel() {
        if (valid) {
            valid = false;
            if (execution && active)
                execution->cancel();
        }
    }
};

uint Query::Private::query_count = 0;

Query::Query(unique_ptr<QueryExecution> execution,
             vector<ResultItem> &&fallbacks,
             const QString &tri,
             const QString &str,
             const QString &syn):
    id(d->query_count++),
    trigger(tri),
    string(str),
    synopsis(syn),
    d(make_unique<Query::Private>(*this))
{
    d->execution = ::move(execution);
    d->fallbacks = ::move(fallbacks);

    connect(d->execution.get(), &QueryExecution::finished,
            this, [this]{ d->setActive(false); });
}


Query::~Query()
{
    if (active())
    {
        cancel();
        WARN << QString("Busy wait on query: #%1").arg(id);
        d->execution.reset();
    }
    DEBG << QString("Query deleted. [#%1 '%2']").arg(id).arg(string);
}

Query::operator QString() const { return string; }

bool Query::triggered() const  { return !trigger.isEmpty(); }

bool Query::global() const { return trigger.isEmpty(); }

const bool &Query::valid() const { return d->valid; }

bool Query::active() const { return d->active; }

bool Query::canFetchMore() const { return valid() && !active() && d->canFetchMore; }

void Query::fetchMore() { d->fetchMore(); }

void Query::cancel() { d->cancel(); }

const vector<ResultItem> &Query::matches() const { return d->matches; }

const vector<ResultItem> &Query::fallbacks() const { return d->fallbacks; }

static bool activate(const vector<ResultItem> &result_items, Query &q, uint iidx, uint aidx)
{
    try {
        auto &[e, i] = result_items.at(iidx);

        try {
            auto a = i->actions().at(aidx);

            INFO << QString("Activating action %1 > %2 > %3 (%4 > %5 > %6) ")
                        .arg(e.id(), i->id(), a.id, e.name(), i->text(), a.text);

            // Order is cumbersome here
            UsageHistory::addActivation(q.string, e.id(), i->id(), a.id);

            // May delete the query, due to hide()
            // Notes to self:
            // - QTimer::singleShot(0, this, [a]{ a.function(); });
            //   Disconnects on query deletion.

            a.function();  // May delete the query, due to hide()
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

bool Query::activateMatch(uint i, uint a) { return activate(d->matches, *this, i, a); }

bool Query::activateFallback(uint i, uint a) { return activate(d->fallbacks, *this, i, a); }

lock_guard<mutex> Query::getLock() { return lock_guard(d->buffer_mutex); }

vector<ResultItem> &Query::buffer() { return d->buffer; }

void Query::invokeCollectResults()
{
    QMetaObject::invokeMethod(this, &Query::collectResults, Qt::AutoConnection);
}

void Query::collectResults()
{
    if (!valid())
        return;

    auto lock = getLock();

    if (d->buffer.empty()) // buffer should not be empty here
        return;

    d->canFetchMore = true;  // see also Query::Private::fetchMore

    d->matches.reserve(d->matches.size() + d->buffer.size());

    emit matchesAboutToBeAdded(d->buffer.size());

    for (auto &item : d->buffer)
        d->matches.emplace_back(::move(item));

    emit matchesAdded();

    d->buffer.clear();
}

