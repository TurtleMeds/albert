// Copyright (c) 2024 Manuel Schneider

#pragma once
#include <QObject>
#include <vector>
#include <memory>
class QueryEngine;
namespace albert {
class QueryExecution;
class Frontend;
}

class Session : public QObject
{
    Q_OBJECT

public:

    Session(QueryEngine &engine, albert::Frontend &frontend);
    ~Session();

private:

    void runQuery(const QString &query);
    void displayQuery();

    QueryEngine &engine_;
    albert::Frontend &frontend_;
    std::vector<std::unique_ptr<albert::QueryExecution>> queries_;

};

