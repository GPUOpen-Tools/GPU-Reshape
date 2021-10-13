#include <DispatchTables.h>

std::mutex InstanceDispatchTable::Mutex;
std::map<void*, InstanceDispatchTable> InstanceDispatchTable::Table;

std::mutex DeviceDispatchTable::Mutex;
std::map<void*, DeviceDispatchTable*> DeviceDispatchTable::Table;