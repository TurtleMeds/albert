// Copyright (c) 2022-2024 Manuel Schneider

#include "albert.h"
#include "app.h"
#include "frontend.h"
#include "logging.h"
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QGuiApplication>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <memory>
using namespace std;


void albert::show(const QString &input_text)
{
    if (!input_text.isNull())
        App::instance()->frontend()->setInput(input_text);
    App::instance()->frontend()->setVisible(true);
}

void albert::restart()
{ QMetaObject::invokeMethod(qApp, "exit", Qt::QueuedConnection, Q_ARG(int, -1)); }

void albert::quit()
{ QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection); }

QNetworkAccessManager &albert::network()
{
    static thread_local QNetworkAccessManager network_manager;
    return network_manager;
}

inline static filesystem::path getFilesystemPath(QStandardPaths::StandardLocation loc)
{ return filesystem::path(QStandardPaths::writableLocation(loc).toStdString()); }

filesystem::path albert::cacheLocation()
{ return getFilesystemPath(QStandardPaths::CacheLocation); }

filesystem::path albert::configLocation()
{ return getFilesystemPath(QStandardPaths::AppConfigLocation); }

filesystem::path albert::dataLocation()
{ return getFilesystemPath(QStandardPaths::AppDataLocation); }

inline static unique_ptr<QSettings> settingsFromPath(const filesystem::path &path)
{ return make_unique<QSettings>(path.string().data(), QSettings::IniFormat); }

unique_ptr<QSettings> albert::settings()
{ return settingsFromPath(configLocation() / "config"); }

unique_ptr<QSettings> albert::state()
{ return settingsFromPath(cacheLocation() / "state"); }

void albert::showSettings(QString plugin_id)
{ App::instance()->showSettings(plugin_id); }

void albert::openUrl(const QString &url)
{
    if (QUrl qurl(url); qurl.isValid())
        open(QUrl(url));
    else
        WARN << "Invalid URL" << url << qurl.errorString();
}

void albert::open(const QUrl &url)
{
    DEBG << QString("Open URL '%1'").arg(url.toString());

    if (qApp->platformName() == "wayland")
        runDetachedProcess({"xdg-open", url.toString()});
    else if (!QDesktopServices::openUrl(url))
        WARN << "Failed to open URL" << url;
}

void albert::open(const QString &path) { open(QUrl::fromLocalFile(path)); }

void albert::open(const string &path) { open(QString::fromStdString(path)); }

void albert::openWebsite()
{
    static const char *website_url = "https://albertlauncher.github.io/";
    QDesktopServices::openUrl(QUrl(website_url));
}

void albert::setClipboardText(const QString &text)
{
    QGuiApplication::clipboard()->setText(text, QClipboard::Clipboard);
    QGuiApplication::clipboard()->setText(text, QClipboard::Selection);
}

static bool checkPasteSupport()
{
#if defined Q_OS_MACOS
    return !QStandardPaths::findExecutable("osascript").isEmpty();
#elif defined(Q_OS_UNIX)
    bool xdotool = !QStandardPaths::findExecutable("xdotool").isEmpty();
    bool wtype = qApp->platformName() == "wayland" && !QStandardPaths::findExecutable("wtype").isEmpty();
    bool wlrctl = qApp->platformName() == "wayland" && !QStandardPaths::findExecutable("wlrctl").isEmpty();
    bool ydotool = !QStandardPaths::findExecutable("ydotool").isEmpty();
    bool have_paste_support = xdotool || wtype || wlrctl || ydotool;
    if(!have_paste_support)
        WARN << "neither xdotool or wtype are available. No paste support.";
    else if(qgetenv("XDG_SESSION_TYPE") != "x11" && !ydotool && !wtype)
        WARN << "xdotool is available but but session type is not x11. "
                "Unless your compositor supports libei, "
                "Paste will work for X11 windows only. "
                "Please install ydotool, wlrctl or wtype.";
    return have_paste_support;
#endif
}

bool albert::havePasteSupport()
{
    static bool have_paste_support = checkPasteSupport();
    return have_paste_support;
}

