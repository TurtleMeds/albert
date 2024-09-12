// Copyright (c) 2024 Manuel Schneider

#pragma once
#include <QObject>
#include <QString>
#include <set>
class QWidget;
class PluginRegistry;
namespace albert {
class PluginInstance;
class PluginLoader;
class PluginMetaData;
class PluginProvider;
}


class Plugin : public QObject
{
    Q_OBJECT

public:

    Plugin(albert::PluginProvider&, albert::PluginLoader&) noexcept;
    ~Plugin();

    const albert::PluginProvider &provider;
    albert::PluginLoader &loader;
    std::set<Plugin*> dependencies;
    std::set<Plugin*> dependees;
    uint load_order;

    enum State {
        Unloaded,
        Loading,
        Loaded,
        Unloading,
    } state;
    QString state_info;
    bool enabled;
    albert::PluginInstance *instance;

    static QString localizedStateString(State);

    // Convenience functions
    QString path() const;
    const albert::PluginMetaData &metaData() const;
    const QString &id() const;
    bool isUser() const;
    bool isFrontend() const;
    QWidget *buildConfigWidget() const;
    std::set<Plugin*> transitiveDependencies() const;
    std::set<Plugin*> transitiveDependees() const;

    void setEnabled(bool);
    void setState(Plugin::State state, QString info = {});

    QString load();
    QString unload();

signals:

    void enabledChanged(bool);
    void stateChanged(Plugin::State, QString);

};
