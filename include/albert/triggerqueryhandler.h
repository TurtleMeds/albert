// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <QString>
#include <albert/export.h>
#include <albert/extension.h>
#include <albert/item.h>
#include <albert/triggerquery.h>

namespace albert
{

///
/// Abstract trigger query handler extension.
///
/// Extensions of this type are used by the trigger query execution to provide results for triggered
/// queryies.
///
class ALBERT_EXPORT TriggerQueryHandler : virtual public Extension
{
public:
    ///
    /// Returns the input hint to be displayed on empty query.
    ///
    /// The base class implementation returns an empty string.
    ///
    virtual QString synopsis(const QString &query) const;

    ///
    /// Returns `true` if the user is allowed to set a custom trigger, otherwise returns `false`.
    ///
    /// The base class implementation returns `true`.
    ///
    virtual bool allowTriggerRemap() const;

    ///
    /// Returns the default trigger.
    ///
    /// The base class implementation returns id().
    ///
    virtual QString defaultTrigger() const;

    ///
    /// Notifies about changes to the user defined `trigger` used to call the handler.
    ///
    /// The base class implementation does nothing.
    ///
    virtual void setTrigger(const QString &trigger);

    ///
    /// Return `true` if the handler supports error tolerant matching, otherwise returns `false`.
    ///
    /// The base class implementation returns `false`.
    ///
    virtual bool supportsFuzzyMatching() const;

    ///
    /// Sets the error tolerant matching to `enabled`.
    ///
    /// The base class implementation does nothing.
    ///
    virtual void setFuzzyMatching(bool enabled);

    ///
    /// Handles the triggered `query`.
    ///
    /// @note Executed in a worker thread.
    virtual void handleTriggerQuery(TriggerQuery &query) = 0;

protected:
    ///
    /// Destructs the trigger query handler.
    ///
    ~TriggerQueryHandler() override;

};

}
