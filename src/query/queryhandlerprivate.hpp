// Copyright (c) 2023-2024 Manuel Schneider

#pragma once
#include <QSettings>
#include <QString>
#include "albert.h"
#include "queryhandler.h"
#include "matchconfig.h"
using namespace albert;
static const char *CFG_TRIGGER = "trigger";
static const char *CFG_FUZZY = "fuzzy";

class QueryHandlerPrivate
{
public:
    QueryHandlerPrivate(QueryHandler &qh) : q(qh) { }

    void loadSettings()
    {
        auto s = settings();
        s->beginGroup(q.id());

        if (q.allowTriggerRemap())
        {
            trigger_ = s->value(CFG_TRIGGER, q.defaultTrigger()).toString();
            q.setTrigger(trigger_);  // notify handler
        }

        if (q.supportsFuzzyMatching())
        {
            match_config.fuzzy = s->value(CFG_FUZZY, false).toBool();
            q.setFuzzy(match_config.fuzzy);  // notify handler
        }
    }

    const QString &trigger() const { return trigger_; }

    void setTrigger(const QString &t)
    {
        if (q.allowTriggerRemap() && trigger_ != t)
        {
            if (t.isEmpty() || t == q.defaultTrigger())
            {
                trigger_ = q.defaultTrigger();
                settings()->remove(QString("%1/%2").arg(q.id(), CFG_TRIGGER));
            }
            else
            {
                trigger_ = t;
                settings()->setValue(QString("%1/%2").arg(q.id(), CFG_TRIGGER), t);
            }
            q.setTrigger(t);  // notify handler
        }
    }

    bool fuzzy() const { return match_config.fuzzy; }

    void setFuzzy(bool f)
    {
        if (q.supportsFuzzyMatching() && match_config.fuzzy != f)
        {
            match_config.fuzzy = f;
            settings()->setValue(QString("%1/%2").arg(q.id(), CFG_FUZZY), f);
            q.setFuzzy(f);  // notify handler
        }
    }

private:
    QueryHandler &q;
    QString trigger_;
    MatchConfig match_config;
};
