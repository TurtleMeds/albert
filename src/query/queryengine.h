// Copyright (c) 2023-2024 Manuel Schneider

#pragma once
#include "queryhandler.h"
#include <QObject>
#include <map>
#include <memory>
namespace albert {
class Session;
class Query;
class ExtensionRegistry;
class FallbackHandler;
class GlobalQueryHandler;
}


struct GQH {
    GQH(albert::GlobalQueryHandler *h, bool e):
        handler(h), enabled(e)
    {}
    albert::GlobalQueryHandler *handler;
    bool enabled;
};


struct GlobalQuery : public albert::QueryHandler
{
    GlobalQuery(std::map<QString, GQH> &g);

    QString id() const override;
    QString name() const override;
    QString description() const override;
    albert::QueryExecution *createQueryExecution(albert::Query &q) override;

    std::map<QString, GQH> &global_handlers;
};


class QueryEngine : public QObject
{
    Q_OBJECT

public:

    QueryEngine(albert::ExtensionRegistry&);

    std::unique_ptr<albert::Session> createSession();
    std::unique_ptr<albert::Query> query(const QString &query);

    // Query handlers
    const std::map<QString, albert::QueryHandler*> &queryHandlers() const;
    const std::map<QString, albert::QueryHandler*> &activeTriggerHandlers() const;
    void setTrigger(const QString&, const QString&);
    void setFuzzy(const QString&, bool);

    // Global query handlers
    std::map<QString, albert::GlobalQueryHandler*> globalQueryHandlers();
    bool isEnabled(const QString&) const;
    void setEnabled(const QString&, bool = true);

    // Fallback handlers
    std::map<QString, albert::FallbackHandler*> fallbackHandlers();
    std::map<std::pair<QString, QString>, int> fallbackOrder() const;
    void setFallbackOrder(std::map<std::pair<QString, QString>, int>);

private:

    std::vector<albert::ResultItem> fallbacks(const QString &query) const;
    void onExtensionAdded(albert::Extension*);
    void onExtensionRemoved(albert::Extension*);
    void updateActiveTriggers();
    void saveFallbackOrder() const;
    void loadFallbackOrder();

    albert::ExtensionRegistry &registry_;

    std::map<QString, albert::QueryHandler*> query_handlers_;
    std::map<QString, GQH> global_query_handlers_;
    std::map<QString, albert::FallbackHandler*> fallback_handlers_;

    std::map<QString, albert::QueryHandler*> active_triggers_;
    std::map<std::pair<QString, QString>, int> fallback_order_;

    GlobalQuery global_query{global_query_handlers_};

signals:

    void queryHandlersChanged();
    void globalQueryHandlersChanged();
    void fallbackHandlersChanged();

};
