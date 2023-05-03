#pragma once

// Backend
#include "ResourceToken.h"

enum class AttachmentAction {
    None,
    Load,
    Store,
    Clear,
    Discard,
    Keep,
    Resolve
};
