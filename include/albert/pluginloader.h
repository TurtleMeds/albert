// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <QString>
#include <albert/export.h>

namespace albert
{
class PluginInstance;
class PluginMetaData;

///
/// Plugin loader interface class.
///
class ALBERT_EXPORT PluginLoader
{
public:

    ///
    /// The path to the plugin.
    ///
    /// @return @copybrief path
    ///
    [[nodiscard]] virtual QString path() const noexcept = 0;

    ///
    /// The plugin metadata.
    ///
    /// @return @copybrief metaData
    ///
    [[nodiscard]] virtual const PluginMetaData &metaData() const noexcept = 0;

    ///
    /// Load the plugin and instantiate the plugin instance.
    ///
    /// On exceptions the plugin is expected to be in unloaded state.
    /// On success instance() returns a valid weak reference to the PluginInstance.
    ///
    /// @throw std::exception in case of errors.
    /// @return A weak reference to the PluginInstance.
    ///
    [[nodiscard]] virtual PluginInstance &load() = 0;

    ///
    /// Delete the instance and unload the plugin.
    ///
    /// On exceptions the plugin is expected to be in unloaded state.
    /// Invalidates any former references to the instance.
    ///
    /// @throw std::exception in case of errors.
    ///
    virtual void unload() = 0;

protected:

    virtual ~PluginLoader();

};

}
