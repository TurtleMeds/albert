// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <QString>
#include <albert/export.h>
#include <albert/extension.h>
#include <albert/item.h>
#include <memory>
#include <vector>

namespace albert
{
class Item;

///
/// Abstract fallback item provider.
///
class ALBERT_EXPORT FallbackHandler : virtual public Extension
{
public:
    ///
    /// Returns fallback items for `query`.
    ///
    virtual std::vector<std::shared_ptr<Item>> fallbacks(const QString &query) const = 0;

protected:
    ///
    /// Destructs the fallback handler.
    ///
    ~FallbackHandler() override;

};

}
