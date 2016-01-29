#include "stdafx.h"

//global variables
//sorry for the bad design everyone
map *global_map;
database *global_data;
sf::Vector2f *global_camera;
drawer *global_drawer;
sf::RenderWindow *global_window;
bool *global_hasFocus;
bool *global_renderThreadDone;
soundPlayer *global_sound;
unsigned int *global_musicVol;
unsigned int *global_soundVol;

unsigned int *global_songN;
sf::Music *global_song;

int main()
{
	unsigned int songN = DATA_NONE;
	global_songN = &songN;
	global_song = NULL;
	bool renderThreadDone = false;
	global_renderThreadDone = &renderThreadDone;
	bool hasFocus = true;
	global_hasFocus = &hasFocus;
	sf::Vector2f camera;
	global_camera = &camera;

	unsigned int musicVol = 100;
	unsigned int soundVol = 100;
	global_musicVol = &musicVol;
	global_soundVol = &soundVol;

	global_map = NULL;

	database data;
	if (data.valid)
		global_data = &data;
	else
		return -1; //the database failed to load, so you're done for

	drawer draw;
	if (!draw.valid)
		return -2; //the drawer failed to load, so you're done for
	global_drawer = &draw;

	soundPlayer sound;
	if (!sound.valid)
		return -3; //the sound failed to load
	global_sound = &sound;

    sf::RenderWindow window(sf::VideoMode(SCREENWIDTH, SCREENHEIGHT), "WPRIN!");	
	global_window = &window;

	//initialize random
	srand ((unsigned int) time(NULL));

    menu();

	std::cout << "Shutting down" << std::endl;

    return 0;
}

//utility functions
sf::Color makeColor(unsigned int num)
{
	return sf::Color(global_data->getValue(DATA_COLOR, num, 0), global_data->getValue(DATA_COLOR, num, 1), global_data->getValue(DATA_COLOR, num, 2),
						global_data->getValue(DATA_COLOR, num, 3));
}

void playSong (unsigned int songN)
{
	if (songN != *global_songN)
	{
		*global_songN = songN;

		//play the song
		if (global_song != NULL)
			global_song->stop();
		else
			global_song = new sf::Music();
		if (!global_song->openFromFile("sound/" + global_data->getLine(global_data->getValue(DATA_SONG, songN, 0))))
			return; //just don't play it
		global_song->setLoop(true);
		songVolAdjust();
		global_song->play();
	}
}

void songVolAdjust()
{
	global_song->setVolume(*global_musicVol * 1.0f);
}

unsigned int specialCharacterNum (char c)
{
	switch (c)
	{
	case '$':
		return 0;
	case '(':
		return 1;
	case ')':
		return 2;
	case '.':
		return 3;
	case ':':
		return 4;
	case '+':
		return 5;
	case '^':
		return 6;
	case '-':
		return 7;
	default:
		return DATA_NONE;
	}
}

unsigned int textWidth(std::string text)
{
	unsigned int width = 0;
	for (unsigned int i = 0; i < text.length(); i++)
	{
		char c = text.at(i);
		if (c == ' ')
			width += WHITESPACE;
		else if (c >= '0' && c <= '9' || specialCharacterNum(c) != DATA_NONE)
			width += global_data->getValue(DATA_SHEET, global_data->getValue(DATA_LIST, 0, 0), 1);
		else
			width += global_data->getValue(DATA_SHEET, global_data->getValue(DATA_LIST, 0, 4), 1);
	}
	return width;
}

