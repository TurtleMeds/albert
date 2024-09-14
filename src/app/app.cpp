// Copyright (c) 2023-2024 Manuel Schneider

#include "albert.h"
#include "app.h"
#include "frontend.h"
#include "iconprovider.h"
#include "logging.h"
#include "messagehandler.h"
#include "platform.h"
#include "plugininstance.h"
#include "pluginswidget.h"
#include "querywidget.h"
#include "report.h"
#include "session.h"
#include "settingswindow.h"
#include "telemetry.h"
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QHotkey>
#include <QLibraryInfo>
#include <QMenu>
#include <QMessageBox>
#include <QObject>
#include <QPointer>
#include <QSettings>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QTranslator>
#include <ranges>
Q_LOGGING_CATEGORY(AlbertLoggingCategory, "albert")
using namespace albert;
using namespace std;

App * app{nullptr};  // extern in albert.cpp

namespace {
static const char *STATE_LAST_USED_VERSION = "last_used_version";
static const char *CFG_FRONTEND_ID = "frontend";
static const char *DEF_FRONTEND_ID = "widgetsboxmodel";
static const char* CFG_SHOWTRAY = "showTray";
static const bool  DEF_SHOWTRAY = true;
static const char *CFG_HOTKEY = "hotkey";
static const char *DEF_HOTKEY = "Ctrl+Space";
static const char *CFG_TELEMETRY = "telemetry";
}


App::App(const QStringList &additional_plugin_paths, bool load_enabled):
    plugin_registry_(extension_registry_),
    query_engine_(extension_registry_),
    plugin_provider_(additional_plugin_paths),
    plugin_query_handler_(plugin_registry_),
    triggers_query_handler_(query_engine_)
{
    if (app)
        qFatal("No multiple app instances allowed");
    else
        app = this;

    platform::initPlatform();

    extension_registry_.registerExtension(&app_query_handler_);
    extension_registry_.registerExtension(&plugin_query_handler_);
    extension_registry_.registerExtension(&triggers_query_handler_);
    extension_registry_.registerExtension(&plugin_provider_);  // register plugins

    initFrontend();

    platform::initNativeWindow(frontend_->winId());

    // Invalidate sessions on handler removal or visibility change
    auto reset_session = [this]{
        session_.reset();
        if (frontend_->isVisible())
            session_ = make_unique<Session>(query_engine_, *frontend_);
    };

    connect(frontend_, &Frontend::visibleChanged,
            app, reset_session);

    connect(&query_engine_, &QueryEngine::handlerRemoved,
            app, reset_session);


    if (settings()->value(CFG_SHOWTRAY, DEF_SHOWTRAY).toBool())
        initTrayIcon();

    notifyVersionChange();

    initTelemetry();

    initPRC(); // Also may trigger frontend

    initHotkey();  // Connect hotkey after! frontend has been loaded else segfaults

    // Load plugins not before loop is executing
    QTimer::singleShot(0, this, [this, load_enabled]{
        plugin_registry_.setAutoloadEnabledPlugins(load_enabled);  // loads plugins
    });
}

App::~App()
{
    frontend_->disconnect();
    query_engine_.disconnect();

    if (hotkey_)
    {
        hotkey_.get()->disconnect();
        hotkey_->setRegistered(false);
    }

    delete settings_window_.get();
    session_.reset();

    extension_registry_.deregisterExtension(&plugin_provider_);  // unloads plugins
    extension_registry_.deregisterExtension(&triggers_query_handler_);
    extension_registry_.deregisterExtension(&plugin_query_handler_);
    extension_registry_.deregisterExtension(&app_query_handler_);
};

void App::initTrayIcon()
{
    // menu

    tray_menu_ = make_unique<QMenu>();

    auto *action = tray_menu_->addAction(tr("Show/Hide"));
    connect(action, &QAction::triggered, this, []{ app->toggle(); });

    action = tray_menu_->addAction(tr("Settings"));
    connect(action, &QAction::triggered, this, []{ app->showSettings(); });

    action = tray_menu_->addAction(tr("Open website"));
    connect(action, &QAction::triggered, []{ openWebsite(); });

    tray_menu_->addSeparator();

    action = tray_menu_->addAction(tr("Restart"));
    connect(action, &QAction::triggered, []{ restart(); });

    action = tray_menu_->addAction(tr("Quit"));
    connect(action, &QAction::triggered, []{ quit(); });

    // icon

    auto icon = albert::iconFromUrls({"xdg:albert-tray", "xdg:albert", ":app_tray_icon"});
    icon.setIsMask(true);

    tray_icon_ = make_unique<QSystemTrayIcon>();
    tray_icon_->setIcon(icon);
    tray_icon_->setContextMenu(tray_menu_.get());
    tray_icon_->setVisible(true);

#ifndef Q_OS_MAC
    // Some systems open menus on right click, show albert on left trigger
    connect(tray_icon.get(), &QSystemTrayIcon::activated,
            [](QSystemTrayIcon::ActivationReason reason)
    {
        if( reason == QSystemTrayIcon::ActivationReason::Trigger)
            App::instance()->toggle();
    });
#endif
}

