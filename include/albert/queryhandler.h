// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <QObject>
#include <albert/query.h>
#include <albert/extension.h>
class QueryHandlerPrivate;
class QueryEngine;

#include <rankitem.h>

namespace albert
{

/*

 # 1
   - QueryHandler::fetchMore(Query)
   - QueryHandler::cancel(Query)
   - Query::setInactive()
   - Query::state/setState()
   - ThreadedQueryHandler::fetchMoreThreaded()
# 2
   - Queryexecution::fetchMore(Query)
   - Queryexecution::cancel(Query)
   - Queryexecution::canFetchMore(Query)






 */

// -------------------------------------------------------------------------------------------------
struct QueryExecution
{
    QString trigger;
    QString string;
    QString synopsis;
    bool valid;
};
struct ThreadedQueryExecution : public QueryExecution
{
    void add();
};
struct AsyncQueryExecution : public QueryExecution
{
    void add(...);
    void setDone();
};



class ALBERT_EXPORT AsyncQueryExecution : public QObject
{
    Q_OBJECT
public:
    void add();

    // ------------- #1 -------------
public:
    virtual void fetchMore() = 0;
    virtual void cancel() = 0;
signals:
    void done();

    // ------------- #2 -------------
public:
    class State { virtual ~State(); };
    State state();
    void setState(State*);

};

// -------------------------------------------------------------------------------------------------

class ALBERT_EXPORT QueryHandler : virtual public Extension
{
public:
    virtual bool allowTriggerRemap() const;
    virtual QString defaultTrigger() const;
    virtual QString trigger() const;
    virtual void onTriggerChanged(const QString &);

    virtual bool supportsFuzzyMatching() const;
    virtual bool fuzzy() const;
    virtual void setFuzzy(bool);

    virtual QString synopsis(const QString &query) const;

