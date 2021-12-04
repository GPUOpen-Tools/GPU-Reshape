#pragma once

// Generator
#include "GeneratorHost.h"

// Common
#include <Common/Registry.h>

/// Plugin installation type
using PluginInstall = bool(*)(Registry* registry, GeneratorHost* host);

/// Plugin deinstallation type
using PluginUninstall = void(*)(Registry* registry);
