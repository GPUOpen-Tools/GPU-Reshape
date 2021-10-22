#include <StateTables.h>

std::mutex						   DeviceStateTable::Mutex;
std::map<void*, DeviceStateTable*> DeviceStateTable::Table;

std::mutex							CommandStateTable::Mutex;
std::map<void*, CommandStateTable*> CommandStateTable::Table;