#pragma once

// Backend
#include "ResourceInfo.h"
#include "AttachmentAction.h"

struct AttachmentInfo {
    /// Given resource
    ResourceInfo resource;

    /// Data actions for given resource
    AttachmentAction loadAction;
    AttachmentAction storeAction;
};
