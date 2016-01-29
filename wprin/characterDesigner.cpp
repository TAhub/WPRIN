#include "stdafx.h"

extern database *global_data;
extern bool *global_hasFocus;
extern soundPlayer *global_sound;

characterDesigner::characterDesigner () : y(0), lastD(false), lastSpace(false), done(false)
{
	cr = NULL;

	data.push_back(0);
	data.push_back(0);
	data.push_back(0);
	data.push_back(0);
	data.push_back(0);
	data.push_back(0);

	genC();
}

void characterDesigner::genC()
{
	player *oldCr = cr;
	cr = new player(&data);
	if (oldCr != NULL)
	{
		cr->takeBob(oldCr);
		delete oldCr;
	}
}

characterDesigner::~characterDesigner ()
{
	if (!done)
		delete cr;
}

void characterDesigner::update(float elapsed)
{
	if (done)
		return;

	cr->updateAnimOnly(elapsed);

	int xA = 0;
	int yA = 0;
	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::W))
		yA -= 1;
	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::S))
		yA += 1;
	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		xA -= 1;
	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		xA += 1;

	if (*global_hasFocus && sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
	{
		if (!lastSpace)
		{
			lastSpace = true;

			if (y == data.size())
			{
				global_sound->play(5);

				done = true;

				//give it it's inventory at the last moment
				cr->givePlayerInventory(data[0]);
			}
		}
	}
	else
		lastSpace = false;

	if (xA != 0 || yA != 0)
	{
		if (!lastD)
		{
			lastD = true;

			if (yA != 0)
				global_sound->play(4);
			if (yA == -1 && y == 0)
				y = data.size();
			else if (yA == 1 && y == data.size())
				y = 0;
			else if (yA != 0)
				y += yA;
			else if (y < data.size())
			{
				global_sound->play(5);

				//handle adding, hoo boy
				unsigned int max;
				unsigned int race = global_data->getValue(DATA_CTYPE, data[0], 0);
				unsigned int numParts = global_data->getValue(DATA_RACE, race, DATA_RACE_PARTNUM);
				unsigned int colors = global_data->getValue(DATA_CTYPE, data[0], 1);
				switch(y)
				{
				case 0:
					//ctyp
					max = global_data->getEntrySize(DATA_LIST, 1);
					break;
				case 1:
					//gender
					max = 2;
					break;
				case 2:
					//hat
					max = global_data->getEntrySize(DATA_LIST, 2) / 2;
					break;
				case 3:
					//skinC
					max = global_data->getEntrySize(DATA_COLORLIST, global_data->getValue(DATA_COLORLIST, colors, 0));
					break;
				case 4:
					//hair
					max = global_data->getEntrySize(DATA_LIST, global_data->getValue(DATA_RACE, race, DATA_RACE_PARTNUM + 2 * numParts));
					break;
				case 5:
					//hairC
					max = global_data->getEntrySize(DATA_COLORLIST, global_data->getValue(DATA_COLORLIST, colors, 2));
					break;
				}

				if (max != 1)
				{
					if (data[y] == 0 && xA == -1)
						data[y] = max - 1;
					else if (data[y] == max - 1 && xA == 1)
						data[y] = 0;
					else
						data[y] += xA;

					if (y == 0)
					{
						data[3] = 0;
						data[4] = 0;
						data[5] = 0;
					}

					genC();
				}
			}
		}
	}
	else
		lastD = false;
}

void characterDesigner::render(drawer *draw)
{
	cr->render(draw, SCREENWIDTH * 0.5f, SCREENHEIGHT * 0.5f - cr->getMaskHeight());

	for (unsigned int i = 0; i <= data.size(); i++)
	{
		std::string name;
		switch(i)
		{
		case 0:
			name = "ROLE  ";
			break;
		case 1:
			name = "GENDER  ";
			break;
		case 2:
			name = "HAT  ";
			break;
		case 3:
			name = "COLORATION  ";
			break;
		case 4:
			name = "HAIR STYLE  ";
			break;
		case 5:
			name = "HAIR COLOR  ";
			break;
		case 6:
			name = "DONE";
			break;
		}

		switch(i)
		{
		case 0:
			name += global_data->getLine(global_data->getValue(DATA_LIST, 1, data[i]));
			break;
		case 1:
			if (data[i] == 0)
				name += "MALE";
			else
				name += "FEMALE";
			break;
		case 2:
			name += global_data->getLine(global_data->getValue(DATA_LIST, 2, data[i] * 2 + 1));
			break;
		case 6:
			break; //nothing
		default:
			name += std::to_string((long double) data[i]);
			break;
		}

		sf::Color c;
		if (i == y)
			c = INTERFACE_ACTIVECOLOR;
		else
			c = INTERFACE_INACTIVECOLOR;
		renderText(draw, name, c, 30.0f, 40.0f * i);
	}

	//make the description text
	std::string descText;
	switch(y)
	{
	case 0:
		descText = global_data->getLine(global_data->getValue(DATA_LIST, 1, data[y]) + 1);
		break;
	case 1:
		descText = "NOBODY CARES ABOUT GENDER DOWN IN THE UNDERGROUND^THEY ARE ALL TOO BUSY TRYING TO KILL YOU";
		break;
	case 2:
		descText = global_data->getLine(global_data->getValue(DATA_LIST, 2, data[y] * 2 + 1) + 1);
		break;
	default:
		descText = "";
		break;
	}
	renderText(draw, descText, INTERFACE_ACTIVECOLOR, SCREENWIDTH / 2, SCREENHEIGHT / 2, false, SCREENWIDTH / 2);
}