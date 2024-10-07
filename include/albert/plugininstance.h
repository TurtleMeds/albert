// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <QString>
#include <albert/config.h>
#include <albert/export.h>
#include <filesystem>
#include <memory>
#include <vector>
class PluginInstancePrivate;
class QSettings;
class QWidget;

namespace albert
{
class Extension;
class PluginLoader;

///
/// Abstract plugin instance class.
///
/// The class every plugin has to inherit.
///
class ALBERT_EXPORT PluginInstance
{
public:

    ///
    /// Creates a widget that can be used to configure the plugin properties.
    ///
    /// The caller takes ownership of the returned object.
    ///
    virtual QWidget *buildConfigWidget();

    ///
    /// Returns the extensions provided by this plugin.
    ///
    /// The caller does not take ownership of the returned objects.
    ///
    virtual std::vector<albert::Extension*> extensions();

public:

    ///
    /// Returns the loader of this plugin.
    ///
    [[nodiscard]] const PluginLoader &loader() const;

    ///
    /// Returns the recommended cache location for this plugin.
    ///
    [[nodiscard]] std::filesystem::path cacheLocation() const;

    ///
    /// Returns the recommended config location for this plugin.
    ///
    [[nodiscard]] std::filesystem::path configLocation() const;

    ///
    /// Returns the recommended data location for this plugin.
    ///
    [[nodiscard]] std::filesystem::path dataLocation() const;

    ///
    /// Creates a preconfigured QSettings object for plugin config data.
    ///
    /// Configured to use the group <plugin-id> in albert::config().
    ///
    [[nodiscard]] std::unique_ptr<QSettings> settings() const;

    ///
    /// Creates a preconfigured QSettings object for plugin state data.
    ///
    /// Configured to use the group <plugin-id> in albert::state().
    ///
    [[nodiscard]] std::unique_ptr<QSettings> state() const;

protected:

    ///
    /// Constructs a plugin instance.
    ///
    PluginInstance();

    ///
    /// Destructs a plugin instance.
    ///
    virtual ~PluginInstance();

private:

    std::unique_ptr<PluginInstancePrivate> d;

};

}

///
/// @brief Declare a class as native Albert plugin.
///
/// Sets the interface identifier to #ALBERT_PLUGIN_IID and uses the metadata
/// file named 'metadata.json' located at CMAKE_CURRENT_SOURCE_DIR.
///
/// @note This macro has to be put into the plugin class body.
/// The class this macro appears on must be default-constructible, inherit
/// QObject and contain the Q_OBJECT macro. There should be exactly one
/// occurrence of this macro in the source code for a plugin.
///
#define ALBERT_PLUGIN Q_OBJECT Q_PLUGIN_METADATA(IID ALBERT_PLUGIN_IID FILE "metadata.json")
