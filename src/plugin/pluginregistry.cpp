// Copyright (c) 2023-2024 Manuel Schneider

#include "extensionregistry.h"
#include "logging.h"
#include "plugininstance.h"
#include "pluginloader.h"
#include "pluginmetadata.h"
#include "pluginprovider.h"
#include "pluginregistry.h"
#include "topologicalsort.hpp"
#include <QCoreApplication>
#include <ranges>
using namespace albert;
using namespace std;


PluginRegistry::PluginRegistry(ExtensionRegistry &reg):
    extension_registry_(reg)
{
    connect(&extension_registry_, &ExtensionRegistry::added,
            this, &PluginRegistry::onRegistered);

    connect(&extension_registry_, &ExtensionRegistry::removed,
            this, &PluginRegistry::onDeregistered);
}

PluginRegistry::~PluginRegistry()
{
    if (!plugin_providers_.empty())
        WARN << "PluginRegistry destroyed with active plugin providers";

    if (!plugins_.empty())
        WARN << "PluginRegistry destroyed with active plugins";
}

const map<QString, Plugin> &PluginRegistry::plugins() const { return plugins_; }

bool PluginRegistry::autoloadEnabledPlugins() const { return autoload_enabled_plugins_; }

void PluginRegistry::setAutoloadEnabledPlugins(bool enable)
{
    if (autoload_enabled_plugins_ == enable)
        return;

    autoload_enabled_plugins_ = enable;

    if (enable)
    {
        auto view = plugins_
                | views::transform([](auto &p){ return &p.second; })
                | views::filter([](auto *p){ return p->isUser() && p->enabled; });
        load({view.begin(), view.end()});
    }
}

void PluginRegistry::setEnabled(const QString &id, bool enable)
{
    auto &plugin = plugins_.at(id);

    if (plugin.enabled == enable)
        return;

    // Get closure of plugins to enable/disable
    set<Plugin*> plugins = enable ? plugin.transitiveDependencies() : plugin.transitiveDependees();
    plugins.insert(&plugin);

    // Enable/disable plugins
    for (auto *p : plugins)
        p->setEnabled(enable);  // implicit isUser check

    if (autoload_enabled_plugins_)
        enable ? load(id) : unload(id);
}

void PluginRegistry::load(const QString &id)
{
    auto &plugin = plugins_.at(id);
    auto plugins = plugin.transitiveDependencies();
    plugins.insert(&plugin);
    load({plugins.cbegin(), plugins.cend()});
}

void PluginRegistry::unload(const QString &id)
{
    auto &plugin = plugins_.at(id);
    auto plugins = plugin.transitiveDependees();
    plugins.insert(&plugin);
    unload({plugins.cbegin(), plugins.cend()});
}

