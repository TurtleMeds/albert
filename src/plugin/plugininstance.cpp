// Copyright (c) 2023-2024 Manuel Schneider

#include "albert.h"
#include "plugininstance.h"
#include "plugininstance_p.h"
#include "pluginloader.h"
#include "pluginmetadata.h"
#include <QSettings>
using namespace albert;
using namespace std;

PluginInstance::PluginInstance() : d(make_unique<PluginInstancePrivate>()) {}

PluginInstance::~PluginInstance() = default;

vector<Extension*> PluginInstance::extensions() { return {}; }

QWidget *PluginInstance::buildConfigWidget() { return nullptr; }

filesystem::path PluginInstance::cacheLocation() const
{ return albert::cacheLocation() / d->loader.metaData().id.toStdString(); }

filesystem::path PluginInstance::configLocation() const
{ return albert::configLocation() / d->loader.metaData().id.toStdString(); }

filesystem::path PluginInstance::dataLocation() const
{ return albert::dataLocation() / d->loader.metaData().id.toStdString(); }

unique_ptr<QSettings> albert::PluginInstance::settings() const
{
    auto s = albert::settings();
    s->beginGroup(d->loader.metaData().id);
    return s;
}

unique_ptr<QSettings> albert::PluginInstance::state() const
{
    auto s = albert::state();
    s->beginGroup(d->loader.metaData().id);
    return s;
}

const PluginLoader &PluginInstance::loader() const { return d->loader; }

