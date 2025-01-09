// // Copyright (c) 2023-2024 Manuel Schneider

// #include "triggerqueryhandler.h"
// #include "threadedqueryexecution.h"
// #include <albert/logging.h>
// #include <QtConcurrent>
// using namespace albert;
// using namespace std;

// TriggerQueryHandler::~TriggerQueryHandler() = default;

// ThreadedQueryExecution *TriggerQueryHandler::createQueryExecution(Query &query)
// {

//     struct TriggerQueryHandlerExecution : public ThreadedQueryExecution
//     {
//         TriggerQueryHandlerExecution(Query &q, TriggerQueryHandler &h):
//             ThreadedQueryExecution(q), handler(h) {}

//         void processThreaded() override final { handler.handleTriggerQuery(query, *this); }

//     private:
//         TriggerQueryHandler &handler;
//     };

//     return new TriggerQueryHandlerExecution(query, *this);

// }
