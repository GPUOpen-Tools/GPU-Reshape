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
	native.SetData(span.Data, span.Length, stream->GetCount());
	storage->AddStream(native);
}
