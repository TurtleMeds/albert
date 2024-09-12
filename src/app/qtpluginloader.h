// Copyright (c) 2023-2024 Manuel Schneider

#pragma once
#include "pluginloader.h"
#include "pluginmetadata.h"
#include <QPluginLoader>
namespace albert { class PluginInstance; }
class QTranslator;

class QtPluginLoader : public albert::PluginLoader
{
public:

    QtPluginLoader(const QString &path);
    ~QtPluginLoader();

    QString path() const noexcept override;
    const albert::PluginMetaData &metaData() const noexcept override;
    albert::PluginInstance &load() override;
    void unload() override;

private:

    QPluginLoader loader_;
    albert::PluginMetaData metadata_;
    albert::PluginInstance *instance_;
    std::unique_ptr<QTranslator> translator;

};
