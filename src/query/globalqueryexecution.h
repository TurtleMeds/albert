// Copyright (c) 2022-2024 Manuel Schneider

#pragma once
#include "triggerqueryexecution.h"
#include "triggerqueryhandler.h"
namespace albert { class GlobalQueryHandler; }

class GlobalQueryExecution : public TriggerQueryExecution, public albert::TriggerQueryHandler
{
public:
    GlobalQueryExecution(std::vector<albert::ResultItem> fallbacks,
                         std::vector<albert::GlobalQueryHandler *> handlers,
                         const QString &string);

    QString id() const override;
    QString name() const override;
    QString description() const override;
    void handleTriggerQuery(TriggerQuery &query) override;

private:
    std::vector<albert::GlobalQueryHandler *> handlers_;
};
