// Copyright (c) 2022-2024 Manuel Schneider

#include "extension.h"
#include "extensionregistry.h"
#include "logging.h"
using namespace albert;
using namespace std;

bool ExtensionRegistry::registerExtension(Extension *e)
{
    auto id = e->id();
    if (id.isEmpty()){
        CRIT << "Registered extension id must not be empty";
        return false;
    }

    const auto&[it, success] = extensions_.emplace(id, e);
    if (success) {
        DEBG << QStringLiteral("Extension registered: '%1'").arg(e->id());
        emit added(e);
    }
    else
        CRIT << "Extension registered more than once:" << e->id();
    return success;
}

void ExtensionRegistry::deregisterExtension(Extension *e)
{
    if (extensions_.erase(e->id())) {
        DEBG << QStringLiteral("Extension deregistered: '%1'").arg(e->id());
        emit removed(e);
    }
    else
        CRIT << "Removed extension that has not been registered before:" << e->id();
}

const map<QString, Extension*> &ExtensionRegistry::extensions() const { return extensions_; }
