// Copyright (c) 2024 Manuel Schneider

#include "albert.h"
#include "extension.h"
#include "extensionregistry.h"
#include "logging.h"
#include "plugin.h"
#include "plugininstance.h"
#include "plugininstance_p.h"
#include "pluginloader.h"
#include "pluginmetadata.h"
#include <QRegularExpression>
#include <QSettings>
#include <chrono>
using namespace albert;
using namespace std::chrono;
using namespace std;

Plugin::Plugin(PluginProvider &pp, PluginLoader &pl) noexcept:
    provider(pp),
    loader(pl),
    state(State::Unloaded),
    instance(nullptr)
{
    enabled = settings()->value(QString("%1/enabled").arg(id()), false).toBool();

    static const auto regex_id = QRegularExpression("[a-z0-9_]");
    if (!regex_id.match(metaData().id).hasMatch())
        WARN << id() << "Invalid plugin id. Use [a-z0-9_].";

    static const QRegularExpression regex_version(R"R(^(\d+)(?>\.(\d+))?\.(\d+)$)R");
    if (!regex_version.match(metaData().version).hasMatch())
        WARN << id() << "metadata: Invalid version scheme. Use '<major>.[<minor>.]<patch>'.";

    if (metaData().name.isEmpty())
        WARN << id() << "metadata: Name should not be empty.";

    if (metaData().description.isEmpty())
        WARN << id() << "metadata: Description should not be empty.";

    if (metaData().license.isEmpty())
        WARN << id() << "metadata: License should not be empty.";

    if (metaData().url.isEmpty())
        WARN << id() << "metadata: URL should not be empty.";

    if (metaData().authors.isEmpty())
        WARN << id() << "metadata: Authors should not be empty.";
}

Plugin::~Plugin() { Q_ASSERT(state == State::Unloaded); }

QString Plugin::path() const
{ return loader.path(); }

const PluginMetaData &Plugin::metaData() const
{ return loader.metaData(); }

const QString &Plugin::id() const
{ return loader.metaData().id; }

bool Plugin::isUser() const
{ return loader.metaData().load_type == PluginMetaData::LoadType::User; }

bool Plugin::isFrontend() const
{ return loader.metaData().load_type == PluginMetaData::LoadType::Frontend; }

QWidget *Plugin::buildConfigWidget() const
{ return instance ? instance->buildConfigWidget() : nullptr; }

std::set<Plugin*> Plugin::transitiveDependencies() const
{
    set<Plugin*> D = dependencies;
    for (auto * d : D)
        D.merge(d->transitiveDependencies());
    return D;
}

set<Plugin*> Plugin::transitiveDependees() const
{
    set<Plugin*> D = dependees;
    for (auto * d : D)
        D.merge(d->transitiveDependees());
    return D;
}

void Plugin::setEnabled(bool enable)
{
    // Only user plugins
    if (!isUser() || enabled == enable)
        return;

    enabled = enable;
    settings()->setValue(QString("%1/enabled").arg(id()), enable);
    emit enabledChanged(enable);
}

void Plugin::setState(State s, QString i)
{
    state = s;
    state_info = i;
    emit stateChanged(state, state_info);
}

QString Plugin::localizedStateString(State state)
{
    switch (state) {
    case Plugin::State::Unloaded:
        return tr("Plugin is unloaded.");
    case Plugin::State::Loading:
        return tr("Plugin is loading.");
    case Plugin::State::Loaded:
        return tr("Plugin is loaded.");
    case Plugin::State::Unloading:
        return tr("Plugin is unloading.");
    }
    return {};
}

QString Plugin::load()
{
    Q_ASSERT (state == State::Unloaded);

    QString err;
    try {
        INFO << "Loading plugin" << id();
        setState(State::Loading);
        auto t = system_clock::now();
        PluginInstancePrivate::current_loader = &loader;
        instance = &loader.load();  // throws
        auto d = duration_cast<milliseconds>(system_clock::now() - t).count();
        DEBG << QString("Plugin loaded: %1 (%2 ms)").arg(id()).arg(d);
        setState(State::Loaded, tr("Load time: %1 ms.").arg(d));
        return {};
    } catch (const exception &e) {
        err = e.what();
    } catch (...) {
        err = tr("Unknown exception.");
    }
    WARN << QString("Failed loading plugin: %1 (%2)").arg(id(), err);
    setState(State::Unloaded, err);
    return err;
}

QString Plugin::unload()
{
    Q_ASSERT (state == State::Loaded);

    QString err;
    try {
        INFO << "Unloading plugin" << id();
        setState(State::Unloading);
        instance = nullptr;
        auto t = system_clock::now();
        loader.unload();
        auto d = duration_cast<milliseconds>(system_clock::now() - t).count();
        DEBG << QString("Plugin unloaded: %1 (%2 ms)").arg(id()).arg(d);
    } catch (const exception &e) {
        err = e.what();
    } catch (...) {
        err = "Unknown exception.";
    }
    setState(State::Unloaded, err);

    return err;
}
