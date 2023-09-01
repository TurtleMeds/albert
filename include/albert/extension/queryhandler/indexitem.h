// Copyright (c) 2023 Manuel Schneider

#pragma once
#include "albert/extension/queryhandler/item.h"
#include <QString>
#include <memory>

namespace albert
{

/// The eligible for the internal index of IndexQueryHandler
/// @see IndexQueryHandler
class ALBERT_EXPORT IndexItem
{
public:

    // Required for ETI. See https://stackoverflow.com/questions/69556553/how-to-create-an-explicit-template-instantiation-declaration-for-stdvector-wit
    IndexItem();
    IndexItem(const IndexItem&);
    IndexItem(IndexItem&&);
    IndexItem &operator=(const IndexItem&);
    IndexItem &operator=(IndexItem&&);

    /// \param item @copydoc item
    /// \param string @copydoc string
    IndexItem(std::shared_ptr<Item> item, QString string);

    /// The item to be indexed
    std::shared_ptr<Item> item;

    /// The corresponding lookup string
    QString string;
};

}

// Instanciate common templates explicitly to reduce binary sizes
extern template class ALBERT_EXPORT std::vector<albert::IndexItem>;
