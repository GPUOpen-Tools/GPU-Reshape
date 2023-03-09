#pragma once

// Backend
#include "ResourceInfo.h"
#include "AttachmentAction.h"

struct AttachmentInfo {
    /// Given resource
    ResourceInfo resource;

    /// Data action for given resource
    AttachmentAction action;
};
