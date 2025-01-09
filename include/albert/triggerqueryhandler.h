// // SPDX-FileCopyrightText: 2024 Manuel Schneider
// // SPDX-License-Identifier: MIT

// #pragma once
// #include "threadedqueryexecution.h"
// #include <albert/queryhandler.h>

// namespace albert
// {
// class ThreadedQueryExecution;

// ///
// /// Abstract trigger query handler.
// ///
// /// If the trigger matches this handler is the only query handler chosen to
// /// process the user query. Inherit this class if you dont want your results to
// /// be reordered or if you want to display your items of a long running query
// /// as soon as they are available.
// ///

// class [[deprecated]] ALBERT_EXPORT TriggerQueryHandler : public QueryHandler
// {
// public:

//     /// Returns a new \ref ThreadedQueryExecution instance executing \ref handleTriggerQuery.
//     ThreadedQueryExecution *createQueryExecution(Query&) override;

//     /// The trigger query processing function.
//     /// @note Executed in a worker thread.
//     virtual void handleTriggerQuery(const albert::Query&, ThreadedQueryExecution&) = 0;

// protected:

//     ~TriggerQueryHandler() override;

// };

// }  // namespace albert
