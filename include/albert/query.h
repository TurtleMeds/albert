// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include "fallbackhandler.h"
#include "itemsmodel.h"
#include "triggerqueryhandler.h"
#include <QAbstractListModel>
#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <albert/export.h>
#include <memory>
#include <vector>
class QueryEngine;
namespace albert {
class FallbackHandler;
class Item;
class TriggerQueryHandler;
}


namespace albert
{

///
/// The query context.
///
class ALBERT_EXPORT QueryContext
{
public:
    virtual ~QueryContext();
};


///
/// The query object.
///
class ALBERT_EXPORT Query : public QObject
{
    Q_OBJECT

public:

    /// The synopsis of this query.
    Q_INVOKABLE QString synopsis() const;


    /// The trigger of this query.
    Q_INVOKABLE QString trigger() const;

    /// True if this query has a trigger.
    Q_INVOKABLE bool isTriggered() const;


    /// Query string _excluding_ the trigger.
    Q_INVOKABLE QString string() const;



    /// True if query has not been cancelled.
    Q_INVOKABLE const bool &isValid() const;



    /// True if the query thread stopped.
    Q_INVOKABLE bool isFinished() const;


    /// Returns the matches.
    Q_INVOKABLE QAbstractListModel *matches();

    /// Returns the fallbacks.
    Q_INVOKABLE QAbstractListModel *fallbacks();


    Q_INVOKABLE QueryContext *context();
    Q_INVOKABLE void setContext(std::unique_ptr<QueryContext>);


    /// Executes match a match action.
    Q_INVOKABLE void activateMatch(uint item, uint action = 0);

    /// Executes match a fallback action.
    Q_INVOKABLE void activateFallback(uint item, uint action = 0);

    /// Copy add single item.
    /// @note Use batch add if you can to avoid UI flicker.
    /// @see add(const std::vector<std::shared_ptr<Item>> &items)
    void add(const std::shared_ptr<Item> &item);

    /// Move add single item.
    /// @note Use batch add if you can to avoid UI flicker.
    /// @see add(std::vector<std::shared_ptr<Item>> &&items)
    void add(std::shared_ptr<Item> &&item);

    /// Copy add multiple items.
    void add(const std::vector<std::shared_ptr<Item>> &items);

    /// Move add multiple items.
    void add(std::vector<std::shared_ptr<Item>> &&items);

    /// Type conversion to QString
    /// Syntactic sugar for context conversions
    /// @since 0.24
    inline operator QString() const { return string(); }

private:

    Query(QueryEngine &e,
          std::vector<FallbackHandler*> &&fallback_handlers,
          albert::TriggerQueryHandler &query_handler,
          const QString &string,
          const QString &trigger);
    ~Query() override;

    void preAdd();
    void postAdd();
    void collectResults();

    friend class QueryEngine;
    class Private;
    std::unique_ptr<Private> d;

signals:

    /// Emitted when the query finished processing
    void finished();
};

}
