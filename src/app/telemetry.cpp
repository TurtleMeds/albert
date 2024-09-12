// Copyright (C) 2014-2024 Manuel Schneider

#include "logging.h"
#include "telemetry.h"
#include "pluginregistry.h"
#include "util.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
#include <QTimer>
static const char *CFG_LAST_REPORT = "last_report";
static const char *CFG_LAST_TELEMETRY_DATE = "last_telemetry_date";
using namespace albert;


Telemetry::Telemetry(const PluginRegistry &pr):
    plugin_registry(pr),
    last_report_date(state()->value(CFG_LAST_TELEMETRY_DATE).toDate())
{
    timer.start(60000);  // every minute
    QObject::connect(&timer, &QTimer::timeout,
                     &timer, [this]{ trySendReport(); });

    QTimer::singleShot(0, [this]{ trySendReport(); });

}

void Telemetry::trySendReport()
{
    INFO << buildReportString();

    // At 3 AM most people are asleep. Use it as the beginning of a "human day".
    auto curr_date = QDateTime::currentDateTime().addSecs(-10800).date();

    // Skip. Sent already today.
    // if (curr_date == last_report_date)
    //     return;

    QString a = "Zffb,!!*\" $## $\"' **!";
    for (auto &c : a)
        c.unicode() = c.unicode() + 14;

    QNetworkRequest request((QUrl(a)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply* reply = network()->put(request, buildReport().toJson(QJsonDocument::Compact));
    QObject::connect(reply, &QNetworkReply::finished, [reply, curr_date]
    {
        if (reply->error() == QNetworkReply::NoError)
        {
            DEBG << "Successfully sent telemetry data.";
            state()->setValue(CFG_LAST_TELEMETRY_DATE, curr_date);
        }
        else
            WARN << "Failed to send telemetry data:" << reply->errorString();

        reply->deleteLater();
    });
}

QJsonDocument Telemetry::buildReport() const
{
    QJsonObject object;

    object.insert("report", 2);  // report version

    object.insert("version", qApp->applicationVersion());

    object.insert("timezone", QDateTime::currentDateTime().offsetFromUtc() / 3600);

    object.insert("os", QSysInfo::prettyProductName());

    auto hash = QCryptographicHash::hash(QSysInfo::machineUniqueId(), QCryptographicHash::Sha1);
    object.insert("id", QString::fromUtf8(hash.toHex()).left(12));

    QJsonArray enabled_plugins;
    for (const auto &[id, plugin] : plugin_registry.plugins())
        if (plugin.enabled)
            enabled_plugins.append(id);
    object.insert("enabled_plugins", enabled_plugins);

    return QJsonDocument(object);
}

QString Telemetry::buildReportString() const
{
    return buildReport().toJson(QJsonDocument::Indented);
}
