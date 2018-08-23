#include "PktGameStateDriver.h"
#include "CodalDmesg.h"
#include "Timer.h"

using namespace codal;

PktArcadeDevice::PktArcadeDevice(PktDevice d, uint8_t playerNumber, GameEngine& ge) :
                 PktSerialDriver(d, PKT_DRIVER_CLASS_ARCADE, DEVICE_ID_PKT_ARCADE_HOST)
{
    playerNumber = playerNumber;
    next = NULL;
}

void PktArcadeDevice::handleControlPacket(ControlPacket* cp)
{

}

void PktArcadeDevice::handlePacket(PktSerialPkt* p)
{

}