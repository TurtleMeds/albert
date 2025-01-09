// Copyright (c) 2023-2024 Manuel Schneider

#include "app.h"
#include "matcher.h"
#include "queryengine.h"
#include "standarditem.h"
// #include <threadedqueryexecution.h>
#include "triggersqueryhandler.h"
using namespace albert;
using namespace std;

const QStringList TriggersQueryHandler::icon_urls{QStringLiteral(":app_icon")};

TriggersQueryHandler::TriggersQueryHandler(const QueryEngine &query_engine):
    query_engine_(query_engine) {}

QString TriggersQueryHandler::id() const
{ return QStringLiteral("triggers"); }

QString TriggersQueryHandler::name() const
{ return QStringLiteral("Triggers"); }

QString TriggersQueryHandler::description() const
{ return tr("Trigger completion items."); }

static shared_ptr<Item> make_item(const QString &trigger, QueryHandler &handler)
{
    return StandardItem::make(
        handler.id(),
        QString(trigger).replace(" ", "•"),
        QString("%1 - %2").arg(handler.name(), handler.description()),
        trigger,
        {QStringLiteral("gen:?&text=🚀")},
        {}  // Todo input action action
    );
}

std::vector<RankItem> TriggersQueryHandler::handleGlobalQuery(const albert::Query &query)
{
    vector<RankItem> r;

    if (query.triggered())
    {
        Matcher matcher(query);
        for (const auto &[t, h] : query_engine_.activeTriggerHandlers())
            if (auto m = matcher.match(t, h->name(), h->id()); m)
                r.emplace_back(make_item(t, *h), m);
    }
    else
    {
        Matcher matcher(query, {.ignore_case=false, .ignore_word_order=false});
        for (const auto &[t, h] : query_engine_.activeTriggerHandlers())
            if (auto m = matcher.match(t); m)
                r.emplace_back(make_item(t, *h), m);
    }

    ranges::sort(r, greater());

    return r;
}
