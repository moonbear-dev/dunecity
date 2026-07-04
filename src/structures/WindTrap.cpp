/*
 *  This file is part of Dune Legacy.
 *
 *  Dune Legacy is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Dune Legacy is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Dune Legacy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <structures/WindTrap.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <House.h>
#include <Game.h>

#include <GUI/ObjectInterfaces/WindTrapInterface.h>

WindTrap::WindTrap(House* newOwner) : StructureBase(newOwner) {
    WindTrap::init();

    setHealth(getMaxHealth());
}

WindTrap::WindTrap(InputStream& stream) : StructureBase(stream) {
    WindTrap::init();
}

void WindTrap::init() {
    itemID = Structure_WindTrap;
    owner->incrementStructures(itemID);

    structureSize.x = 2;
    structureSize.y = 2;

    graphicID = ObjPic_Windtrap;
    graphic = pGFXManager->getObjPic(graphicID,getOwner()->getHouseID());
    // DuneCity 1.0.263: diagnostic logging for the vanilla Windtrap
    // crash (kept for the user's next test run). The early-return
    // guard that was here in 1.0.263.0 was reverted because it
    // skipped the numImagesX/Y/firstAnimFrame/lastAnimFrame setup
    // and broke the vanilla Windtrap render path downstream
    // (mentat menu and post-placement blits).
    SDL_Log("WindTrap::init: houseID=%d graphicID=%d graphic[0]=%p graphic[1]=%p graphic[2]=%p",
            (int)getOwner()->getHouseID(), (int)graphicID,
            (void*)graphic[0], (void*)graphic[1], (void*)graphic[2]);
    numImagesX = NUM_WINDTRAP_ANIMATIONS_PER_ROW;
    numImagesY = (2+NUM_WINDTRAP_ANIMATIONS+NUM_WINDTRAP_ANIMATIONS_PER_ROW-1)/NUM_WINDTRAP_ANIMATIONS_PER_ROW;
    firstAnimFrame = 2;
    lastAnimFrame = 2+NUM_WINDTRAP_ANIMATIONS-1;
}

WindTrap::~WindTrap() = default;

ObjectInterface* WindTrap::getInterfaceContainer() {
    if((pLocalHouse == owner) || (debug == true)) {
        return WindTrapInterface::create(objectID);
    } else {
        return DefaultObjectInterface::create(objectID);
    }
}

bool WindTrap::update() {
    bool bResult = StructureBase::update();

    if(bResult) {
        // we are still alive
        if(justPlacedTimer <= 0 || curAnimFrame != 0) {
            curAnimFrame = 2 + ((currentGame->getGameCycleCount()/8) % NUM_WINDTRAP_ANIMATIONS);
        }

        // Register as power source for DuneCity power grid
        auto* citySim = currentGame->getCitySimulation();
        if (citySim) {
            citySim->registerPowerSource(location.x, location.y, getProducedPower());
        }
    }

    return bResult;
}

void WindTrap::setHealth(FixPoint newHealth) {
    int producedPowerBefore = getProducedPower();
    StructureBase::setHealth(newHealth);
    int producedPowerAfterwards = getProducedPower();

    owner->setProducedPower(owner->getProducedPower() - producedPowerBefore + producedPowerAfterwards);
}

int WindTrap::getProducedPower() const {
    int windTrapProducedPower = abs(currentGame->objectData.data[Structure_WindTrap][originalHouseID].power);

    FixPoint ratio = getHealth() / getMaxHealth();
    return lround(ratio * windTrapProducedPower);
}
