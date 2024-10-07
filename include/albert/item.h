// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <QStringList>
#include <albert/action.h>
#include <albert/export.h>
#include <vector>

namespace albert
{
class ALBERT_EXPORT Item
{
public:

    /// Destructs the item.
    virtual ~Item();

    ///
    /// Returns the item identifier.
    ///
    /// Has to be unique per extension. This function is involved in several time critical
    /// operartions such as indexing and sorting. Return a string that is as short as possible as
    /// fast as possible.
    ///
    [[nodiscard]] virtual QString id() const = 0;

    ///
    /// Returns the item text.
    ///
    /// Primary text displayed emphasized in a list item. The string must not be empty, since the
    /// text length is used as divisor for scoring. Return as fast as possible. No checks are
    /// performed. If empty you get undefined behavior.
    ///
    [[nodiscard]] virtual QString text() const = 0;

    ///
    /// Returns the item subtext.
    ///
    /// Secondary descriptive text displayed in a list item.
    ///
    [[nodiscard]] virtual QString subtext() const = 0;

    ///
    /// Returns the item icon urls.
    ///
    /// Used to get the item icon using the IconProvider.
    ///
    [[nodiscard]] virtual QStringList iconUrls() const = 0;

    ///
    /// Returns the item input action text.
    ///
    /// Used as input text replacement (usually by pressing Tab).
    ///
    [[nodiscard]] virtual QString inputActionText() const;

    ///
    /// Returns the item actions.
    ///
    /// These are the actions a users can choose to activate.
    ///
    [[nodiscard]] virtual std::vector<Action> actions() const;
};

}  // namespace albert