void App::initTelemetry()
{
    if (auto s = settings(); !s->contains(CFG_TELEMETRY))
    {
        auto text = tr("Albert collects anonymous data to enhance user experience. "
                       "You can review the data to be sent in the details. Opt in?");

        QMessageBox mb(QMessageBox::Question, qApp->applicationDisplayName(),
                       text, QMessageBox::No|QMessageBox::Yes);

        mb.setDefaultButton(QMessageBox::Yes);
        mb.setDetailedText(telemetry_->buildReportString());
        s->setValue(CFG_TELEMETRY, mb.exec() == QMessageBox::Yes);
    }
    else if (s->value(CFG_TELEMETRY).toBool())
        telemetry_ = make_unique<Telemetry>();
}

void App::initHotkey()
{
    if (!QHotkey::isPlatformSupported())
    {
        INFO << "Hotkeys are not supported on this platform.";
        return;
    }

    auto s_hk = settings()->value(CFG_HOTKEY, DEF_HOTKEY).toString();
    auto kc_hk = QKeySequence::fromString(s_hk)[0];

    if (auto hk = make_unique<QHotkey>(kc_hk);
        hk->setRegistered(true))
    {
        hotkey_ = ::move(hk);
        connect(hotkey_.get(), &QHotkey::activated, frontend_, []{ app->toggle(); });
        INFO << "Hotkey set to" << s_hk;
    }
    else
    {
        auto t = QT_TR_NOOP("Failed to set the hotkey '%1'");
        WARN << QString(t).arg(s_hk);
        QMessageBox::warning(nullptr, qApp->applicationDisplayName(),
                             tr(t).arg(QKeySequence(kc_hk)
                                       .toString(QKeySequence::NativeText)));
        showSettings();
    }
}

void App::initPRC()
{
    std::map<QString, RPCServer::RPC> rpc =
    {
        {"show",     [](const QString &t){ app->show(t); return "Albert set visible."; }},
        {"hide",     [](const QString &) { app->hide(); return "Albert set hidden.";}},
        {"toggle",   [](const QString &) { app->toggle(); return "Albert visibility toggled."; }},
        {"settings", [](const QString &t){ app->showSettings(t); return "Settings opened,"; }},
        {"restart",  [](const QString &) { restart(); return "Triggered restart."; }},
        {"quit",     [](const QString &) { quit(); return "Triggered quit."; }},
        {"report",   [](const QString &) { return report().join('\n'); }}
    };

    rpc_server_.setPRC(::move(rpc));
}

void App::initFrontend()
{
    // Load configured frontend or default if unset
    if (loadFrontend(settings()->value(CFG_FRONTEND_ID, DEF_FRONTEND_ID).toString()))
        return;

    // Load any frontend
    auto frontend_plugins = plugin_registry_.plugins()
            | views::values
            | views::filter([](auto &p){ return p.isFrontend(); });

    for (const auto &plugin : frontend_plugins)
        if (loadFrontend(plugin.id()))
            return;

    qFatal("Could not load any frontend.");
}

bool App::loadFrontend(const QString &id)
{
    try {
        auto &p = plugin_registry_.plugins().at(id);
        DEBG << QString("Loading frontend '%1'.").arg(id);
        plugin_registry_.load(id);
        if (p.state != Plugin::State::Loaded)
            WARN << QString("Failed loading frontend '%1': %2").arg(p.id(), p.state_info);
        else if (frontend_ = dynamic_cast<Frontend*>(p.instance); !frontend_)
            WARN << QString("Failed casting Plugin instance to albert::Frontend: %1").arg(p.id());
        else
            return true;
    } catch (const out_of_range&) {
        WARN << QString("Frontend plugin '%1' does not exist.").arg(id);
    }
    return false;
}

