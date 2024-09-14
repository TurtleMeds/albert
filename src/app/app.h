// Copyright (c) 2023-2024 Manuel Schneider

#pragma once
#include "appqueryhandler.h"
#include "extensionregistry.h"
#include "platform/signalhandler.h"
#include "pluginqueryhandler.h"
#include "pluginregistry.h"
#include "qtpluginprovider.h"
#include "queryengine.h"
#include "rpcserver.h"
#include "triggersqueryhandler.h"
#include <QObject>
#include <memory>
class PluginRegistry;
class QHotkey;
class QMenu;
class QSystemTrayIcon;
class QueryEngine;
class Session;
class SettingsWindow;
class Telemetry;
namespace albert {
class ExtensionRegistry;
class Frontend;
int run(int, char**);
}


class App : public QObject
{
    Q_OBJECT

public:

    albert::ExtensionRegistry &extensionRegistry();
    PluginRegistry &pluginRegistry();
    QueryEngine &queryEngine();

    void show(const QString &text);
    void hide();
    void toggle();
    void showSettings(QString plugin_id = {});

    bool trayEnabled() const;
    void setTrayEnabled(bool);

    bool telemetryEnabled() const;
    void setTelemetryEnabled(bool);
    QString displayableTelemetryReport() const;

    const QHotkey *hotkey() const;
    void setHotkey(std::unique_ptr<QHotkey> hotkey);

    albert::Frontend *frontend();
    void setFrontend(const QString &id);

private:

    friend int albert::run(int, char**);

    App(const QStringList &additional_plugin_paths, bool load_enabled);
    ~App() override;

    // void initialize(bool load_enabled);
    // void finalize();

    void initTrayIcon();
    void initTelemetry();
    void initHotkey();
    void initPRC();
    void initFrontend();
    bool loadFrontend(const QString &id);
    void notifyVersionChange();

    // Core
    RPCServer rpc_server_;  // Asap check for other instances
    SignalHandler signal_handler_;  // Asap
    albert::ExtensionRegistry extension_registry_;
    PluginRegistry plugin_registry_;
    QueryEngine query_engine_;
    QtPluginProvider plugin_provider_;
    albert::Frontend *frontend_;

    // Built-in Handlers
    AppQueryHandler app_query_handler_;
    PluginQueryHandler plugin_query_handler_;
    TriggersQueryHandler triggers_query_handler_;

    // Weak, lazy or optional
    std::unique_ptr<QHotkey> hotkey_;
    std::unique_ptr<Telemetry> telemetry_;
    std::unique_ptr<QSystemTrayIcon> tray_icon_;
    std::unique_ptr<QMenu> tray_menu_;
    std::unique_ptr<Session> session_;
    std::unique_ptr<SettingsWindow> settings_window_;

};
