// Copyright (c) 2022-2024 Manuel Schneider

#include "pluginmetadata.h"
#include "pluginregistry.h"
#include "pluginsmodel.h"
#include <QApplication>
#include <QIcon>
#include <QPalette>
#include <QStyle>
#include <ranges>
using namespace albert;
using namespace std;

PluginsModel::PluginsModel(PluginRegistry &plugin_registry) : plugin_registry_(plugin_registry)
{
    connect(&plugin_registry, &PluginRegistry::pluginsChanged,
            this, &PluginsModel::updatePluginList);

    connect(&plugin_registry_, &PluginRegistry::pluginStateChanged,
            this, &PluginsModel::updateView);

    connect(&plugin_registry_, &PluginRegistry::pluginEnabledChanged,
            this, &PluginsModel::updateView);

    updatePluginList();
}

QIcon PluginsModel::getCachedIcon(const QString &url) const
{
    try {
        return icon_cache.at(url);
    } catch (const out_of_range &e) {
        return icon_cache.emplace(url, url).first->second;
    }
}

int PluginsModel::rowCount(const QModelIndex &) const
{ return static_cast<int>(plugins_.size()); }

int PluginsModel::columnCount(const QModelIndex &) const
{ return 1; }

QVariant PluginsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    switch (const auto &p = *plugins_[index.row()]; role) {

    case Qt::CheckStateRole:
        if (p.isUser())
        {
            if (p.state == Plugin::State::Loading)
                return Qt::PartiallyChecked;
            else
                return p.enabled ? Qt::Checked : Qt::Unchecked;
        }
        break;

    case Qt::DecorationRole:
        if (p.state == Plugin::State::Unloaded && !p.state_info.isNull())
            return QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical);
        break;

    case Qt::DisplayRole:
        return p.metaData().name;

    case Qt::ForegroundRole:
        if (p.state != Plugin::State::Loaded)
            return qApp->palette().color(QPalette::PlaceholderText);
        break;

    case Qt::ToolTipRole:
        return p.state_info;

    case Qt::UserRole:
        return p.id();

    }
    return {};
}

bool PluginsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && index.column() == 0 && role == Qt::CheckStateRole){
        try {
            if (auto &p = *plugins_[index.row()]; p.isUser())
            {
                if (value == Qt::Checked)
                    plugin_registry_.setEnabled(p.id(), true);
                else if (value == Qt::Unchecked)
                    plugin_registry_.setEnabled(p.id(), false);
            }
        }
        catch (out_of_range &e){}
    }
    return false;
}

Qt::ItemFlags PluginsModel::flags(const QModelIndex &idx) const
{;
    if (idx.isValid()){
        switch (auto &p = *plugins_[idx.row()]; p.state)
        {
        case Plugin::State::Loading:
        case Plugin::State::Unloading:
            return Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        case Plugin::State::Loaded:
        case Plugin::State::Unloaded:
            return Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
        }
    }
    return Qt::NoItemFlags;
}

void PluginsModel::updatePluginList()
{
    beginResetModel();

    auto v = plugin_registry_.plugins() | views::values
            | views::transform([](auto &p){ return &p; });
    plugins_ = { begin(v), end(v) };

    ranges::sort(plugins_, [](auto &l, auto &r){ return l->metaData().name < r->metaData().name; });

    endResetModel();
}

void PluginsModel::updateView()
{
    // Well not worth the optimizations
    emit dataChanged(index(0), index(plugins_.size()-1));
}
