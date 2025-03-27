// Copyright (c) 2022-2024 Manuel Schneider

#include "extension.h"
#include "frontend.h"
#include "item.h"
#include "itemsmodel.h"
#include "logging.h"
#include "query.h"
#include "usagedatabase.h"
#include <QStringListModel>
#include <QTimer>
using namespace albert;
using namespace std;

ItemsModel::ItemsModel(QObject *parent) : QAbstractListModel(parent) {}

int ItemsModel::rowCount(const QModelIndex &) const { return (int)items.size(); }

QVariant ItemsModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        const auto &[extension, item] = items[index.row()];

        switch (role) {
            case (int)ItemRoles::TextRole:
            {
                QString text = item->text();
                text.replace('\n', ' ');
                return text;
            }
            case (int)ItemRoles::SubTextRole:
            {
                QString text = item->subtext();
                text.replace('\n', ' ');
                return text;
            }
            case Qt::ToolTipRole:
                return QString("%1\n%2").arg(item->text(), item->subtext());

            case (int)ItemRoles::InputActionRole:
                return item->inputActionText();

            case (int)ItemRoles::IconUrlsRole:
                return item->iconUrls();

            case (int)ItemRoles::ActionsListRole:
            {
                if (auto it = actionsCache.find(item.get()); it != actionsCache.end())
                    return it->second;

                QStringList l;
                for (const auto &a : item->actions())
                    l << a.text;

                actionsCache.emplace(item.get(), l);

                return l;
            }
        }
    }
    return {};
}

QHash<int, QByteArray> ItemsModel::roleNames() const
{
    static QHash<int, QByteArray> qml_role_names = {
        {(int)ItemRoles::TextRole, "itemText"},
        {(int)ItemRoles::SubTextRole, "itemSubText"},
        {(int)ItemRoles::InputActionRole, "itemInputAction"},
        {(int)ItemRoles::IconUrlsRole, "itemIconUrls"},
        {(int)ItemRoles::ActionsListRole, "itemActionsList"}
    };
    return qml_role_names;
}

QAbstractListModel *ItemsModel::buildActionsModel(uint i) const
{
    QStringList l;
    for (const auto &a : items[i].item->actions())
        l << a.text;
    return new QStringListModel(l);
}
