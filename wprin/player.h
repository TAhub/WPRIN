#pragma once
#include "creature.h"

#define INVENTORYSIZE 16

//interface constants
#define INTERFACE_BT 1
#define INTERFACE_HEALTHHEIGHT 20.0f
#define INTERFACE_HEALTHWIDTH 100.0f
#define INTERFACE_HEALTHX 20.0f
#define INTERFACE_HEALTHMINUSY 40.0f
#define INTERFACE_HEALTHCOLOR sf::Color(255, 0, 0)
#define INTERFACE_HEALTHBACKCOLOR sf::Color(200, 100, 100)
#define INTERFACE_FOODHEIGHT 20.0f
#define INTERFACE_FOODWIDTH 40.0f
#define INTERFACE_FOODX 20.0f
#define INTERFACE_FOODMINUSY 80.0f
#define INTERFACE_FOODCOLOR sf::Color(255, 130, 50)
#define INTERFACE_FOODBACKCOLOR sf::Color(200, 100, 90)
#define INTERFACE_MANAHEIGHT 20.0f
#define INTERFACE_MANAWIDTH 40.0f
#define INTERFACE_MANAX 80.0f
#define INTERFACE_MANAMINUSY 80.0f
#define INTERFACE_MANACOLOR sf::Color(0, 0, 255)
#define INTERFACE_MANABACKCOLOR sf::Color(100, 100, 200)
#define INTERFACE_EXPX 140.0f
#define INTERFACE_EXPMINUSY 40.0f
#define INTERFACE_EXPWIDTH 40.0f
#define INTERFACE_EXPHEIGHT 20.0f
#define INTERFACE_EXPCOLOR sf::Color(70, 70, 235)
#define INTERFACE_EXPACTIVECOLOR sf::Color(210, 210, 255)
#define INTERFACE_EXPBACKCOLOR sf::Color(100, 100, 200)
#define INTERFACE_AMMOX 200.0f
#define INTERFACE_AMMOMINUSY 50.0f
#define INTERFACE_MAGICITEMXADD 20.0f
#define INTERFACE_MAGICITEMMINUSY 40.0f
#define INTERFACE_PERKDESCRIPTIONWIDTH 300.0f

#define INTERFACE_ACTIVECOLOR sf::Color(215, 190, 190)
#define INTERFACE_INACTIVECOLOR sf::Color(135, 130, 130)
#define INTERFACE_SWOOPSPEED 2.5f
#define INTERFACE_PERKX 300.0f
#define INTERFACE_NUMPERKS 3
#define INTERFACE_INVH 40
#define INTERFACE_INVW 400
#define INTERFACE_TIPUP 100
#define INTERFACE_TIPW 550

#define INTERFACE_LINEMINUSY 150
#define INTERFACE_LINEBORDERX 250
#define INTERFACE_DNUMCOLOR sf::Color(255, 0, 0)
#define INTERFACE_HNUMCOLOR sf::Color(0, 255, 0)

//food costs
#define FOOD_ATTACKCOST 0.0025f
#define FOOD_WALKCOST 0.005f
#define FOOD_JUMPCOST 0.0015f
#define FOOD_STARVINGSLOW 0.3f
#define FOOD_STARVINGATTACKSLOW 0.7f

#define REVIVELENGTH 4.5f
#define TALKDISTANCE 50
#define TALKLENGTHBASE 1.0f
#define TALKLENGTHRAMP 0.05f
#define SELLVALUE 0.55f

class player: public creature
{
	//keyboard info
	bool lastJump, lastAttack, lastSwitch, lastI, lastK, lastArrow, lastMagic, lastDrop;

	//interface info
	int itemsD, craftingD, levelD;
	unsigned int interfaceX, interfaceY;
	float interfaceTimer;
	std::vector<unsigned int> interfaceData;

	//line
	unsigned int line;
	float lineTimer;

	//super special player ability power
	float reviveTimer;

	void talk();
	bool pickUp();
	void generateApplicableCrafts ();
	bool hasItemsForRecipie (unsigned int y);
	item getCraftingExample (unsigned int x, unsigned int y);
	sf::Vector2f renderInventoryBit (unsigned int x, unsigned int y, drawer *draw);
	void renderCraftingBit (unsigned int x, unsigned int y, drawer *draw);
	void renderLevelBit (unsigned int y, drawer *draw);
	void itemsMode (float elapsed);
	void craftingMode (float elapsed);
	void moveMode (float elapsed);
	void levelMode (float elapsed);
	void getValidPerks ();
	void playerInit();
	bool onMerchant();
	item *buyableItemOn();
	void renderBar (drawer *draw, float x, float y, float width, float height, float percent, sf::Color colorFront, sf::Color colorBack);
	void renderInfoTip (drawer *draw, std::string info);
public:
	player(float x, float y);
	player(sf::Packet *loadFrom, unsigned short type);
	player(std::vector<unsigned int> *data);
	void update(float elapsed);
	void render(drawer *draw, float xAt = -DATA_NONE, float yAt = -DATA_NONE);

	bool dialogueActive() { return line != DATA_NONE; }
	void forceDialogue(unsigned int id);
};