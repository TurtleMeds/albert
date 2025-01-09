// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <QString>
#include <albert/export.h>
#include <albert/query.h>
class QueryEngine;

namespace albert
{
class ALBERT_EXPORT Session
{
public:
    Session(QueryEngine &engine);
    ~Session();

    albert::Query &query(const QString &query);
    albert::Query *currentQuery() const;
    albert::Query *pastQuery() const;

private:
    class Private;
    std::unique_ptr<Private> d;
};

}  // namespace albert
