// Copyright (c) 2023-2024 Manuel Schneider

#pragma once
#include <QObject>
#include <memory>
class PluginRegistry;
class QueryEngine;
class QHotkey;
namespace albert {
class Frontend;
class ExtensionRegistry;
int run(int, char**);
}


class App : public QObject
{
    Q_OBJECT

public:

    static App *instance();

    void show(const QString &text);
    void hide();
    void toggle();
    void restart();
    void quit();

    albert::ExtensionRegistry &extensionRegistry();
    PluginRegistry &pluginRegistry();
    QueryEngine &queryEngine();

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

    explicit App(const QStringList &additional_plugin_paths);
    ~App() override;

    void initialize(bool load_enabled);
    void finalize();

    friend int albert::run(int, char**);

    class Private;
    std::unique_ptr<Private> d;

};