void albert::setClipboardTextAndPaste(const QString &text)
{
    setClipboardText(text);
    if (!havePasteSupport())
    {
        auto t = "Received a request to paste, although the feature is not supported. "
                 "Looks like the plugin did not check for feature support before. "
                 "Please report this issue.";
        WARN << t;
        QMessageBox::warning(nullptr, qApp->applicationDisplayName(), t);
        return;
    }

#if defined(Q_OS_MACOS)
    runDetachedProcess({
        "osascript", "-e",
        R"(tell application "System Events" to keystroke "v" using command down)"
    });
#elif defined(Q_OS_UNIX)
    QApplication::processEvents(); // ??
    auto *proc = new QProcess;
    bool xdotool = !QStandardPaths::findExecutable("xdotool").isEmpty();
    bool ydotool = !QStandardPaths::findExecutable("ydotool").isEmpty();
    bool wtype = qApp->platformName() == "wayland" && !QStandardPaths::findExecutable("wtype").isEmpty();
    bool wlrctl = qApp->platformName() == "wayland" && !QStandardPaths::findExecutable("wlrctl").isEmpty();
    if (wtype) {
        proc->start("sh" , {"-c", "sleep 0.1 && wtype -M ctrl v"});
    } else if (wlrctl) {
        proc->start("sh" , {"-c", "sleep 0.1 && wlrctl keyboard type v modifiers CTRL"});
    } else if (ydotool) {
        proc->start("sh" , {"-c", "sleep 0.1 && ydotool key 29:1 47:1 47:0 29:0"}); // These keycodes stand for ctrl v
    } else if (xdotool) { // prefer platform-specific first
        proc->start("sh" , {"-c", "sleep 0.1 && xdotool key ctrl+v"});
    }

    QObject::connect(proc, &QProcess::finished, proc, [proc](int exitCode, QProcess::ExitStatus exitStatus){
        if (exitStatus != QProcess::ExitStatus::NormalExit || exitCode != EXIT_SUCCESS)
        {
            WARN << QString("Paste failed (%1).").arg(exitCode);
            if (auto out = proc->readAllStandardOutput(); out.isEmpty())
                WARN << out;
            if (auto err = proc->readAllStandardError(); err.isEmpty())
                WARN << err;
        }
        proc->deleteLater();
    });
#elif defined(Q_OS_WIN)
    qFatal("Paste not implemented on windows.");
#endif
}

long long albert::runDetachedProcess(const QStringList &commandline, const QString &working_dir)
{
    qint64 pid = 0;
    if (!commandline.empty())
    {
        auto wd = working_dir.isEmpty() ? QDir::homePath() : working_dir;
        if (QProcess::startDetached(commandline[0], commandline.mid(1), wd, &pid))
            INFO << QString("Detached process started successfully. (WD: %1, PID: %2, CMD: %3")
                        .arg(wd).arg(pid).arg(QDebug::toString(commandline));
        else
            WARN << "Starting detached process failed." << commandline;
    } else
        WARN << "runDetachedProcess: commandline must not be empty!";
    return pid;
}

void albert::tryCreateDirectory(const filesystem::path& path)
{
    try {
        filesystem::create_directories(path);
    } catch (const filesystem::filesystem_error &e) {
        throw runtime_error(
            QCoreApplication::translate("albert", "Failed creating directory %1: %2")
                .arg(e.path1().c_str(), e.what()).toStdString());
    }
}

const albert::ExtensionRegistry &albert::extensionRegistry()
{ return App::instance()->extensionRegistry(); }

QMessageBox::StandardButton albert::question(const QString &text,
                                             QMessageBox::StandardButtons buttons,
                                             QMessageBox::StandardButton defaultButton)
{
    return QMessageBox::information(QWidget::find(App::instance()->frontend()->winId()),
                                    qApp->applicationDisplayName(),
                                    text, buttons, defaultButton);
}

QMessageBox::StandardButton albert::information(const QString &text,
                                                QMessageBox::StandardButtons buttons,
                                                QMessageBox::StandardButton defaultButton)
{
    return QMessageBox::information(QWidget::find(App::instance()->frontend()->winId()),
                                    qApp->applicationDisplayName(),
                                    text, buttons, defaultButton);
}

QMessageBox::StandardButton albert::warning(const QString &text,
                                            QMessageBox::StandardButtons buttons,
                                            QMessageBox::StandardButton defaultButton)
{
    return QMessageBox::information(QWidget::find(App::instance()->frontend()->winId()),
                                    qApp->applicationDisplayName(),
                                    text, buttons, defaultButton);
}

QMessageBox::StandardButton albert::critical(const QString &text,
                                             QMessageBox::StandardButtons buttons,
                                             QMessageBox::StandardButton defaultButton)
{
    return QMessageBox::information(QWidget::find(App::instance()->frontend()->winId()),
                                    qApp->applicationDisplayName(),
                                    text, buttons, defaultButton);
}
