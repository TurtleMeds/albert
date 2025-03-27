// Copyright (c) 2023-2024 Manuel Schneider

#pragma once
#include <QAbstractListModel>
#include <map>
#include <memory>
#include <vector>
namespace albert{
class Extension;
class Item;
class Query;
class RankItem;
}


struct ResultItem
{
    albert::Extension *extension;
    std::shared_ptr<albert::Item> item;
};


// TODO remove with C++23
// From https://en.cppreference.com/w/cpp/utility/forward_like
template<class T, class U>
[[nodiscard]] constexpr auto&& forward_like(U&& x) noexcept
{
    constexpr bool is_adding_const = std::is_const_v<std::remove_reference_t<T>>;
    if constexpr (std::is_lvalue_reference_v<T&&>)
        if constexpr (is_adding_const)
            return std::as_const(x);
        else
            return static_cast<U&>(x);
    else
        if constexpr (is_adding_const)
            return std::move(std::as_const(x));
        else
            return std::move(x);
}


class ItemsModel final : public QAbstractListModel
{
public:

    ItemsModel(QObject *parent = nullptr); // important for qml cppownership

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    QAbstractListModel *buildActionsModel(uint i) const;

    void add(std::ranges::input_range auto&& range)
    {
        items.reserve(items.size() + range.size());

        beginInsertRows(QModelIndex(), (int)items.size(),
                        items.size() + range.size() - 1);

        for (auto&& item : range)
            items.emplace_back(forward_like<decltype(range)>(item));

        endInsertRows();
    }



/// itens
    std::vector<ResultItem> items;
    mutable std::map<albert::Item*, QStringList> actionsCache;

};
