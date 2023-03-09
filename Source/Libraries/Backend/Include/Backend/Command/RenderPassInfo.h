#pragma once

// Backend
#include "ResourceToken.h"

// Forward declarations
struct AttachmentInfo;

struct RenderPassInfo {
    /// All color attachments
    const AttachmentInfo* attachments{nullptr};

    /// Number of color attachments
    uint32_t attachmentCount;

    /// Optional, depth attachment
    const AttachmentInfo* depthAttachment{nullptr};
};
