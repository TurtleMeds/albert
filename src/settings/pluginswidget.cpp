// Copyright (c) 2022-2024 Manuel Schneider

#include "albert/extension/pluginprovider/plugininstance.h"
#include "albert/extension/pluginprovider/pluginmetadata.h"
#include "albert/extension/pluginprovider/pluginprovider.h"
#include "pluginregistry.h"
#include "pluginsmodel.h"
#include "pluginswidget.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QScrollArea>
#include <QSortFilterProxyModel>
#include <QSplitter>
using namespace albert;
using namespace std;


bool SortFilterModel::filterAcceptsRow(int source_row, const QModelIndex &) const
{ return !isEnabled_ || sourceModel()->index(source_row, 0).data(Qt::CheckStateRole) == Qt::Checked; }


// Qt::ItemFlags SortFilterModel::flags(const QModelIndex &index) const
// {
//     return QSortFilterProxyModel::flags(index) & ~Qt::ItemIsUserCheckable;
// }

QVariant SortFilterModel::data(const QModelIndex &index, int role) const
{
    if (isEnabled_ && role == Qt::CheckStateRole)
        return {};
    else
        return QSortFilterProxyModel::data(index, role);
}

void SortFilterModel::setEnabled(bool enable)
{
    isEnabled_ = enable;
    invalidate();
}

bool SortFilterModel::isEnabled() const { return isEnabled_; }

PluginsWidget::PluginsWidget(PluginRegistry &plugin_registry)
    : plugin_registry_(plugin_registry), model_(new PluginsModel(plugin_registry))
{
    ui.setupUi(this);
    // ui.horizontalLayout->setContentsMargins(ui.horizontalLayout->spacing(),
    //                                         ui.horizontalLayout->spacing(),
    //                                         ui.horizontalLayout->spacing(),
    //                                         ui.horizontalLayout->spacing());

    // listView_plugins = new QListView(this);
    // listView_plugins->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // listView_plugins->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // listView_plugins->setProperty("showDropIndicator", QVariant(false));
    // listView_plugins->setAlternatingRowColors(true);
    // listView_plugins->setSpacing(1);
    // listView_plugins->setUniformItemSizes(true);

    // scrollArea_info = new QScrollArea(this);
    // scrollArea_info->setFrameShape(QFrame::StyledPanel);
    // scrollArea_info->setFrameShadow(QFrame::Sunken);
    // scrollArea_info->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // scrollArea_info->setWidgetResizable(true);
    // scrollArea_info->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignTop);

    // QSplitter *splitter = new QSplitter(this);
    // splitter->addWidget(listView_plugins);
    // splitter->addWidget(scrollArea_info);

    // QHBoxLayout *horizontalLayout = new QHBoxLayout(this);
    // horizontalLayout->addWidget(splitter);
    // horizontalLayout->setContentsMargins(splitter->handleWidth(), splitter->handleWidth(),
    //                                      splitter->handleWidth(), splitter->handleWidth());

    // Setup model

    // proxy_model_.setFilterRole(Qt::CheckStateRole);
    proxy_model_.setSourceModel(model_.get());
    proxy_model_.setDynamicSortFilter(true);


    ui.listView_plugins->setStyleSheet("QListView::item{height: 22px}");
    ui.listView_plugins->setModel(&proxy_model_);


    ui.listView_plugins->setMaximumWidth(ui.listView_plugins->sizeHintForColumn(0)
                                         + qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent));




    connect(ui.listView_plugins->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &PluginsWidget::onUpdatePluginWidget);

    connect(model_.get(), &PluginsModel::dataChanged,
            this, &PluginsWidget::onUpdatePluginWidget);


    // connect(ui.checkBox, &QCheckBox::clicked, &proxy_model_, &SortFilterModel::setEnabled);

    onUpdatePluginWidget();
}

void PluginsWidget::tryShowPluginSettings(QString plugin_id)
{
    for (auto row = 0; row < model_->rowCount(); ++row)
        if (auto index = model_->index(row); index.data(Qt::UserRole).toString() == plugin_id){
            ui.listView_plugins->setCurrentIndex(index);
            ui.listView_plugins->setFocus();
        }
}

PluginsWidget::~PluginsWidget() = default;

void PluginsWidget::onUpdatePluginWidget()
{
    auto current = ui.listView_plugins->currentIndex();
    QLabel *l;

    if (!current.isValid()){
        l = new QLabel(tr("Select a plugin"));
        l->setAlignment(Qt::AlignCenter);
        ui.scrollArea_info->setWidget(l);
        return;
    }

    auto id = current.data(Qt::UserRole).toString();
    auto &p = plugin_registry_.plugins().at(id);

    auto *widget = new QWidget;
    auto *vl = new QVBoxLayout;
    widget->setLayout(vl);

    // // Title
    // vl->addWidget(new QLabel(QString("<span style=\"font-size:16pt;font-style:bold;\">%1</span>").arg(p.metaData().name)));

    // // Description
    // vl->addWidget(new QLabel(QString("<span style=\"font-size:11pt;font-style:italic;\">%1</span>").arg(p.metaData().description)));

    vl->addWidget(new QLabel(QString("<span style=\"font-size:16pt;font-weight:600;\">%1</span><br>"
                                     "<span style=\"font-size:11pt;font-weight:lighter;font-style:italic;\">%2</span>")
                                               .arg(p.metaData().name, p.metaData().description)));

    // Plugin specific
    if (p.state() == Plugin::State::Loaded)
    {
        // Config widget
        if (auto *inst = p.instance(); inst)
            if (auto *cw = inst->buildConfigWidget())
                vl->addWidget(cw, 1); // Strech=1
    }
    else if (!p.stateInfo().isEmpty())
    {
        // Unloaded info
        if (!p.stateInfo().isEmpty())
        {
            l = new QLabel(p.stateInfo());
            l->setWordWrap(true);
            vl->addWidget(l);
        }
    }

    vl->addStretch();

    // META INFO

    QStringList meta;

    // Credits if any
    if (auto list = p.metaData().third_party_credits; !list.isEmpty())
        meta << tr("Credits: %1").arg(list.join(", "));

    // Required executables, if any
    if (auto list = p.metaData().binary_dependencies; !list.isEmpty())
        meta << tr("Required executables: %1", nullptr, list.size()).arg(list.join(", "));

    // Required libraries, if any
    if (auto list = p.metaData().runtime_dependencies; !list.isEmpty())
        meta << tr("Required libraries: %1", nullptr, list.size()).arg(list.join(", "));

    // Id, version, license, authors
    QStringList authors;
    for (const auto &author : p.metaData().authors)
        if (author.startsWith(QStringLiteral("@")))
            authors << QStringLiteral("<a href=\"https://github.com/%1\">%2</a>")
                           .arg(author.mid(1), author);
        else
            authors << author;

    meta << QString("<span style=\"color:#808080;\"><a href=\"%1\">%2 v%3</a>. %4. %5.</span>")
                       .arg(p.metaData().url,
                            p.metaData().id,
                            p.metaData().version,
                            tr("License: %1").arg(p.metaData().license),
                            tr("Authors: %1", nullptr, authors.size()).arg(authors.join(", ")));

    // Provider
    meta << tr("%1, Interface: %2").arg(p.provider->name(), p.metaData().iid);

    // Path
    meta << p.path();

    // Add meta
    l = new QLabel(QString("<span style=\"font-size:9pt;color:#808080;\">%1</span>").arg(meta.join("<br>")));
    l->setOpenExternalLinks(true);
    l->setWordWrap(true);
    vl->addWidget(l);

    ui.scrollArea_info->setWidget(widget);
}