void App::notifyVersionChange()
{
    auto s = state();
    auto current_version = qApp->applicationVersion();
    auto last_used_version = s->value(STATE_LAST_USED_VERSION).toString();

    // First run
    if (last_used_version.isNull())
    {
        auto text = tr("This is the first time you've launched Albert. Albert is "
                       "plugin based. You have to enable some plugins you want to use.");

        QMessageBox::information(nullptr, qApp->applicationDisplayName(), text);

        showSettings();
    }
    else if (current_version.section('.', 1, 1) != last_used_version.section('.', 1, 1) )  // FIXME in first major version
    {
        auto text = tr("You are now using Albert %1. The major version changed. "
                       "Some parts of the API might have changed. "
                       "Check the <a href=\"https://albertlauncher.github.io/news/\">news</a>."
                       ).arg(current_version);

        QMessageBox::information(nullptr, qApp->applicationDisplayName(), text);
    }

    if (last_used_version != current_version)
        s->setValue(STATE_LAST_USED_VERSION, current_version);
}

ExtensionRegistry &App::extensionRegistry() { return extension_registry_; }

PluginRegistry &App::pluginRegistry() { return plugin_registry_; }

QueryEngine &App::queryEngine() { return query_engine_; }

void App::show(const QString &text)
{
    if (!text.isNull())
        frontend_->setInput(text);
    frontend_->setVisible(true);
}

void App::hide() { frontend_->setVisible(false); }

void App::toggle() { frontend_->setVisible(!frontend_->isVisible()); }

void App::showSettings(QString plugin_id)
{
    if (!settings_window_)
    {
        settings_window_ = make_unique<SettingsWindow>(*this);
        connect(settings_window_.get(), &SettingsWindow::destroyed,
                this, [this]{ settings_window_.release(); });  // Deletes on close
    }
    hide();
    settings_window_->bringToFront(plugin_id);
}

bool App::trayEnabled() const { return tray_icon_.get(); }

void App::setTrayEnabled(bool enable)
{
    if (enable && !trayEnabled())
        initTrayIcon();
    else if (!enable && trayEnabled()) {
        tray_icon_.reset();
        tray_menu_.reset();
    }
    else
        return;
    settings()->setValue(CFG_SHOWTRAY, enable);
}

bool App::telemetryEnabled() const { return telemetry_.get(); }

void App::setTelemetryEnabled(bool enable)
{
    if (enable && !telemetryEnabled())
        telemetry_ = make_unique<Telemetry>();
    else if (!enable && telemetryEnabled())
        telemetry_.reset();
    else
        return;
    settings()->setValue(CFG_TELEMETRY, enable);
}

QString App::displayableTelemetryReport() const
{ return telemetry_ ? telemetry_->buildReportString() : QString(); }

const QHotkey *App::hotkey() const { return hotkey_.get(); }

void App::setHotkey(unique_ptr<QHotkey> hk)
{
    if (!hk)
    {
        hotkey_.reset();
        settings()->remove(CFG_HOTKEY);
    }
    else if (hk->isRegistered())
    {
        hotkey_ = ::move(hk);
        connect(hotkey_.get(), &QHotkey::activated, frontend_, [this]{ toggle(); });
        settings()->setValue(CFG_HOTKEY, hotkey_->shortcut().toString());
    }
    else
        WARN << "Set unregistered hotkey. Ignoring.";
}

Frontend *App::frontend() { return frontend_; }

void App::setFrontend(const QString &id)
{
    settings()->setValue(CFG_FRONTEND_ID, id);

    auto text = tr("Changing the frontend requires a restart. "
                   "Do you want to restart Albert?");

    if (QMessageBox::question(nullptr, qApp->applicationDisplayName(), text) == QMessageBox::Yes)
        restart();
}


