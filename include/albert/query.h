// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <QString>
#include <albert/export.h>

namespace albert
{

/// The global query execution interface class.
class ALBERT_EXPORT Query
{
public:
    /// Returns the trigger of this query.
    virtual QString trigger() const = 0;

    /// Returns the query string.
    virtual QString string() const = 0;

    /// Returns `true` if the query has been cancelled or invalidated.
    virtual bool isValid() const = 0;

protected:

    /// Destructs the query.
    virtual ~Query();
};

}  // namespace albert
