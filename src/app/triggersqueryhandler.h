// Copyright (c) 2023-2024 Manuel Schneider

#pragma once
#include "globalqueryhandler.h"
#include <QCoreApplication>
class QueryEngine;
class App;

class TriggersQueryHandler : public albert::GlobalQueryHandler
{
    Q_DECLARE_TR_FUNCTIONS(TriggersQueryHandler)

public:

    TriggersQueryHandler(const QueryEngine &query_engine, App &app);
    QString id() const override;
    QString name() const override;
    QString description() const override;
    void handleTriggerQuery(albert::Query *) override;
    std::vector<albert::RankItem> handleGlobalQuery(const albert::Query*) override;

private:

    std::shared_ptr<albert::Item> makeItem(const QString &trigger, Extension *handler) const;

    static const QStringList icon_urls;
    const QueryEngine &query_engine_;
    App &app_;

};
