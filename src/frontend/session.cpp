// Copyright (c) 2024 Manuel Schneider

#include "frontend.h"
#include "queryengine.h"
#include "query.h"
#include "session.h"
#include <memory>
using namespace albert;
using namespace std;

class Session::Private
{
public:
    Session &q;
    QueryEngine &engine;
    std::list<std::unique_ptr<albert::Query>> queries;

    void cancelLastQuery() {
        if(!queries.empty())
            queries.back()->cancel();
    }
};

Session::Session(QueryEngine &e):
    d(make_unique<Private>(*this, e))
{
    // connect(&frontend_, &Frontend::inputChanged, this, &Session::runQuery);
    // runQuery(frontend_.input());
}

Session::~Session()
{
    // disconnect(&fronend_, &Frontend::inputChanged,
    //            this, &Session::runQuery);
    // frontend_.setQuery(nullptr);

    d->cancelLastQuery();

    for (auto &q : d->queries)
        q.release()->deleteLater();
}

Query &Session::query(const QString &query)
{
    d->cancelLastQuery();

    return *d->queries.emplace_back(d->engine.query(query));

    // q->setParent(this);  // double-free?  // important for qml ownership determination
    // frontend_.setQuery(q.get());
}

Query *Session::currentQuery() const
{
    if(d->queries.size() > 0)
        return d->queries.back().get();
    return nullptr;
}

Query *Session::pastQuery() const
{
    if(d->queries.size() > 1)
        return next(d->queries.rbegin())->get();
    return nullptr;
};

