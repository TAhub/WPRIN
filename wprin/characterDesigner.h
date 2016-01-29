#pragma once
#include "player.h"
#include "drawer.h"


class characterDesigner
{
	player *cr;
	unsigned int y;
	bool lastD, lastSpace;

	std::vector<unsigned int> data;
	void genC();
public:
	characterDesigner ();
	~characterDesigner ();

	bool done;

	void update(float elapsed);
	void render(drawer *draw);

	player *getPlayer() { return cr; }
};