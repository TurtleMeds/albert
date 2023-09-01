// Copyright (c) 2023 Manuel Schneider

#include "albert/extension/queryhandler/rankitem.h"
using namespace albert;
using namespace std;

template class std::vector<RankItem>;

RankItem::RankItem() = default;
RankItem::RankItem(RankItem &&) = default;
RankItem::RankItem(const RankItem &) = default;
RankItem &RankItem::operator=(RankItem &&) = default;
RankItem &RankItem::operator=(const RankItem &) = default;


RankItem::RankItem(shared_ptr<Item> i, float s):
    item(std::move(i)), score(s)
{}
