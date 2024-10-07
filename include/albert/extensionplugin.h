// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <QObject>
#include <albert/extension.h>
#include <albert/plugininstance.h>

namespace albert
{

///
/// Extension plugin base class.
///
/// Implements functions common to extension plugins.
///
class ALBERT_EXPORT ExtensionPlugin : public QObject,
                                      public PluginInstance,
                                      virtual public Extension
{
public:
    ///
    /// Returns the id from the plugin metadata.
    ///
    [[nodiscard]] QString id() const override;

    ///
    /// Returns the name from the plugin metadata.
    ///
    [[nodiscard]] QString name() const override;

    ///
    /// Returns the description from the plugin metadata.
    ///
    [[nodiscard]] QString description() const override;

    ///
    /// Returns this extension.
    ///
    [[nodiscard]] std::vector<albert::Extension*> extensions() override;

};

}
