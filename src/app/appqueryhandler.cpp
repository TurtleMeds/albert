// Copyright (c) 2023-2024 Manuel Schneider

#include "albert.h"
#include "app.h"
#include "appqueryhandler.h"
#include "matcher.h"
#include "standarditem.h"
#include <QString>
#include <QUrl>
using namespace albert;
using namespace std;

const QStringList AppQueryHandler::icon_urls{QStringLiteral(":app_icon")};

AppQueryHandler::AppQueryHandler()
{
    items_ = {
        StandardItem::make(
            "sett",
            tr("Settings"),
            tr("Albert settings"),
            icon_urls,
            {{ "sett", tr("Open"), [] { showSettings(); } }}),

        StandardItem::make(
            "quit",
            tr("Quit"),
            tr("Quit Albert"),
            icon_urls,
            {{ "quit", tr("Quit"), [] { quit(); } }}),

        StandardItem::make(
            "restart",
            tr("Restart"),
            tr("Restart Albert"),
            icon_urls,
            {{ "restart", tr("Restart"), [] { restart(); } }}),

        StandardItem::make(
            "cache",
            tr("Cache location"),
            tr("Albert cache location"),
            icon_urls,
            {{ "cache", tr("Open"), [] { open(cacheLocation()); } }}),

        StandardItem::make(
            "config",
            tr("Config location"),
            tr("Albert config location"),
            icon_urls,
            {{ "config", tr("Open"), [] { open(configLocation()); } }}),

        StandardItem::make(
            "data",
            tr("Data location"),
            tr("Albert data location"),
            icon_urls,
            {{ "data", tr("Open"), [] { open(dataLocation()); } }})
    };
}

QString AppQueryHandler::id() const
{ return QStringLiteral("albert"); }

QString AppQueryHandler::name() const
{ return QStringLiteral("Albert"); }

QString AppQueryHandler::description() const
{ return tr("Control the app"); }

QString AppQueryHandler::defaultTrigger() const
{ return QStringLiteral("albert "); }

vector<RankItem> AppQueryHandler::handleGlobalQuery(const Query *query)
{
    Matcher matcher(query->string());
    vector<RankItem> rank_items;
    for (const auto &item : items_)
        if (auto m = matcher.match(item->text()); m)
            rank_items.emplace_back(item, m);
    return rank_items;
}
