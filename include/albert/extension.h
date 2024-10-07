// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <QString>
#include <albert/export.h>

namespace albert
{

///
/// Abstract extension class.
///
/// Inherited by classes that want to join the extensions pool.
///
/// \sa ExtensionRegistry
///
class ALBERT_EXPORT Extension
{
public:
    ///
    /// Returns the extension identifier.
    ///
    /// @note To avoid naming conflicts use the namespace of your plugin, e.g. files (root
    /// extension), files.rootbrowser, files.homebrowser, …
    ///
    [[nodiscard]] virtual QString id() const = 0;

    ///
    /// Returns the pretty, human readable extension name.
    ///
    [[nodiscard]] virtual QString name() const = 0;

    ///
    /// Returns the brief extension description.
    ///
    [[nodiscard]] virtual QString description() const = 0;

protected:
    ///
    /// Destructs the extension.
    ///
    virtual ~Extension();

};

}

