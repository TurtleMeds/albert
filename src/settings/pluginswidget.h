// Copyright (c) 2022-2024 Manuel Schneider

#pragma once
#include <QSortFilterProxyModel>
#include <QWidget>
#include <memory>
#include "ui_pluginswidget.h"
class PluginRegistry;
class PluginsModel;
class QListView;
class QScrollArea;


class SortFilterModel : public QSortFilterProxyModel
{
public:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    // Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant data(const QModelIndex &index, int role) const override;


    void setEnabled(bool enable);
    bool isEnabled() const;

private:

    bool isEnabled_;

};

class PluginsWidget final : public QWidget
{
    Q_OBJECT
public:
    PluginsWidget(PluginRegistry&);
    ~PluginsWidget();

    void tryShowPluginSettings(QString);

private:
    void onUpdatePluginWidget();

    Ui::PluginsWidget ui;
    PluginRegistry &plugin_registry_;
    std::unique_ptr<PluginsModel> model_;
    SortFilterModel proxy_model_;
};

