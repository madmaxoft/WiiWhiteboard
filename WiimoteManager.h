// WiimoteManager.h

// Declares the WiimoteManager class representing the singleton that manages individual Wiimote instances





#pragma once





#include <vector>
#include "Wiimote.h"





class WiimoteManager
{
public:
    static WiimoteManager & get();

		std::vector<Wiimote::Id> enumWiimotes();

protected:
    WiimoteManager();
    ~WiimoteManager();
};




