#pragma once
#include "drawer.h"
#include "item.h"
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>

#define RUMBLESTRENGTH 3
#define RUMBLELENGTH 0.35f
#define DEADCOLOR sf::Color(65, 65, 65)
#define GRAVITY 1100
#define TERMINALVEL 500
#define JUMPFORCE -500
#define BOBRATE 0.8f
#define WALKRATE 3.5f
#define MOVECHUNK 30.0f
#define ANIMSLOW 0.4f
#define MAXANGLEDIF 0.7f
#define BASEMAXAMMO 8
#define SPEEDRANDOMF 0.05f
#define SUMMONEFFECTLENGTH 0.5f
#define SUMMONEFFECTFADE 0.1f
#define SUMMONEFFECTSPEED 20
#define SUMMONEFFECTSTROBE 0.05f
#define SUMMONEFFECTSTROBEC1 sf::Color(255, 0, 0, 200)
#define SUMMONEFFECTSTROBEC2 sf::Color(0, 255, 0, 200)

//death rewards
#define DREWARD_EXPERIENCEBASE 20
#define DREWARD_EXPERIENCERAMP 8
#define DREWARD_EXPERIENCECOSTBASE 50
#define DREWARD_EXPERIENCECOSTRAMP 60
#define DREWARD_EXPERIENCECOSTEXP 5
#define DREWARD_METER 0.1f

//drops
#define DROPS_CHANCE 75
#define DROPS_RARECHANCE 20

//deathsmoke
#define DSMOKE_SMOKEPERAREA 0.0625f
#define DSMOKE_LEVELRAMP 0.03f

//knockback
#define KNOCK_HEALTHF 1.0f
#define KNOCK_DAMAGEF 0.6f
#define KNOCK_FSTRENGTH 300.0f
#define KNOCK_DECAY 800.0f

//blood effect
#define BLOODBASE 3
#define BLOODRAMP 1
#define BLOODRADIUS 5.0f

//attack animation blocking
#define AANIM_MELEE_STARTF 0
#define AANIM_MELEE_IDLEF 2
#define AANIM_MELEE_PHASE1E 0.25f
#define AANIM_MELEE_PHASE1F 0
#define AANIM_MELEE_PHASE2E 0.45f
#define AANIM_MELEE_PHASE2F 1
#define AANIM_MELEE_PHASE3E 0.7f
#define AANIM_MELEE_PHASE3F 2
#define AANIM_MELEE_PHASE4F 3
#define AANIM_MELEE_HOPPOINT AANIM_MELEE_PHASE2E
#define AANIM_MELEE_POWPOINT AANIM_MELEE_PHASE3E

#define AANIM_CROSSBOW_STARTF 4
#define AANIM_CROSSBOW_LOADEDF 0
#define AANIM_CROSSBOW_IDLEF 1
#define AANIM_CROSSBOW_LOADINGF 2

#define AANIM_BOW_STARTF 7
#define AANIM_BOW_IDLEF 0
#define AANIM_BOW_DRAW1F 1
#define AANIM_BOW_DRAW2F 2
#define AANIM_BOW_FULLF 3
#define AANIM_BOW_MINDRAW 0.6f

#define AANIM_NONE_STARTF 15

class creature
{
	//animation variables
	float bob, walk, attackAnim, rumble;
	int bobD;
	bool loaded, loaded2;

	//appearance
	std::vector<sf::Color> colors;
	std::vector<unsigned int> parts;
	unsigned int glow;
	bool gender;

	//misc stats
	unsigned int race, maskWidth, maskHeight, line;
	unsigned short type;
	bool knockbackImmune, invincible, merchant, hurtSelf;

	//primary stats
	unsigned int maxHealth, baseAttack, baseDefense, baseMoveSpeed;

	//resources
	bool doubleJump;

	//effects
	float invisibility, slow, paralyzed;
	bool wolfPearl;

	bool collideAt(float x, float y, int yDir);
	bool moveInner(float amount, float *axis);
	void boundInner();
	void meleePow();
	void rangedPow(float mult = 1);
	unsigned int getPartFromList(unsigned int list, unsigned int lN = DATA_LIST);
	void miscInit();
	void foodCost(float cost);
protected:
	//calculated stats
	unsigned int attack();
	unsigned int defense();
	unsigned int weaponMagic();
	float attackInterval();

	//position, speed, and misc AI variables
	float x, y, ySpeed, lastX, knockback, summonEffectTimer;
	int facingX, movingX;
	bool dropMode;
	bool flying;
	bool interruptImmune;
	unsigned int chargeD;

