// Copyright (c) 2023 Manuel Schneider

#pragma once
#include "albert/extension/pluginprovider/pluginloader.h"
#include "albert/extension/pluginprovider/pluginmetadata.h"
#include <QLoggingCategory>
using namespace albert;

class PluginInstancePrivate
{
public:
    inline static const PluginLoader *in_construction;

    PluginLoader const * const loader;
    // QLoggingCategory does not take ownership of the cstr. Keep the std::string alive.
    const std::string logging_category_name;
    const QLoggingCategory logging_category;

    PluginInstancePrivate():
        loader(in_construction),
        logging_category_name(loader->metaData().id.toUtf8().toStdString()),
        logging_category(logging_category_name.c_str(), QtDebugMsg)
    {}

};

