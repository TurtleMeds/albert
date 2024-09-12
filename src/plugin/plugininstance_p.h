// Copyright (c) 2023-2024 Manuel Schneider

#pragma once
#include "plugininstance.h"
using namespace albert;
namespace albert { class PluginLoader; }

class PluginInstancePrivate
{
public:
    // Static DI, for clean interfaces and reduced boilerplate.
    inline static PluginLoader *current_loader;
    PluginLoader &loader{*current_loader};
};
