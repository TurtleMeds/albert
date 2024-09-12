// Copyright (C) 2014-2024 Manuel Schneider

#pragma once
#include <QTimer>
#include <QDate>
class PluginRegistry;
class QJsonDocument;

class Telemetry final
{
public:

    Telemetry(const PluginRegistry&);

    QJsonDocument buildReport() const;
    QString buildReportString() const;

private:

    void trySendReport();

    const PluginRegistry &plugin_registry;
    QDate last_report_date;
    QTimer timer;

};
