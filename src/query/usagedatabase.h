// Copyright (c) 2022-2024 Manuel Schneider

#pragma once
#include <QSqlDatabase>
#include <QString>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
namespace albert {
class Extension;
class RankItem;
}

struct Activation {
    Activation(QString q, QString e, QString i, QString a);
    QString query;
    QString extension_id;
    QString item_id;
    QString action_id;
};

using Key = std::pair<QString, QString>;
using UsageScores = std::unordered_map<Key, double>;

class UsageHistory
{
public:
    static void initialize();

    static void applyScores(const QString &extension_id, std::vector<albert::RankItem> &rank_items);

    static double memoryDecay();
    static void setMemoryDecay(double);

    static bool prioritizePerfectMatch();
    static void setPrioritizePerfectMatch(bool);

    static void addActivation(const QString &query, const QString &extension,
                              const QString &item, const QString &action);

private:
    static void updateScores();

    static std::shared_mutex global_data_mutex_;
    static UsageScores usage_scores_;
    static bool prioritize_perfect_match_;
    static double memory_decay_;

    static std::recursive_mutex db_recursive_mutex_;
    static void db_connect();
    static void db_initialize();
    static void db_clearActivations();
    static void db_addActivation(const QString &query, const QString &extension,
                                 const QString &item, const QString &action);
};


