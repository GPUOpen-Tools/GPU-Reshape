// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#pragma once

// Layer
#include <Backends/DX12/Compiler/DXBC/Blocks/DXBCPhysicalBlockShaderSourceInfo.h>
#include <Backends/DX12/Compiler/IDXCompilerEnvironment.h>
#include <Backends/DX12/Compiler/DXCompilerArgument.h>

// DXC
#include <DXC/dxcapi.h>

// Std
#include <atomic>

/// Native compilation environment
struct DXBCCompilerEnvironment final : public IDXCompilerEnvironment, public IDxcIncludeHandler {
    DXBCCompilerEnvironment(IDxcLibrary* library, const DXBCPhysicalBlockShaderSourceInfo& info) : library(library), info(info) {
        for (size_t i = 0; i < info.sourceFiles.size(); i++) {
            const DXBCPhysicalBlockShaderSourceInfo::SourceFile& sourceFile = info.sourceFiles[i];

            // Normalize the path
            std::wstring path(sourceFile.filename.begin(), sourceFile.filename.end());
            NormalizePath(path);

            // If the main file, determine the main directory for relative paths
            if (i == 0) {
                if (size_t end = path.find_last_of(L'\\'); end != std::wstring::npos) {
                    mainDirectory = path.substr(0, end);
                } else {
                    mainDirectory = L"";
                }
            }
            
            fileIndices[path] = static_cast<uint32_t>(i);
        }
    }

    /// Get the primary source contents
    /// \return always valid
    std::string_view GetSourceContents() override {
        return info.sourceFiles[0].contents;
    }

    /// Enumerate all arguments
    /// \param count number of arguments
    /// \param arguments all arguments, if null, count is filled with the number of arguments
    void EnumerateArguments(uint32_t *count, DXCompilerArgument *arguments) override {
        if (arguments) {
            for (uint32_t i = 0; i < *count; i++) {
                arguments[i].name = info.sourceArgs[i].name;
                arguments[i].value = info.sourceArgs[i].value;
            }
        } else {
            *count = static_cast<uint32_t>(info.sourceArgs.size());
        }
    }

    /// Try to load a source
    /// \param pFilename filename query
    /// \param ppIncludeSource destination blob, in-place
    /// \return result code 
    HRESULT LoadSource(LPCWSTR pFilename, IDxcBlob **ppIncludeSource) override {
        // Normalize the path
        std::wstring filename = pFilename;
        NormalizePath(filename);

        // Try to find index
        auto indexIt = fileIndices.find(filename);
        if (indexIt == fileIndices.end()) {
            return E_FAIL;
        }

        // Get indexed file
        const DXBCPhysicalBlockShaderSourceInfo::SourceFile& sourceFile = info.sourceFiles[indexIt->second];

        // Try to create an in-place blob
        // Lifetime of contents guaranteed during compilation
        IDxcBlobEncoding* encoding{nullptr};
        if (HRESULT hr = library->CreateBlobWithEncodingFromPinned(
            sourceFile.contents.data(),
            static_cast<uint32_t>(sourceFile.contents.length()),
            DXC_CP_ACP,
            &encoding
        ); FAILED(hr)) {
            return hr;
        }

        // OK
        *ppIncludeSource = encoding;
        return S_OK;
    }

    /// Get the include handler
    /// \return always valid
    IDxcIncludeHandler* GetDxcIncludeHandler() override {
        return this;
    }

public:
    /// Query an interface
    HRESULT QueryInterface(const IID &riid, void **ppvObject) override {
        if (riid == __uuidof(IDxcIncludeHandler)) {
            AddRef();
            *ppvObject = static_cast<IDxcIncludeHandler*>(this);
            return S_OK;
        } else if (riid == __uuidof(IUnknown)) {
            AddRef();
            *ppvObject = static_cast<IUnknown*>(this);
            return S_OK;
        } else {
            return E_NOINTERFACE;
        }
    }

    /// Add an external reference
    ULONG AddRef() override {
        return ++users;
    }

    /// Release an external reference
    ULONG Release() override {
        return --users;
    }

    /// Get all users
    ULONG GetUsers() {
        return users;
    }

private:
    /// Normalize a path
    /// \param path path to sanitize
    void NormalizePath(std::wstring& path) {
        // Sanitize all path delims
        std::ranges::replace(path, L'/', L'\\');

        // Case insensitive OS, ignore all capitalization during indexing
        std::ranges::transform(path, path.begin(), ::tolower);

        // May be relative to root
        if (path.starts_with(L".\\")) {
            path = mainDirectory + path.substr(1);
        }
    }

private:
    /// Owning library
    IDxcLibrary* library;

    /// DXBC Block
    const DXBCPhysicalBlockShaderSourceInfo& info;

    /// All mapped indices
    std::unordered_map<std::wstring, uint32_t> fileIndices;

    /// The main directory for relative mappings
    std::wstring mainDirectory; 
    
    /// Number of external users
    std::atomic<ULONG> users{1};
};
