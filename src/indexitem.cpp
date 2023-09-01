// Copyright (c) 2023 Manuel Schneider

#include "albert/extension/queryhandler/indexitem.h"
using namespace albert;
using namespace std;

template class std::vector<IndexItem>;

IndexItem::IndexItem() = default;
IndexItem::IndexItem(IndexItem &&) = default;
IndexItem::IndexItem(const IndexItem &) = default;
IndexItem &IndexItem::operator=(IndexItem &&) = default;
IndexItem &IndexItem::operator=(const IndexItem &) = default;

IndexItem::IndexItem(shared_ptr<Item> i, QString s):
    item(::move(i)), string(::move(s))
{}
