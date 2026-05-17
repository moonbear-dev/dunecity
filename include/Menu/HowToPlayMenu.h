/*
 *  This file is part of Dune City.
 *
 *  Licensed under GPL-2.0-or-later.
 */

#ifndef HOWTOPLAYMENU_H
#define HOWTOPLAYMENU_H

#include "MenuBase.h"
#include <GUI/StaticContainer.h>
#include <GUI/PictureLabel.h>
#include <GUI/Label.h>
#include <GUI/TextButton.h>
#include <GUI/TextView.h>

class HowToPlayMenu : public MenuBase
{
public:
    HowToPlayMenu();
    ~HowToPlayMenu() override;

    HowToPlayMenu(const HowToPlayMenu&) = delete;
    HowToPlayMenu(HowToPlayMenu&&) = delete;
    HowToPlayMenu& operator=(const HowToPlayMenu&) = delete;
    HowToPlayMenu& operator=(HowToPlayMenu&&) = delete;

private:
    void onBack();

    StaticContainer windowWidget;

    PictureLabel    planetPicture;
    PictureLabel    duneLegacy;

    Label           title;
    TextView        body;
    TextButton      backButton;
};

#endif // HOWTOPLAYMENU_H
