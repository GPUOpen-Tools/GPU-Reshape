#pragma once

// Message
#include <Message/Message.h>

// Std
#include <cstdint>

/// Shader export type metadata
struct ShaderExportTypeInfo {
    template<typename T>
    static ShaderExportTypeInfo FromType() {
        using ShaderExport = typename T::ShaderExport;
        using Schema = typename T::Schema;

        ShaderExportTypeInfo info{};
        info.messageSchema = Schema::GetSchema(T::kID);
        info.noSGUID = ShaderExport::kNoSGUID;
        info.structured = ShaderExport::kStructured;
        return info;
    }

    MessageSchema messageSchema{};
    bool noSGUID{false};
    bool structured{false};
};
