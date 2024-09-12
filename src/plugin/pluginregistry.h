// Copyright (c) 2023-2024 Manuel Schneider

#pragma once
#include "plugin.h"
#include <QObject>
#include <QString>
#include <map>
#include <set>
namespace albert {
class Extension;
class ExtensionRegistry;
class PluginLoader;
class PluginInstance;
class PluginProvider;
class PluginMetaData;
}


class PluginRegistry : public QObject
{
    Q_OBJECT

public:

    PluginRegistry(albert::ExtensionRegistry&);
    ~PluginRegistry();

    /// Get map of all registered plugins
    const std::map<QString, Plugin> &plugins() const;

    /// Automatically load enabled plugins.
    inline bool autoloadEnabledPlugins() const;

    /// Set autoloadEnabled.
    void setAutoloadEnabledPlugins(bool enabled);

    /// Enable/Disable a plugin and its transitive dependencies/dependees.
    /// If loadEnabled is set to true, load/unload the plugins.
    /// @throws std::out_of_range if `id` does not exist.
    void setEnabled(const QString &id, bool enable);

    /// Load a plugin and its transitive dependencies.
    /// @throws std::out_of_range if `id` does not exist.
    void load(const QString &id);

    /// Unload a plugin and its transitive dependees.
    /// @throws std::out_of_range if `id` does not exist.
    void unload(const QString &id);

private:

    std::map<Plugin*,QString> load(std::vector<Plugin*>);
    std::map<Plugin*,QString> unload(std::vector<Plugin*>);

    void onRegistered(albert::Extension*);
    void onDeregistered(albert::Extension*);

    albert::ExtensionRegistry &extension_registry_;
    std::set<albert::PluginProvider*> plugin_providers_;
    std::map<QString, Plugin> plugins_;
    bool autoload_enabled_plugins_{false};

signals:

    void pluginsChanged();
    void pluginEnabledChanged(const QString &id, bool enabled);
    void pluginStateChanged(const QString &id, Plugin::State state, QString info);

};
