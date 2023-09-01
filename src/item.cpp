// Copyright (c) 2023 Manuel Schneider

#include "albert/extension/queryhandler/item.h"
using namespace albert;

template class std::shared_ptr<Item>;
template class std::vector<std::shared_ptr<Item>>;

QString Item::inputActionText() const { return {}; }

std::vector<Action> Item::actions() const { return {}; }