void PluginRegistry::onRegistered(Extension *e)
{
    auto *pp = dynamic_cast<PluginProvider*>(e);
    if (!pp)
        return;

    // Register plugin provider
    const auto &[_, pp_reg_success] = plugin_providers_.insert(pp);
    if (!pp_reg_success)
        qFatal("Plugin provider registered twice.");

    // Make the loaders unique by id
    map<QString, PluginLoader*> unique_loaders;
    for (auto &loader : pp->plugins())
        if (const auto &[it, succ] = unique_loaders.emplace(loader->metaData().id, loader); !succ)
            INFO << QString("Plugin '%1' at '%2' shadowed by '%3'")
                        .arg(it->first, loader->path(), it->second->path());

    // Build a dependeny graph (edges)
    map<PluginLoader*, set<PluginLoader*>> dependency_graph;
    for (auto *loader : unique_loaders | views::values)
    {
        try{
            auto v = loader->metaData().plugin_dependencies
                    | views::transform([&](auto &id){ return unique_loaders.at(id); });
            dependency_graph.emplace(piecewise_construct,
                                     forward_as_tuple(loader),
                                     forward_as_tuple(begin(v), end(v)));

        } catch (const out_of_range &) {
            WARN << "Skipping plugin" << loader->path() << "because of missing dependencies.";
        }
    }

    // Build dependee graph (reverse edges)
    map<PluginLoader*, set<PluginLoader*>> dependee_graph;
    for (auto &[loader, deps] : dependency_graph)
        for (auto *dep : deps)
            dependee_graph[dep].insert(loader);

    // topological sort the graph, print error
    auto topo = topologicalSort(dependency_graph);
    if ( !topo.error_set.empty())
    {
        // auto msg = tr("Cyclic or missing dependencies detected:");
        for (const auto &[loader, deps] : topo.error_set)
        {
            auto d = deps | views::transform([](auto *l){ return l->metaData().id; });
            WARN << "Skipping plugin" << loader->path() << "because of cyclic dependencies:"
                 << QStringList(d.begin(), d.end()).join(", ");
            dependency_graph.erase(loader);
        }
    }

    // Register plugins
    int load_order{0};
    for (auto *loader : topo.sorted)
    {
        const auto &[it, succ] = plugins_.emplace(piecewise_construct,
                                                  forward_as_tuple(loader->metaData().id),
                                                  forward_as_tuple(*pp, *loader));
        if (!succ)
            qFatal("Duplicate plugin id registered: %s", qPrintable(it->first));

        auto &p = it->second;

        p.load_order = load_order++;

        for (const auto &dependency_id : p.metaData().plugin_dependencies)
        {
            auto &dep = plugins_.at(dependency_id);
            p.dependencies.insert(&dep);
            dep.dependees.insert(&p);
        }

        // Signal mappers

        connect(&p, &Plugin::enabledChanged, this,
                [this, &p](bool e){ emit pluginEnabledChanged(p.id(), e); });

        connect(&p, &Plugin::stateChanged, this,
                [this, &p](Plugin::State s, QString i){ emit pluginStateChanged(p.id(), s, i); });
    }

    emit pluginsChanged();

    // Load enabled user plugins of this provider
    if (autoload_enabled_plugins_)
    {
        auto view = plugins_
                | views::transform([](auto &p){ return &p.second; })
                | views::filter([pp](auto *p){ return &p->provider == pp
                                                      && p->isUser()
                                                      && p->enabled; });
        load({view.begin(), view.end()});
    }
}

void PluginRegistry::onDeregistered(Extension *e)
{
    auto *pp = dynamic_cast<PluginProvider*>(e);
    if (!pp)
        return;

    // Unload plugins of this provider
    auto view = plugins_
            | views::transform([](auto &p){ return &p.second; })
            | views::filter([pp](auto *p){ return &p->provider == pp; });
    unload({view.begin(), view.end()});

    // Remove plugins of this provider
    erase_if(plugins_, [pp](const auto& it){ return &it.second.provider == pp; });
    emit pluginsChanged();

    // Remove provider
    if (!plugin_providers_.erase(pp))
        qFatal("Plugin provider was not registered onRem.");
}

map<Plugin*, QString> PluginRegistry::load(vector<Plugin*> plugins)
{
    ranges::sort(plugins, [](auto *l, auto *r){ return l->load_order < r->load_order; });

    map<Plugin*, QString> errors;
    for (auto *p : plugins)
    {
        if (p->state == Plugin::Loaded)
            continue;

        qApp->processEvents();

        Q_ASSERT (p->state == Plugin::Unloaded);

        if (auto err = p->load(); err.isEmpty())
        {
            qApp->processEvents();

            for (auto *e : p->instance->extensions())
                if (!extension_registry_.registerExtension(e))
                    CRIT << "Root extension registration failed: " << p->id();
        }
        else
            errors.emplace(p, err);

        qApp->processEvents();
    }

    return errors;


    // QStringList errors;
    // for (auto *p : plugins)
    //     if (p->state() != Plugin::State::Loaded)
    //         if (auto err = p->loader_.load(); !err.isEmpty())
    //         {
    //             WARN << QString("Failed loading plugin '%1': %2").arg(p->id(), err);
    //             errors << QString("%1 (%2):\n%3").arg(p->metaData().name, p->id(), err);
    //         }

    // if (!errors.isEmpty())
    //     QMessageBox::warning(nullptr, qApp->applicationDisplayName(),
    //                          QString("%1:\n\n%2")
    //                              .arg(tr("Failed loading plugins", nullptr, errors.size()),
    //                                   errors.join("\n")));
    // // Load enabled plugins
    // QStringList errors;
    // for (auto *p : plugins)
    //     if (auto err = p->load(); err.isEmpty())
    //     {
    //         for (auto *e : p->instance()->extensions())
    //             extension_registry_.registerExtension(e);
    //     }
    //     else
    //     {
    //         WARN << QString("Failed loading plugin '%1': %2").arg(p->id(), err);
    //         errors << QString("%1 (%2):\n%3").arg(p->metaData().name, p->id(), err);
    //     }

    // if (!errors.isEmpty())
    //     QMessageBox::warning(nullptr, qApp->applicationDisplayName(),
    //                          QString("%1:\n\n%2")
    //                              .arg(tr("Failed loading plugins", nullptr, errors.size()),
    //                                   errors.join("\n")));
}