    virtual void handle(AsyncQueryExecution&) = 0;
    virtual void cancel(AsyncQueryExecution&) = 0;

protected:
    QueryHandler();
    ~QueryHandler() override;

private:
    friend QueryEngine;
    std::unique_ptr<QueryHandlerPrivate> d;
};



class ALBERT_EXPORT ThreadedQueryHandler : public QueryHandler
{
public:
    virtual void handle(ThreadedQueryExecution &) = 0;
private:
    void handle(AsyncQueryExecution &) override final {}
};



class ALBERT_EXPORT GlobalQueryHandler : public ThreadedQueryHandler
{
public:
    virtual std::vector<RankItem> handleGlobalQuery(QueryExecution&) = 0;
    virtual std::vector<std::shared_ptr<Item>> handleEmptyQuery(QueryExecution&);
    // void applyUsageScore(std::vector<RankItem> &) const;
private:
    void handle(ThreadedQueryExecution &) override final {}
};







































// class CoreQueryExecution : public Query, public QueryExecution
// {
// public:
//     CoreQueryExecution(QueryHandler &handler, const QString &trigger, const QString &string):
//         executor_(handler.createQueryExecutor()),
//         trigger_(trigger),
//         string_(string),
//         synopsis_(handler.synopsis(string)),
//         valid_(true)
//     {

//     }

//     QString trigger() const override { return trigger_; }
//     QString string() const override { return string_; }
//     QString synopsis() const override { return synopsis_; }
//     bool isValid() const override { return valid_; }
//     bool isProcessing() const override { return executor_->isProcessing();}
//     bool canFetchMore() const override { return isValid() && !isProcessing()
//                                                 && executor_->canFetchMore(); }
//     void fetchMore() override { executor_->process(*this);}
//     void cancel() override {
//         valid_ = false;
//         executor_->cancel();
//         emit invalidated();
//     }
//     const std::vector<ResultItem> &matches() const override { return matches_; }
//     const std::vector<ResultItem> &fallbacks() const override { return fallbacks_; }
//     bool activateMatch(uint item, uint action) override { return {}; }
//     bool activateFallback(uint item, uint action) override { return {}; }


// private:

//     std::unique_ptr<QueryHandler::QueryExecutor> executor_;
//     QString trigger_;
//     QString string_;
//     QString synopsis_;
//     std::vector<ResultItem> matches_;
//     std::vector<ResultItem> fallbacks_;
//     bool valid_;

// signals:

//     void matchesAboutToBeAdded(uint count);
//     void matchesAdded();
//     void invalidated();
//     void stateChanged(bool isProcessing);
// };


// class ALBERT_EXPORT QueryExecutionImpl : public QObject, public Query
// {
//     Q_OBJECT

//     /// Constructs a query with `trigger`, `string` handled by `handler`

//     // Query

//     /// Returns the trigger that dispatched this query.
//     const QString &trigger() const;

//     /// Returns the query string _excluding_ the trigger.
//     const QString &string() const;


//     // Results

//     const std::vector<ResultItem> &matches() const;
//     const std::vector<ResultItem> &fallbacks() const;
//     bool activateMatch(uint item, uint action = {}) const;
//     bool activateFallback(uint item, uint action = {}) const;

//     ///
//     /// Add `item` to the results.
//     /// @note Use range add if you can to avoid UI flicker.
//     ///
//     void add(auto &&item)
//     {
//         if (!valid())
//             return;

//         auto lock = getLock();
//         if constexpr (std::same_as<decltype(item), std::shared_ptr<Item>>)
//             buffer().emplace_back(handler(), std::forward<decltype(item)>(item));
//         else if constexpr (std::same_as<decltype(item), ResultItem>)
//             buffer().emplace_back(std::forward<decltype(item)>(item));
//         else
//             static_assert(false, "Supports only Item and ResultItem");
//         invokeCollectResults();
//     }

//     ///
//     /// Add `items` to the results.
//     ///
//     void add(std::ranges::range auto &&items)
//     {
//         if (!valid() || items.empty())
//             return;

//         auto lock = getLock();
//         auto &buf = buffer();
//         buf.reserve(buf.size() + items.size());
//         for (auto &&item : items)
//             if constexpr (std::same_as<std::ranges::range_value_t<decltype(items)>,
//                                        std::shared_ptr<Item>>)
//                 buf.emplace_back(handler(), forward_like<decltype(items)>(item));
//             else if constexpr (std::same_as<std::ranges::range_value_t<decltype(items)>,
//                                             ResultItem>)
//                 buf.emplace_back(forward_like<decltype(items)>(item));
//             else
//                 static_assert(false, "Supports only Item and ResultItem");
//         invokeCollectResults();
//     }


//     // Execution

//     /// Returns the handler of this query.
//     const QueryHandler &handler() const;

//     /// Returns the execution of this query.
//     std::unique_ptr<QueryExecution> execution;

//     /// Returns `false` if the query has been cancelled otherwise `true`.
//     virtual bool valid() const;



//     /// Returns the synopsis of this query.
//     const QString &synopsis() const;


// signals:

//     /// Emitted before `count` matches are added to the matches.
//     void matchesAboutToBeAdded(uint count);

//     /// Emitted after matches have been added to the matches.
//     void matchesAdded();

//     /// Emitted when the query has been invalidated.
//     void invalidated();

//     /// Emitted when query processing started or finished.
//     void isProcessingChanged(bool processing);

// private:

//     std::lock_guard<std::mutex> getLock();
//     std::vector<ResultItem> &buffer();
//     void invokeCollectResults();
//     void collectResults();

//     class Private;
//     std::unique_ptr<Private> d;
// };







// // ------------------------------- Example implementations -----------------------------------------

// class QH : public QueryHandler
// {
// public:
//     struct E : public QueryExecutor
//     {
//         bool more = true;
//         QNetworkReply *reply;

//         bool canFetchMore() const override { return more; }

//         bool isProcessing() const override { return reply && reply->isRunning(); }

//         void cancel() override {
//             reply->abort();
//             reply->deleteLater();
//             reply = nullptr;
//         }

//         void process(QueryExecution&) override {
//             reply = albert::network().get(QNetworkRequest(QUrl("https://example.com")));
//             connect(reply, &QNetworkReply::finished, this, [this] {
//                 /* hanlde reply*/
//                 more = false;
//             });
//         }
//     };

//     std::unique_ptr<QueryExecutor> createQueryExecutor() override { return std::make_unique<E>(); }
// };



// class LegacyTQH : public LegacyTriggerQueryHandler
// {
// public:
//     void handleQuery(QueryExecution &) override {

//     }
// };

// class StatefulThreadedQH : public QueryHandler
// {
// public:
//     struct StatefulExecutor : public ThreadedQueryExecutor
//     {
//         int executions = 0;
//         void processThreaded(QueryExecution &q) override
//         {
//             q.add(StandardItem::make({}, {}, {}, {}, {}, {}));
//             ++executions;
//         }
//     };

//     std::unique_ptr<QueryExecutor> createQueryExecutor() override
//     {
//         return std::make_unique<StatefulExecutor>();
//     }
// };




// ----------------------------------------------------------------------------


// class GlobalQueryExecution : public AbstractQueryExecution
// {
// public:
//     bool canFetchMore() const override { return {}; } // future is done
//     void process() override final {}  // run handlers in threads
//     bool isProcessing() const override { return {}; }  // future is running
//     void cancel() override {}  // cancel future
//     bool isValid() const override { return {}; }  // future is cancelled

// private:
//           // ExtensionRegistry registry_;
//           // std::vector<albert::GlobalQueryHandler> handlers_;
// };


// class GlobalQueryHandler : public AbstractQueryHandler
// {
// public:

//             /// Returns GlobalQueryExecution
//     virtual std::unique_ptr<AbstractQueryExecution>
//     createQueryExecution(const QString &trigger, const QString &string) = 0;

// private:
//           // hanlders_;
// };


// ----------------------------------------------------------------------------

// class ConcreteQueryExecution : public QueryExecution
// {
// public:
//     virtual QString trigger() = 0;
//     virtual QString string() = 0;
//     virtual QString synopsis() = 0;

//     virtual bool canProcessFurther() = 0;  // implies             !isFinished !isProcessing !isValid
//     virtual bool isProcessing() = 0;       // implies !canProcess !isFinishes
//     virtual bool isValid() = 0;            // implies !canProcess
//     virtual bool isFinished() = 0;         // implies !canProcess
//     virtual void process() = 0;
//     virtual void cancelProcessing() = 0;

//     std::vector<ResultItem> &matches();
//     std::vector<ResultItem> &fallbacks();

// signals:
//           // void matchesAboutToBeAdded(uint count);
//           // void matchesAdded();
//           // void invalidated();
//           // void stateChanged(bool processing);
// };

// class ConcreteQueryExecutor : public QueryExecutor
// {
// public:
//     std::unique_ptr<QueryExecution>
//     createQueryExecution(const QString &trigger, const QString &string) override
//     {
//         return std::make_unique<ConcreteQueryExecution>(trigger, string);
//     }
// };




}  // namespace albert