	//equipment
	unsigned int weapon, weaponMaterial;
	unsigned int weapon2, weaponMaterial2;
	unsigned int armor, armorMaterial;
	unsigned int hat, magicItem;

	//resources
	unsigned int health, experience, level, perkPoints, ammo, money;
	float food, mana;
	//perk stats
	unsigned int efficiency, maxAmmo, soulArrow, venemous;

	//perk functions
	bool perkValid(unsigned int perk);
	void perkApply(unsigned int perk);

	//misc utility functions
	bool isWalking() { return walk != -1; }
	bool canHoldAttack();

	void initInner(float _x, float _y, unsigned int cType, bool boss);
	bool move(int xDir, float elapsed);
	bool flyMove(float xDir, float yDir, float elapsed);
	void jump(float mult = 1.0f);
	void startAttack();
	void endAttack();
	void switchWeapon();
	void receivePacketInner(sf::Packet *inPacket, bool always = false);
	bool isOnscreen();
	bool weaponMelee();
	unsigned int weaponReach();
	bool bowDrawn() { return attackAnim == 1; }
	bool injured() { return health < maxHealth; }
	float healthFraction() { return 1.0f * health / maxHealth; }
	void useMagic();
	unsigned int toNextLevel();
	bool addAmmo(unsigned int amount);
	bool hasAmmo() { return weaponMelee() || ammo > 0 || soulArrow > 0; }
	bool hasRanged();
	unsigned int getRace() { return race; }
	bool isParalyzed() { return paralyzed > 0; }
	void setAttackAnim(float newAA) { attackAnim = newAA; }
	bool walking() { return walk != -1; }
	void silentDeath();
	void giantPoof();
public:
	creature(std::vector<unsigned int> *data);
	creature(float x, float y, unsigned short type, unsigned int cType, bool boss);
	creature(sf::Packet *loadFrom, unsigned short type);
	~creature();
	void save (sf::Packet *saveTo);
	virtual void update(float elapsed);
	void updateAnimOnly(float elapsed);
	virtual void render(drawer *draw, float xAt = -DATA_NONE, float yAt = -DATA_NONE);
	virtual void sendPacket(sf::Packet *outPacket, bool always = false);
	virtual void receivePacket(sf::Packet *inPacket) {}
	void pollCamera (unsigned int *checkCounter);
	void takeHit (unsigned int damage, unsigned int attack, int knockDir, float hitX, float hitY, bool hitSound = true);
	virtual void drop() {};

	//misc accessors
	unsigned int getType() { return type; }
	float getX() { return x; }
	float getY() { return y; }
	unsigned int getMaskWidth() { return maskWidth; }
	unsigned int getMaskHeight() { return maskHeight; }
	bool dead() { return health == 0; }
	bool collideProjectile(float xP, float yP) { return (xP >= x && xP <= x + maskWidth && yP >= y && yP <= y + maskHeight); }
	virtual bool projectileInteract() { return true; }
	virtual bool active() { return true; }
	bool attacking() { return attackAnim != -1; }
	bool canMeleeHit(creature *other);
	unsigned int moveSpeed ();
	virtual bool makeDamageNum () { return true; }
	bool grounded();
	void teleport(float xTo, float yTo) { x = xTo; y = yTo - maskHeight; }
	void bloodEffect(float bloodX, float bloodY, unsigned int magnitude);
	bool getItem(item *it);
	std::vector<item *> inventory;
	bool hopCue(creature *vs) { return (attackAnim > AANIM_MELEE_HOPPOINT && canMeleeHit(vs)); }
	void forceShoot (unsigned int damage, unsigned int attack, unsigned int projectileType, unsigned int projectileMagic, bool ven = false, bool aim = true);
	void getEXP(unsigned int amount);
	void heal (unsigned int amount);
	bool invisible() { return invisibility > 0; }
	void applyEffect(unsigned int effect);
	void takeBob (creature *other) { bob = other->bob; bobD = other->bobD; }
	void givePlayerInventory (unsigned int cType);
	virtual unsigned int getLine() { return line; }
	virtual bool dialogueActive() { return false; }
	void summonEffect();
	bool isInvincible() { return invincible || summonEffectTimer > 0; }
	bool isMerchant() { return merchant; }
	float projectileStartX();
	float projectileStartY();
	int getFacingX() { return facingX; }
	bool summonEffectActive() { return summonEffectTimer > 0; }
};