map<Plugin*, QString> PluginRegistry::unload(vector<Plugin*> plugins)
{
    ranges::sort(plugins, [](auto *l, auto *r){ return l->load_order > r->load_order; });

    map<Plugin*, QString> errors;
    for (auto *p : plugins)
    {
        if (p->state == Plugin::Unloaded)
            continue;

        qApp->processEvents();

        Q_ASSERT (p->state == Plugin::Loaded);

        for (auto *e : p->instance->extensions())
            extension_registry_.deregisterExtension(e);

        qApp->processEvents();

        if (auto err = p->unload(); !err.isEmpty())
            errors.emplace(p, err);

        qApp->processEvents();
    }

    return errors;

    // QStringList errors;
    // for (auto *p : v)
    // {
    //     if (p->state() != Plugin::State::Loaded)
    //         for (auto *e : p->instance()->extensions())
    //             extension_registry_.deregisterExtension(e);

    //     if (auto err = p->unload(); !err.isEmpty())
    //     {
    //         WARN << QString("Failed unloading plugin '%1': %2").arg(p->id(), err);
    //         errors << QString("%1 (%2):\n%3").arg(p->metaData().name, p->id(), err);
    //     }
    // }

    // if (!errors.isEmpty())
    //     QMessageBox::warning(nullptr, qApp->applicationDisplayName(),
    //                          QString("%1:\n\n%2")
    //                              .arg(tr("Failed unloading plugins", nullptr, errors.size()),
    //                                   errors.join("\n")));





    // // Unload plugins
    // for (auto *p : plugins)
    //     if (auto err = p->unload(); !err.isEmpty())
    //         WARN << QString("Failed unloading plugin '%1': %2").arg(p->id(), err);
    //     else
    //     {
    //         // Wait for finished loading
    //         QEventLoop loop;
    //         QObject::connect(p, &Plugin::stateChanged, &loop, &QEventLoop::quit);
    //         loop.exec();
    //         loop.processEvents();
    //     }

    // // Remove registerd plugins of this provider
    // erase_if(registered_plugins_, [=](const auto& it){ return it.second.provider == plugin_provider; });

    // // Remove provider
    // plugin_providers_.erase(plugin_provider);


}






// // Auto deregister root extensions
// if (auto *e = dynamic_cast<Extension*>(instance_); e)
//     if (!PluginRegistry::staticDI.registry->deregisterExtension(e))
//         CRIT << "Root extension deregistration failed: " << id();



// [[nodiscard]]
// QString Plugin::load() noexcept
// {

// }


// [[nodiscard]]
// QString Plugin::unload() noexcept
// {
//     if (state_ != State::Loaded)
//         return localStateString();

//     // Auto deregister root extensions
//     if (auto *e = dynamic_cast<Extension*>(instance_); e)
//         if (!PluginRegistry::staticDI.registry->deregisterExtension(e))
//             CRIT << "Root extension deregistration failed: " << id();

//     instance_ = nullptr;

