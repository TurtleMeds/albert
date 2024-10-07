// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <QString>
#include <albert/export.h>
#include <filesystem>
#include <memory>
class QNetworkAccessManager;
class QSettings;
class QUrl;

namespace albert
{
class ExtensionRegistry;

///
/// Restarts the application.
///
void restart();

///
/// Quits the application.
///
void quit();

///
/// Creates and/or shows the settings window. If `plugin_id` is not empty, shows the plugins tab and
/// selects the plugin having the `plugin_id`.
///
ALBERT_EXPORT void showSettings(QString plugin_id = {});

///
/// Returns the path to the directory where cache files should be stored (app cache location).
///
ALBERT_EXPORT std::filesystem::path cacheLocation();

///
/// Returns the path to the directory where config files should be stored (app config location).
///
ALBERT_EXPORT std::filesystem::path configLocation();

///
/// Returns the path to the directory where data files should be stored (app data location).
///
ALBERT_EXPORT std::filesystem::path dataLocation();

///
/// Creates a QSettings object for app config data.
///
ALBERT_EXPORT std::unique_ptr<QSettings> settings();

///
/// Creates a QSettings object for app state data.
///
ALBERT_EXPORT std::unique_ptr<QSettings> state();

///
/// Returns the global extension registry.
///
/// Utilze to look up extensions or watch for changes. Const because registering plugins via this
/// registry is not allowed. Use PluginInstance::extensions().
///
/// @see WeakDependency and StrongDependency.
///
ALBERT_EXPORT const ExtensionRegistry &extensionRegistry();

///
/// Returns the threadlocal, global QNetworkAccessManager.
///
ALBERT_EXPORT QNetworkAccessManager &network();

///
/// Opens the albert website in the default browser.
///
ALBERT_EXPORT void openWebsite();

///
/// Opens the URL `url` with the default URL handler.
///
ALBERT_EXPORT void openUrl(const QString &url);

///
/// Open the URL `url` with the default url handler.
///
ALBERT_EXPORT void open(const QUrl &url);

///
/// Opens the file at `path` with the default application.
///
ALBERT_EXPORT void open(const QString &path);

///
/// Opens the file at `path` with the default application.
///
ALBERT_EXPORT void open(const std::string &path);

///
/// Sets the system clipboard to `text`.
///
ALBERT_EXPORT void setClipboardText(const QString &text);

///
/// Returns `true` if the platform supports pasting, otherwise returns `false`.
///
/// @note This is a requirement for setClipboardTextAndPaste(…) to work.
///
ALBERT_EXPORT bool havePasteSupport();

///
/// Sets the system clipboard to `text` and paste the content to the front-most window.
///
/// @note Requires paste support. Check havePasteSupport() before using this function.
///
ALBERT_EXPORT void setClipboardTextAndPaste(const QString &text);

///
/// Starts the `commandline` in a new process, and detaches from it. Returns the PID on success;
/// otherwise returns 0. The process will be started in the directory `working_dir`. If
/// `working_dir` is empty, the working directory is the users home directory.
///
ALBERT_EXPORT long long runDetachedProcess(const QStringList &commandline, const QString &working_dir = {});

///
/// Creates a directory at `path` if it does not exist yet.
///
/// This is a utility function for use with the *Location functions.
///
/// @throws std::runtime_error if the directory could not be created.
///
ALBERT_EXPORT void tryCreateDirectory(const std::filesystem::path &path);

} // namespace albert
