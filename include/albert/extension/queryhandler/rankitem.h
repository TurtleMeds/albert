// Copyright (c) 2023 Manuel Schneider

#pragma once
#include "albert/extension/queryhandler/item.h"
#include <memory>
#include <vector>

namespace albert
{

/// Scored item
/// Used to rank item results of mutliple handlers
class ALBERT_EXPORT RankItem
{
public:

    // Required for ETI. See https://stackoverflow.com/questions/69556553/how-to-create-an-explicit-template-instantiation-declaration-for-stdvector-wit
    RankItem();
    RankItem(const RankItem&);
    RankItem(RankItem&&);
    RankItem &operator=(const RankItem&);
    RankItem &operator=(RankItem&&);

    /// \param item @copydoc item
    /// \param score @copydoc score
    explicit RankItem(std::shared_ptr<Item> item, float score);

    /// The matched item
    std::shared_ptr<Item> item;

    /// The match score. Must be in the range (0,1]. Not checked for performance.
    float score;
};

}

// Instanciate common templates explicitly to reduce binary sizes
extern template class ALBERT_EXPORT std::vector<albert::RankItem>;
