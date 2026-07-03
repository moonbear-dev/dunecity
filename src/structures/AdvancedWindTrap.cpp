#include <structures/AdvancedWindTrap.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>
#include <House.h>
#include <Game.h>
#include <ScreenBorder.h>
#include <mmath.h>

#include <GUI/ObjectInterfaces/WindTrapInterface.h>
#include <Definitions.h>
#include <misc/DrawingRectHelper.h>

AdvancedWindTrap::AdvancedWindTrap(House* newOwner) : StructureBase(newOwner) {
    AdvancedWindTrap::init();

    setHealth(getMaxHealth());
}

AdvancedWindTrap::AdvancedWindTrap(InputStream& stream) : StructureBase(stream) {
    AdvancedWindTrap::init();
}

void AdvancedWindTrap::init() {
    itemID = Structure_AdvancedWindTrap;
    owner->incrementStructures(itemID);

    structureSize.x = 3;
    structureSize.y = 3;

    graphicID = ObjPic_AdvancedWindTrap;
    graphic = pGFXManager->getObjPic(graphicID, getOwner()->getHouseID());
    // DuneCity 1.0.251: drop the 4-frame animation. The Tornie static sprite
    // is used as-is — no cycling between frames. House tint is not applied
    // to the building body; only the two tiny corner flags (top-left +
    // bottom-right) are drawn, and they're hardcoded to Harkonnen red as
    // a generic "house-color switching" marker (see blitToScreen()).
    numImagesX = 1;
    numImagesY = 1;
    firstAnimFrame = 0;
    lastAnimFrame = 0;
}

AdvancedWindTrap::~AdvancedWindTrap() = default;

bool AdvancedWindTrap::update() {
    bool bResult = StructureBase::update();

    if(bResult) {
        // DuneCity 1.0.251: removed per-frame animation. curAnimFrame stays
        // at the static frame set during init.
        auto* citySim = currentGame->getCitySimulation();
        if (citySim) {
            citySim->registerPowerSource(location.x, location.y, getProducedPower());
        }
    }

    return bResult;
}

void AdvancedWindTrap::blitToScreen() {
    // Draw the building via the base class (structure sprite + smoke)
    StructureBase::blitToScreen();

    if (fogged) return;

    // DuneCity 1.0.251: tiny corner flags (top-left + bottom-right) only,
    // hardcoded to Harkonnen red. Per user request, the flags are NOT
    // per-house-tinted — they serve as a generic "house-color switching
    // marker" only. The building body itself is left untinted.
    SDL_Texture* flagTex = pGFXManager->getZoomedObjPic(ObjPic_CornerFlag, HOUSE_HARKONNEN, currentZoomlevel);
    if (!flagTex) return;

    // Flag frame size is derived from the loaded texture height (supports both
    // old 7px and new 48px flag sprites).  Zoom levels scale linearly.
    int baseFlagPx = 7;  // fallback
    {
        int texW = 0, texH = 0;
        SDL_QueryTexture(flagTex, nullptr, nullptr, &texW, &texH);
        if (texH > 0) baseFlagPx = texH;
    }
    const int flagPx = baseFlagPx;
    // No animation: always draw frame 0.
    const int flagFrame = 0;

    // Building screen rect (single-frame now that the building is static)
    SDL_Rect buildDest = calcSpriteDrawingRect(graphic[currentZoomlevel],
                                                screenborder->world2screenX(lround(realX)),
                                                screenborder->world2screenY(lround(realY)),
                                                numImagesX, numImagesY);

    SDL_Rect flagSrc = { flagFrame * flagPx, 0, flagPx, flagPx };

    // Top-left corner
    SDL_Rect tlDst = { buildDest.x, buildDest.y, flagPx, flagPx };
    SDL_RenderCopy(renderer, flagTex, &flagSrc, &tlDst);

    // Bottom-right corner
    SDL_Rect brDst = { buildDest.x + buildDest.w - flagPx, buildDest.y + buildDest.h - flagPx, flagPx, flagPx };
    SDL_RenderCopy(renderer, flagTex, &flagSrc, &brDst);
}

void AdvancedWindTrap::setHealth(FixPoint newHealth) {
    int producedPowerBefore = getProducedPower();
    StructureBase::setHealth(newHealth);
    int producedPowerAfterwards = getProducedPower();

    owner->setProducedPower(owner->getProducedPower() - producedPowerBefore + producedPowerAfterwards);
}

ObjectInterface* AdvancedWindTrap::getInterfaceContainer() {
    if((pLocalHouse == owner) || (debug == true)) {
        return WindTrapInterface::create(objectID);
    } else {
        return DefaultObjectInterface::create(objectID);
    }
}

int AdvancedWindTrap::getProducedPower() const {
    int nominal = abs(currentGame->objectData.data[Structure_AdvancedWindTrap][originalHouseID].power);
    FixPoint ratio = getHealth() / getMaxHealth();
    return lround(ratio * nominal);
}
