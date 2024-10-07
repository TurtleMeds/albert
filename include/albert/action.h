// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <QString>
#include <albert/export.h>
#include <functional>

namespace albert
{

class ALBERT_EXPORT Action final
{
public:

    /// Constructs an Action with the contents initialized with the data passed.
    /// \param id @copybrief Action::id
    /// \param text @copybrief Action::text
    /// \param function @copybrief Action::function
    Action(QString id, QString text, std::function<void()> function) noexcept;

    /// The action identifier.
    QString id;

    /// The action title.
    QString text;

    /// The action function executed on activation.
    std::function<void()> function;
};

}
