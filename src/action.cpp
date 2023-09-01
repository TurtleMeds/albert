// Copyright (c) 2023 Manuel Schneider

#include "albert/extension/queryhandler/action.h"
using namespace albert;

template class std::vector<Action>;

Action::Action() = default;
Action::Action(Action &&) = default;
Action::Action(const Action &) = default;
Action &Action::operator=(Action &&) = default;
Action &Action::operator=(const Action &) = default;

Action::Action(QString i, QString t, std::function<void()> f):
    id(std::move(i)), text(std::move(t)), function(std::move(f))
{}

