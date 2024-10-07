// Copyright (c) 2022-2024 Manuel Schneider

#pragma once
#include "queryexecution.h"
#include "triggerquery.h"
#include <QFutureWatcher>
#include <QString>
#include <memory>
#include <mutex>
#include <vector>
namespace albert {
class Item;
class TriggerQueryHandler;
}

class TriggerQueryExecution : public albert::QueryExecution, public albert::TriggerQuery
{
    Q_OBJECT  // Needed for invokable collectResults
public:
    TriggerQueryExecution(std::vector<albert::ResultItem> fallbacks,
                          albert::TriggerQueryHandler &query_handler,
                          const QString &trigger,
                          const QString &string);
    ~TriggerQueryExecution();

    QString synopsis() const override;
    QString trigger() const override;
    QString string() const override;

    bool isValid() const override;
    bool isProcessing() const override;
    bool canFetchMore() const override;

    void fetchMore() override;
    void cancel() override;
    void invalidate();

    void setCanFetchMore() override;

    albert::QueryState *state() const override;
    albert::QueryState *setState(std::unique_ptr<albert::QueryState> state) override;

    void add(const std::shared_ptr<albert::Item> &item) override;
    void add(std::shared_ptr<albert::Item> &&item) override;
    void add(const std::vector<std::shared_ptr<albert::Item>> &items) override;
    void add(std::vector<std::shared_ptr<albert::Item>> &&items) override;

    const std::vector<albert::ResultItem> &matches() const override;
    const std::vector<albert::ResultItem> &fallbacks() const override;

    bool activateMatch(uint item_index, uint action_index) override;
    bool activateFallback(uint item_index, uint action_index) override;

protected:
    void invokeCollectResults();
    Q_INVOKABLE void collectResults();

    static uint query_count;
    const uint query_id;

    albert::TriggerQueryHandler &handler_;

    const QString synopsis_;
    const QString trigger_;
    const QString string_;

    bool valid_;
    bool can_fetch_more_;
    std::unique_ptr<albert::QueryState> state_;

    std::vector<albert::ResultItem> matches_;
    std::vector<albert::ResultItem> fallbacks_;

    QFutureWatcher<void> future_watcher_;
    std::mutex results_buffer_mutex_;
    std::vector<albert::ResultItem> results_buffer_;
};
