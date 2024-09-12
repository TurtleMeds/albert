// Copyright (c) 2022-2024 Manuel Schneider

#include "extension.h"
#include "extensionregistry.h"
#include "logging.h"
using namespace albert;
using namespace std;

bool ExtensionRegistry::registerExtension(Extension *e)
{
    auto id = e->id();
    if (id.isEmpty())
    {
        CRIT << "Registered extension id must not be empty";
        return false;
    }

    const auto&[it, success] = extensions_.emplace(id, e);
    if (success)
        emit added(e);
    else
        CRIT << "Extension registered more than once:" << e->id();
    return success;
}

void ExtensionRegistry::deregisterExtension(Extension *e)
{
    if (extensions_.erase(e->id()))
        emit removed(e);
    else
        CRIT << "Removed extension that has not been registered before:" << e->id();
}

const map<QString, Extension*> &ExtensionRegistry::extensions() const { return extensions_; }