void renderText(drawer *draw, std::string text, sf::Color color, float x, float y, bool centered, unsigned int maxWidth)
{
	unsigned int numberFont = global_data->getValue(DATA_LIST, 0, 0);
	unsigned int letterFont = global_data->getValue(DATA_LIST, 0, 4);
	unsigned int numberWidth = global_data->getValue(DATA_SHEET, numberFont, 1);
	unsigned int numberHeight = global_data->getValue(DATA_SHEET, numberFont, 2);
	unsigned int letterWidth = global_data->getValue(DATA_SHEET, letterFont, 1);
	unsigned int letterHeight = global_data->getValue(DATA_SHEET, letterFont, 2);

	std::vector<std::string> lines;
	if (maxWidth == 0)
		lines.push_back(text); //just one line
	else
	{
		bool badCut = false;
		std::string remaining = text;
		while (remaining.length() > 0)
		{
			bool cutLast = false;
			std::string line = "";
			for (unsigned int i = 0; i < remaining.length(); i++)
			{
				if (remaining.at(i) == '^')
				{
					line += " ";
					cutLast = true;
					break; //a cut!
				}
				line += remaining.at(i);
				if (textWidth(line) > maxWidth)
				{
					if (badCut)
					{
						badCut = false;
						break; //just break immediately
					}

					//go backwards until you reach a space
					while (true)
					{
						i -= 1;
						line = line.substr(0, line.length() - 1);
						if (i == 0)
							break;
						else if (remaining.at(i) == ' ')
						{
							cutLast = true;
							break;
						}
					}
					break;
				}
			}

			if (line.length() == 0)
				badCut = true; //don't be as picky next time
			else
			{
				remaining = remaining.substr(line.length(), remaining.length());
				if (cutLast)
					line = line.substr(0, line.length() - 1);
				lines.push_back(line);
			}
		}
	}

	//center
	if (centered)
	{
		unsigned int width = 0;
		for (unsigned int i = 0; i < lines.size(); i++)
		{
			unsigned int w = textWidth(lines[i]);
			if (w > width)
				width = w;
		}
		x -= width / 2;
		y -= lines.size() * numberHeight / 2;
	}

	//draw the lines
	unsigned int hOn = 0;
	for (unsigned int i = 0; i < lines.size(); i++)
	{
		unsigned int wOn = 0;
		for (unsigned int j = 0; j < lines[i].length(); j++)
		{
			char c = lines[i].at(j);
			if (c == ' ')
				wOn += WHITESPACE;
			else if (c >= '0' && c <= '9' || specialCharacterNum(c) != DATA_NONE)
			{
				unsigned int frame;
				sf::Color colorF;
				if (specialCharacterNum(c) != DATA_NONE)
				{
					frame = global_data->getValue(DATA_SPECIALCHARACTER, specialCharacterNum(c), 0);
					unsigned int colorN = global_data->getValue(DATA_SPECIALCHARACTER, specialCharacterNum(c), 1);
					if (colorN == DATA_NONE)
						colorF = color;
					else
						colorF = makeColor(colorN);
				}
				else
				{
					frame = c - '0';
					colorF = color;
				}
				draw->renderSprite(numberFont, frame, x + wOn, y + hOn, false, colorF);
				wOn += numberWidth;
			}
			else
			{
				draw->renderSprite(letterFont, c - 'A', x + wOn, y + hOn + (numberHeight - letterHeight) / 2, false, color);
				wOn += letterWidth;
			}
		}
		hOn += numberHeight;
	}
}

void renderNumber(drawer *draw, unsigned int number, sf::Color color, float x, float y)
{
	unsigned int numberFont = global_data->getValue(DATA_LIST, 0, 0);

	unsigned int width = 0;
	unsigned int nWidth = global_data->getValue(DATA_SHEET, numberFont, 1);
	unsigned int height = global_data->getValue(DATA_SHEET, numberFont, 2);
	unsigned int tNumber = number;
	while (tNumber > 0)
	{
		width += nWidth;
		tNumber /= 10;
	}
	tNumber = number;
	unsigned int dOn = 1;
	while (tNumber > 0)
	{
		draw->renderSprite(numberFont, tNumber % 10, x + (width / 2) - nWidth * dOn, y - (height / 2), false, color);
		dOn += 1;
		tNumber /= 10;
	}
}