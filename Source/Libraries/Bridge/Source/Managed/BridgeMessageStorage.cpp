// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

#include <Bridge/Managed/BridgeMessageStorage.h>
#include <Bridge/RemoteClientBridge.h>

// Common
#include <Common/Registry.h>

Bridge::CLR::BridgeMessageStorage::BridgeMessageStorage(::IMessageStorage* storage) : storage(storage)
{

}

Bridge::CLR::BridgeMessageStorage::~BridgeMessageStorage()
{

}

void Bridge::CLR::BridgeMessageStorage::AddStream(Message::CLR::IMessageStream^ stream)
{
	// Translate schema
	MessageSchema schema;
	schema.id = stream->GetSchema().id;
	schema.type = static_cast<MessageSchemaType>(stream->GetSchema().type);

	// Get memory span
	Message::CLR::ByteSpan span = stream->GetSpan();

	// Convert to native stream
	MessageStream native;
	native.SetSchema(schema);
	native.SetVersionID(stream->GetVersionID());
	native.SetData(span.Data, span.Length, stream->GetCount());
	storage->AddStream(native);
}
