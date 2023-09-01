// Copyright (c) 2023 Manuel Schneider

#pragma once
#include "albert/export.h"
#include <QString>
#include <functional>
#include <vector>

namespace albert
{

/// Action used by items.
class ALBERT_EXPORT Action final
{
public:
    // Required for ETI. See https://stackoverflow.com/questions/69556553/how-to-create-an-explicit-template-instantiation-declaration-for-stdvector-wit
    Action();
    Action(const Action&);
    Action(Action&&);
    Action &operator=(const Action&);
    Action &operator=(Action&&);

    /// Action constructor
    /// \param id Identifier of the action.
    /// \param text Description of the action.
    /// \param function The action function.
    Action(QString id, QString text, std::function<void()> function);

    QString id;  ///< Identifier of the action.
    QString text;  ///< Description of the action.
    std::function<void()> function;  ///< The action function.
};

}

// Instanciate common templates explicitly to reduce binary sizes
extern template class ALBERT_EXPORT std::vector<albert::Action>;
