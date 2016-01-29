// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

//standard libraries
#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <fstream>
#include <sstream>
#define _USE_MATH_DEFINES
#include <math.h>

//my stuff
#include "creature.h"
#include "slave.h"
#include "drawer.h"
#include "player.h"
#include "map.h"
#include "database.h"
#include "projectile.h"
#include "enemy.h"
#include "damageNumber.h"
#include "particle.h"
#include "item.h"
#include "trap.h"
#include "characterDesigner.h"
#include "npc.h"
#include "soundPlayer.h"

//sfml libraries
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Network.hpp>
#include <SFML/Audio.hpp>

//constants
#define SCREENWIDTH 840
#define SCREENHEIGHT 600
#define WHITESPACE 20
#define TYPE_ENEMY 0
#define TYPE_PLAYER 1
#define TYPE_ALLY 2

//functions
sf::Color makeColor(unsigned int num);
void renderNumber(drawer *draw, unsigned int number, sf::Color color, float x, float y);
void renderText(drawer *draw, std::string text, sf::Color color, float x, float y, bool centered = false, unsigned int maxWidth = 0);
void menu();
void playSong (unsigned int songN);
void songVolAdjust();