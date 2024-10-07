// SPDX-FileCopyrightText: 2024 Manuel Schneider
// SPDX-License-Identifier: MIT

#pragma once
#include <QString>
#include <albert/globalqueryhandler.h>
#include <albert/indexitem.h>
#include <memory>
#include <vector>

namespace albert
{

///
/// A GlobalQueryHandler providing implicit indexing and matching.
///
class ALBERT_EXPORT IndexQueryHandler : public GlobalQueryHandler
{
public:
    ///
    /// Returns `true`.
    ///
    bool supportsFuzzyMatching() const override;

    ///
    /// Sets the fuzzy matching mode to `enabled` and triggers updateIndexItems().
    ///
    void setFuzzyMatching(bool) override;

    ///
    /// Returns the matching items from the index.
    ///
    std::vector<RankItem> handleGlobalQuery(const Query &) override;

    ///
    /// Updates the index.
    ///
    /// Called when the index needs to be updated, i.e. for initialization, on user changes to the
    /// index config (fuzzy, etc…) and probably by the client itself if the items changed. This
    /// function should call setIndexItems(std::vector<IndexItem>&&) to update the index.
    ///
    /// @note Do not call this method in the constructor. It will be called on plugin
    /// initialization.
    ///
    virtual void updateIndexItems() = 0;

    ///
    /// Sets the items of the index.
    ///
    /// Intended to be called in updateIndexItems().
    ///
    void setIndexItems(std::vector<IndexItem> &&);

protected:
    ///
    /// Constructs an index query handler.
    ///
    IndexQueryHandler();

    ///
    /// Destructs the index query handler.
    ///
    ~IndexQueryHandler() override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

}  // namespace albert
