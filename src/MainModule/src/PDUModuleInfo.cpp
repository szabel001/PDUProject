#include "PDUModuleInfo.h"

uint64_t PDUModuleInfo::getID()
{
    return 5;//ESP.getEfuseMac();
}

void PDUModuleInfo::setID(uint16_t _id)
{
    ID = _id;
}