//     QString err;
//     try
//     {
//         auto t = system_clock::now();
//         loader_->unload();
//         DEBG << QStringLiteral("%1 ms spent unloading plugin '%2'")
//                 .arg(duration_cast<milliseconds>(system_clock::now()-t).count()).arg(id());
//     }
//     catch (const exception &e)
//     {
//         err = e.what();
//     }
//     catch (...)
//     {
//         err = "Unknown exception.";
//     }

//     setState(State::Unloaded, err);
//     return err;
// }























// void PluginRegistry::loadEnabledPlugins(PluginProvider *pp)
// {
//     // Load enabled user plugins of this provider
//     vector<Plugin*> plugins_to_load;
//     ranges::copy_if(registered_plugins_,
//                     plugins_to_load.begin(),
//                     [&](const auto &p){ return p.second.provider == pp
//                                                && p.second.isUser() && p.second.isEnabled(); });

//     for (auto &[id, p] : registered_plugins_)
//         if (p.provider == pp && p.isUser() && p.isEnabled())
//             plugins_to_load.push_back(&p);

//     // Sort by load order
//     ranges::sort(plugins_to_load,
//                  [](const auto *l, const auto *r){ return l->load_order < r->load_order; });

//     // Load enabled plugins
//     for (auto *p : plugins_to_load)
//     {
//         QString err = p->load();

//         if (err.isEmpty())
//         {
//             // Wait for finished loading
//             QEventLoop loop;
//             QObject::connect(p, &Plugin::stateChanged, &loop, &QEventLoop::quit);
//             loop.exec();
//             loop.processEvents();

//             if (p->state() == Plugin::State::Loaded)
//                 continue;
//             else
//             {
//                 Q_ASSERT(p->state() == Plugin::State::Unloaded);
//                 err = p->stateInfo();
//             }
//         }

//         WARN << QString("Failed loading plugin '%1': %2").arg(p->id(), err);
//         QMessageBox::warning(nullptr, qApp->applicationDisplayName(),
//                              QString("%1:\n\n%2").arg(tr("Failed loading plugin"), err));
//     }
// }

// void PluginRegistry::loadEnabledPlugins()
// {
//     using PS = Plugin::State;

//     while (true)
//     {
//         vector<Plugin*> plugins_to_load;
//         for (auto &[id, plugin] : registered_plugins_)
//             if (plugin.isUser()
//                     && plugin.isEnabled()
//                     && plugin.state() == Plugin::State::Unloaded
//                     && plugin.stateInfo().isEmpty()  // no error
//                     && ranges::all_of(plugin.transitiveDependencies(),
//                                       [](const auto *p){ return p->state() == PS::Loaded; }))
//                 plugins_to_load.push_back(&plugin);

//         if (plugins_to_load.empty())
//             break;

//         for (auto *p : plugins_to_load)
//             p->load();

//         // Wait for all of them to be loaded
//         while (ranges::any_of(plugins_to_load,
//                               [](const auto *p){ return p->state() == PS::Busy; }))
//             qApp->processEvents(QEventLoop::AllEvents|QEventLoop::WaitForMoreEvents);
//     }



//     // function<QString(Plugin *p)> map = [](Plugin *p) { return p->load(); };

//     // watcher.setFuture(QtConcurrent::mapped(plugins_to_load, &Plugin::load));

//     // auto conn = connect(&watcher, &QFutureWatcher<void>::finished, this, [this, plugins_to_load]
//     // {
//     //     for (uint i = 0; i < plugins_to_load.size(); ++i)
//     //         if (auto error = watcher.resultAt(i); !error.isEmpty())
//     //         {
//     //             auto id = plugins_to_load.at(i)->id();
//     //             WARN << QString("Failed loading plugin '%1': %2").arg(id, error);
//     //             QMessageBox::warning(nullptr, qApp->applicationDisplayName(),
//     //                                  QString("%1:\n\n%2").arg(
//     //                                      tr("Failed loading plugin '%1'").arg(id),
//     //                                      error));
//     //         }

//     //     watcher.disconnect();
//     //     loadEnabledPlugins();
//     // });

// }