int ALBERT_EXPORT albert::run(int argc, char **argv)
{
    if (app != nullptr)
        qFatal("Calling main twice is not allowed.");

    QLoggingCategory::setFilterRules("*.debug=false");
    qInstallMessageHandler(messageHandler);


    // Put /usr/local/bin hardcoded to env… why?

    // {
    //     auto usr_local_bin = QStringLiteral("/usr/local/bin");
    //     auto PATHS = QString(qgetenv("PATH")).split(':');
    //     if (!PATHS.contains(usr_local_bin))
    //         PATHS.prepend(usr_local_bin);
    //     auto PATH = PATHS.join(':').toLocal8Bit();
    //     qputenv("PATH", PATH);
    // }


    // Set locale from env vars (for macos debug builds)

    // if (const char *key = "LANGUAGE"; qEnvironmentVariableIsSet(key))
    //     QLocale::setDefault(QLocale(qEnvironmentVariable(key)));
    // else if (key = "LANG"; qEnvironmentVariableIsSet(key))
    //     QLocale::setDefault(QLocale(qEnvironmentVariable(key)));


    // Initialize Qt application

    QApplication qapp(argc, argv);
    QApplication::setApplicationName("albert");
    QApplication::setApplicationDisplayName("Albert");
    QApplication::setApplicationVersion(ALBERT_VERSION_STRING);
    QApplication::setWindowIcon(iconFromUrls({"xdg:albert", "qrc:app_icon"}));
    QApplication::setQuitOnLastWindowClosed(false);


    // Parse command line (asap for fast cli commands)

    struct {
        QStringList plugin_dirs;
        bool autoload;
    } config;

    {
        auto opt_p = QCommandLineOption({"p", "plugin-dirs"},
                                        App::tr("Set the plugin dirs to use. Comma separated."),
                                        App::tr("directories"));
        auto opt_r = QCommandLineOption({"r", "report"},
                                        App::tr("Print report and quit."));
        auto opt_n = QCommandLineOption({"n", "no-autoload"},
                                        App::tr("Do not implicitly load enabled plugins."));

        QCommandLineParser parser;
        parser.addOptions({opt_p, opt_r, opt_n});
        parser.addPositionalArgument(App::tr("command"),
                                     App::tr("RPC command to send to the running instance."),
                                     App::tr("[command [params...]]"));
        parser.addVersionOption();
        parser.addHelpOption();
        parser.setApplicationDescription(App::tr("Launch Albert or control a running instance."));
        parser.process(qapp);

        if (parser.isSet(opt_r))
            printReportAndExit();

        if (auto args = parser.positionalArguments(); !args.isEmpty())
            return RPCServer::trySendMessage(args.join(" ")) ? 0 : 1;

        config = {
            .plugin_dirs = parser.value(opt_p).split(',', Qt::SkipEmptyParts),
            .autoload    = !parser.isSet(opt_n),
        };
    }


    // Initialize app directories

    for (const auto &path : { cacheLocation(), configLocation(), dataLocation() })
        try {
            tryCreateDirectory(path);
            QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
        } catch (...) {
            qFatal("Failed creating config dir at: %s", path.c_str());
        }


    // Section for ports

    {
        // Move old config file to new location TODO: Remove from 0.26 on

        {
            auto conf_loc = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
            auto old_conf_loc = QDir(conf_loc).filePath("albert.conf");
            QFile config_file(old_conf_loc);
            if (config_file.exists())
            {
                auto new_conf_loc = QDir(configLocation()).filePath("config");
                if (config_file.rename(new_conf_loc))
                    INFO << "Config file successfully moved to new location.";
                else
                    qFatal("Failed to move config file to new location. "
                           "Please move the file at %s to %s manually.",
                           old_conf_loc.toUtf8().data(), new_conf_loc.toUtf8().data());
            }
        }

        // Merge settings sections of applications plugins

        {
            auto s = settings();
            auto groups = s->childGroups();

            for (const char *old_group : { "applications_macos", "applications_xdg"})
            {
                if (groups.contains(old_group))
                {
                    s->beginGroup(old_group);
                    auto child_keys = s->childKeys();
                    s->endGroup();

                    for (const QString &child_key : child_keys)
                    {
                        auto old_key = QString("%1/%2").arg(old_group, child_key);

                        s->setValue(QString("applications/%1").arg(child_key),
                                    s->value(old_key));

                        s->remove(old_key);
                    }
                }
            }
        }
    }


    // Load translations

    DEBG << "Loading translations";

    auto *t = new QTranslator(&qapp);
    if (t->load(QLocale(), "qtbase", "_", QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        qapp.installTranslator(t);
    else
        delete t;

    t = new QTranslator(&qapp);
    if (t->load(QLocale(), qapp.applicationName(), "_", ":/i18n"))
        qapp.installTranslator(t);
    else
        delete t;


    // Run app

    try {
        app = new App(config.plugin_dirs, config.autoload);
        int return_value = qapp.exec();
        delete app;

        if (return_value == -1 && runDetachedProcess(qApp->arguments(), QDir::currentPath()))
            return_value = EXIT_SUCCESS;

        INFO << "Bye.";
        return return_value;
    } catch (const std::exception &e) {
        CRIT << "Uncaught exception in main: " << e.what();
        return EXIT_FAILURE;
    } catch (...) {
        CRIT << "Uncaught unknown exception in main. Exiting.";
        return EXIT_FAILURE;
    }
}
