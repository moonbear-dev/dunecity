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

#include <globals.h>

#include <CutScenes/Finale.h>
#include <CutScenes/WSAVideoEvent.h>
#include <CutScenes/HoldPictureVideoEvent.h>
#include <CutScenes/FadeOutVideoEvent.h>
#include <CutScenes/FadeInVideoEvent.h>
#include <CutScenes/CrossBlendVideoEvent.h>
#include <CutScenes/TextEvent.h>
#include <CutScenes/CutSceneSoundTrigger.h>
#include <CutScenes/CutSceneMusicTrigger.h>

#include <FileClasses/FileManager.h>
#include <FileClasses/TextManager.h>
#include <FileClasses/Pakfile.h>
#include <FileClasses/Wsafile.h>
#include <FileClasses/Cpsfile.h>
#include <FileClasses/Vocfile.h>
#include <FileClasses/IndexedTextFile.h>

#include <misc/draw_util.h>
#include <misc/sound_util.h>

#include <string>

Finale::Finale(int house) {

    // DuneCity 1.0.253: Tornie mod only — per-house opponent rotation
    // for the Emperor / first-opponent / second-opponent colours shown
    // in the cinematic finale.  See kCinematicOpponents[] below.  The
    // palette indices (PALCOLOR_*) are house-invariant, but the WSA
    // assets only exist for HARKONNEN, ATREIDES, and ORDOS — non-HAO
    // houses reuse the ATREIDES WSAs while keeping their own colours.
    const int cutsceneHouse = (house == HOUSE_NEUTRAL) ? HOUSE_ORDOS : house;

    // Per-house cinematic palette: which cinematic WSA set to play.
    // Houses without their own WSA assets fall back to ATREIDES
    // since its palace WSA has the most frames + longest dialogue.
    int cinematicAssetHouse = cutsceneHouse;
    if(cinematicAssetHouse != HOUSE_HARKONNEN
       && cinematicAssetHouse != HOUSE_ATREIDES
       && cinematicAssetHouse != HOUSE_ORDOS) {
        cinematicAssetHouse = HOUSE_ATREIDES;
    }

    switch(cinematicAssetHouse) {
        case HOUSE_HARKONNEN: {
            pPalace1 = std::make_unique<Wsafile>(pFileManager->openFile("HFINALA.WSA").get());
            pPalace2 = std::make_unique<Wsafile>(pFileManager->openFile("HFINALB.WSA").get(), pFileManager->openFile("HFINALC.WSA").get());
        } break;

        case HOUSE_ATREIDES: {
            pPalace1 = std::make_unique<Wsafile>(pFileManager->openFile("AFINALA.WSA").get());
            pPalace2 = std::make_unique<Wsafile>(pFileManager->openFile("AFINALB.WSA").get());
        } break;

        case HOUSE_ORDOS: {
            pPalace1 = std::make_unique<Wsafile>(pFileManager->openFile("OFINALA.WSA").get(), pFileManager->openFile("OFINALB.WSA").get(), pFileManager->openFile("OFINALC.WSA").get());
            pPalace2 = std::make_unique<Wsafile>(pFileManager->openFile("OFINALD.WSA").get());
        } break;

        default: {
            // Should be unreachable; cinematicAssetHouse is only set to
            // HARKONNEN, ATREIDES, or ORDOS above.
        } break;
    }

    if(cinematicAssetHouse == HOUSE_HARKONNEN || cinematicAssetHouse == HOUSE_ATREIDES || cinematicAssetHouse == HOUSE_ORDOS) {
        pImperator = std::make_unique<Wsafile>(pFileManager->openFile("EFINALA.WSA").get());
        pImperatorShocked = std::make_unique<Wsafile>(pFileManager->openFile("EFINALB.WSA").get());
    }

    auto pPlanetDuneNormalSurface = LoadCPS_RW(pFileManager->openFile("BIGPLAN.CPS").get());
    if(pPlanetDuneNormalSurface == nullptr) {
        THROW(std::runtime_error, "Finale::Finale(): Cannot open BIGPLAN.CPS!");
    }

    auto pPlanetDuneInHouseColorSurface = LoadCPS_RW(pFileManager->openFile("MAPPLAN.CPS").get());
    if(pPlanetDuneInHouseColorSurface == nullptr) {
        THROW(std::runtime_error, "Finale::Finale(): Cannot open MAPPLAN.CPS!");
    }
    pPlanetDuneInHouseColorSurface = mapSurfaceColorRange(pPlanetDuneInHouseColorSurface.get(), houseToPaletteIndex[HOUSE_HARKONNEN], houseToPaletteIndex[house]);

    if(cinematicAssetHouse == HOUSE_HARKONNEN || cinematicAssetHouse == HOUSE_ATREIDES || cinematicAssetHouse == HOUSE_ORDOS) {
        lizard = getChunkFromFile("LIZARD1.VOC");
        glass = getChunkFromFile("GLASS6.VOC");
        click = getChunkFromFile("CLICK.VOC");
        blaster = getChunkFromFile("BLASTER.VOC");
        blowup = getChunkFromFile("BLOWUP1.VOC");
    }

    const auto pIntroText = std::make_unique<IndexedTextFile>(pFileManager->openFile("INTRO." + _("LanguageFileExtension")).get());

    // DuneCity 1.0.253: per-cinematic-house colour override.
    // The existing vanilla code hardcoded the Emperor colour to
    // PALCOLOR_SARDAUKAR for every cinematic.  The user spec for
    // 1.0.253 maps each cutscene house to (opponent1, opponent2,
    // emperor), and we pull those three colours off the global
    // palette via houseToPaletteIndex[].  The reb-grey Custom_IBM.pal
    // override already in 1.0.251 will rewrite index 192–199 of the
    // palette so Rebels-related colours come out grey automatically.
    static constexpr struct { int opponent1; int opponent2; int emperor; } kCinematicOpponents[NUM_HOUSES] = {
        /* HARKONNEN */  { HOUSE_SARDAUKAR, HOUSE_MERCENARY, HOUSE_ATREIDES   },
        /* ATREIDES  */  { HOUSE_MERCENARY, HOUSE_ORDOS,    HOUSE_HARKONNEN  },
        /* ORDOS     */  { HOUSE_FREMEN,    HOUSE_NEUTRAL,  HOUSE_SARDAUKAR  },
        /* FREMEN    */  { HOUSE_ORDOS,     HOUSE_HARKONNEN,HOUSE_MERCENARY  },
        /* SARDAUKAR */  { HOUSE_NEUTRAL,   HOUSE_ORDOS,    HOUSE_HARKONNEN  },
        /* MERCENARY */  { HOUSE_HARKONNEN, HOUSE_FREMEN,   HOUSE_NEUTRAL    },
        /* NEUTRAL   */  { HOUSE_ATREIDES,  HOUSE_SARDAUKAR,HOUSE_ORDOS      },
        /* REBELS    */  { HOUSE_ATREIDES,  HOUSE_HARKONNEN,HOUSE_ORDOS      },
    };
    const int csc = cutsceneHouse;
    const Uint32 playerColor = SDL2RGB(palette[houseToPaletteIndex[csc]+1]);
    const Uint32 opponent1Color = SDL2RGB(palette[houseToPaletteIndex[kCinematicOpponents[csc].opponent1]+1]);
    const Uint32 opponent2Color = SDL2RGB(palette[houseToPaletteIndex[kCinematicOpponents[csc].opponent2]+1]);
    const Uint32 emperorColor   = SDL2RGB(palette[houseToPaletteIndex[kCinematicOpponents[csc].emperor]+1]);

    // marker: previous sardaukarColor removed, replaced by per-cinematic colours.
    (void)opponent2Color; // Reserved for follow-up: a third dialogue scene shifting colour to opponent2.

    switch(cinematicAssetHouse) {
        case HOUSE_HARKONNEN: {
            startNewScene();

            addVideoEvent(std::make_unique<FadeInVideoEvent>(pPalace1->getPicture(0).get(), 20));
            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pPalace1->getPicture(0).get(), 25));
            addVideoEvent(std::make_unique<WSAVideoEvent>(pPalace1.get()));
            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pPalace1->getPicture(0).get(), 52));
            addVideoEvent(std::make_unique<WSAVideoEvent>(pPalace1.get()));
            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pPalace1->getPicture(0).get(), 23));
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_You_are_indeed_not_entirely),playerColor,22,47,false,true,false));
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_You_have_lied_to_us),playerColor,70,60,true,true,false));
            addTrigger(std::make_unique<CutSceneMusicTrigger>(0,MUSIC_FINALE_H));

            startNewScene();

            addVideoEvent(std::make_unique<WSAVideoEvent>(pImperator.get()));
            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pImperator->getPicture(pImperator->getNumFrames()-1).get(), 3));
            // DuneCity 1.0.253: Emperor dialogue now uses kCinematicOpponents'
            // emperor color (was PALCOLOR_SARDAUKAR regardless of cinematic house).
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_What_lies_What_are),emperorColor,2,100,false,true,false));

            startNewScene();

            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pPalace1->getPicture(0).get(), 50));
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_Your_lies_of_loyalty),opponent1Color,0,50,true,true,false));

            startNewScene();

            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pImperatorShocked->getPicture(0).get(), 45));
            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pImperatorShocked->getPicture(1).get(), 15));
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_A_crime_for_which_you),playerColor,2,38,true,false,false));
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_with_your_life),playerColor,42,100,false,false,false));

            startNewScene();

            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pPalace2->getPicture(0).get(), 5));
            addVideoEvent(std::make_unique<WSAVideoEvent>(pPalace2.get()));
            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pPalace2->getPicture(pPalace2->getNumFrames()-1).get(), 15));
            addVideoEvent(std::make_unique<FadeOutVideoEvent>(pPalace2->getPicture(pPalace2->getNumFrames()-1).get(), 20));
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_NO_NO_NOOO),emperorColor,10,30,false,true,false));
            addTrigger(std::make_unique<CutSceneSoundTrigger>(10,click.get()));
            addTrigger(std::make_unique<CutSceneSoundTrigger>(15,blaster.get()));
            addTrigger(std::make_unique<CutSceneSoundTrigger>(17,blowup.get()));
            addTrigger(std::make_unique<CutSceneSoundTrigger>(38,click.get()));
            addTrigger(std::make_unique<CutSceneSoundTrigger>(43,blaster.get()));
            addTrigger(std::make_unique<CutSceneSoundTrigger>(45,blowup.get()));
            addTrigger(std::make_unique<CutSceneSoundTrigger>(67,glass.get()));

        } break;

        case HOUSE_ATREIDES: {
            startNewScene();

            addVideoEvent(std::make_unique<FadeInVideoEvent>(pPalace1->getPicture(0).get(), 20));
            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pPalace1->getPicture(0).get(), 50));
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_Greetings_Emperor),playerColor,15,48,false,true,false));
            addTrigger(std::make_unique<CutSceneMusicTrigger>(0,MUSIC_FINALE_A));

            startNewScene();

            addVideoEvent(std::make_unique<WSAVideoEvent>(pImperator.get()));
            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pImperator->getPicture(pImperator->getNumFrames()-1).get(), 3));
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_What_is_the_meaning),emperorColor,2,100,false,false,false));

            startNewScene();

            addVideoEvent(std::make_unique<WSAVideoEvent>(pPalace1.get()));
            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pPalace1->getPicture(pPalace1->getNumFrames()-1).get(), 25));
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_You_are_formally_charged),opponent1Color,0,105,true,false,false));

            startNewScene();

            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pImperatorShocked->getPicture(0).get(), 34));
            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pImperatorShocked->getPicture(1).get(), 20));
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_The_House_shall_determine),playerColor,2,40,false,false,false));

            startNewScene();

            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pPalace2->getPicture(0).get(), 15));
            addVideoEvent(std::make_unique<WSAVideoEvent>(pPalace2.get()));
            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pPalace2->getPicture(pPalace2->getNumFrames()-1).get(), 30));
            addVideoEvent(std::make_unique<FadeOutVideoEvent>(pPalace2->getPicture(pPalace2->getNumFrames()-1).get(), 20));
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_Until_then_you_shall_no),opponent2Color,2,48,false,true,false));

        } break;

        case HOUSE_ORDOS: {
            startNewScene();

            addVideoEvent(std::make_unique<FadeInVideoEvent>(pPalace1->getPicture(0).get(), 20));
            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pPalace1->getPicture(0).get(), 50));
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_You_are_aware_Emperor),playerColor,22,46,false,true,false));
            addTrigger(std::make_unique<CutSceneMusicTrigger>(0,MUSIC_FINALE_O));

            startNewScene();

            addVideoEvent(std::make_unique<WSAVideoEvent>(pImperator.get()));
            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pImperator->getPicture(pImperator->getNumFrames()-1).get(), 3));
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_What_games_What_are_you),emperorColor,2,100,false,true,false));

            startNewScene();

            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pPalace1->getPicture(0).get(), 40));
            addVideoEvent(std::make_unique<WSAVideoEvent>(pPalace1.get()));
            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pPalace1->getPicture(pPalace1->getNumFrames()-1).get(), 65));
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_I_am_referring_to_your_game),opponent1Color,2,35,false,true,false));
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_We_were_your_pawns_and_Dune),opponent1Color,40,45,true,true,false));
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_We_have_decided_to_take),opponent2Color,88,105,true,false,false));
            addTrigger(std::make_unique<CutSceneSoundTrigger>(42,lizard.get()));
            addTrigger(std::make_unique<CutSceneSoundTrigger>(62,lizard.get()));

            startNewScene();

            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pImperatorShocked->getPicture(0).get(), 29));
            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pImperatorShocked->getPicture(1).get(), 20));
            addTextEvent(std::make_unique<TextEvent>(pIntroText->getString(FinaleText_You_are_to_be_our_pawn),playerColor,2,47,false,true,false));

            startNewScene();

            addVideoEvent(std::make_unique<WSAVideoEvent>(pPalace2.get()));
            addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pPalace2->getPicture(pPalace2->getNumFrames()-1).get(), 15));
            addVideoEvent(std::make_unique<FadeOutVideoEvent>(pPalace2->getPicture(pPalace2->getNumFrames()-1).get(), 20));

        } break;

        default: {
            // Should be unreachable; cinematicAssetHouse is only set to
            // HARKONNEN, ATREIDES, or ORDOS above.
        } break;
    }

    addVideoEvent(std::make_unique<FadeInVideoEvent>(pPlanetDuneNormalSurface.get(), 20));
    addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pPlanetDuneNormalSurface.get(), 10));
    addVideoEvent(std::make_unique<CrossBlendVideoEvent>(pPlanetDuneNormalSurface.get(), pPlanetDuneInHouseColorSurface.get()));
    addVideoEvent(std::make_unique<HoldPictureVideoEvent>(pPlanetDuneInHouseColorSurface.get(), 50));
    addVideoEvent(std::make_unique<FadeOutVideoEvent>(pPlanetDuneInHouseColorSurface.get(), 20));
    addVideoEvent(std::make_unique<HoldPictureVideoEvent>(nullptr, 10));
}

Finale::~Finale() = default;

