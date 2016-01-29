#include "stdafx.h"
#include <time.h>
#include <Windows.h>

#define MENUMUSIC 5

extern soundPlayer *global_sound;
extern database *global_data;
extern map *global_map;
extern drawer *global_drawer;
extern sf::RenderWindow *global_window;
extern bool *global_hasFocus;
//extern bool *global_renderThreadDone;
extern unsigned int *global_musicVol;
extern unsigned int *global_soundVol;

bool *lastIPPress;
//std::string *lastIP;

/*void tempRenderThreadFunction()
{
	global_window->setActive(true);
	while (true)
	{
		if (global_map == NULL)
			renderText(global_drawer, "WAITING FOR CONNECTION ON " + *lastIP, INTERFACE_ACTIVECOLOR, SCREENWIDTH / 2, SCREENHEIGHT / 2, true);
		else
			global_map->render(global_drawer);
		global_drawer->render(global_window);
		if (global_map != NULL && (global_map->ready() || global_map->endState() != DATA_NONE))
			break;
	}
	global_window->setActive(false);
	(*global_renderThreadDone) = true;
}*/
void networkThreadFunction()
{
	/*sf::Clock clock;
	while (true)
		if (clock.getElapsedTime().asSeconds() >= NETWORK_PING_INTERVAL)
		{
			global_map->networkAction();
			clock.restart();
		}*/
	while (true)
		global_map->networkAction();
}

/**
void networkUpdateReceiveFunction()
{
	sf::Clock clock;
	while (true)
		if (clock.getElapsedTime().asSeconds() >= NETWORK_PING_INTERVAL)
		{
			global_map->networkUpdateReceive();
			clock.restart();
		}
}

void networkUpdateSendFunction()
{
	sf::Clock clock;
	while (true)
		if (clock.getElapsedTime().asSeconds() >= NETWORK_PING_INTERVAL)
		{
			global_map->networkUpdateSend();
			clock.restart();
		}
}
/**/

void IPinput (std::string *IP)
{
	char c = ' ';
	bool back = sf::Keyboard::isKeyPressed(sf::Keyboard::BackSpace);

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num0))
		c = '0';
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1))
		c = '1';
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2))
		c = '2';
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3))
		c = '3';
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num4))
		c = '4';
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num5))
		c = '5';
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num6))
		c = '6';
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num7))
		c = '7';
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num8))
		c = '8';
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num9))
		c = '9';
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::SemiColon))
		c = ':';
	else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Period))
		c = '.';

	if (c != ' ' || back)
	{
		if (!(*lastIPPress))
		{
			*lastIPPress = true;

			if (back)
			{
				if (IP->length() > 0)
					(*IP) = IP->substr(0, IP->length() - 1);
			}
			else
				(*IP) += c;
		}
	}
	else
		*lastIPPress = false;
}

void menu()
{
	bool madeThread = false;
	//bool renderThread = false;
	sf::Thread networkThread(networkThreadFunction);
	//sf::Thread tempRenderThread(tempRenderThreadFunction);

	map *m = NULL;

	unsigned int menuOption = 0;
	std::string IP = "";

	bool lastSpace = false;
	bool lastArrow = false;
	bool lIPP = false;
	//std::string lIP = "";
	lastIPPress = &lIPP;
	//lastIP = &lIP;
	unsigned int endState = DATA_NONE;

	sf::Clock clock;
	sf::Time lastTime = clock.getElapsedTime();

	playSong(MENUMUSIC);

	sf::TcpSocket socket;
	while (global_window->isOpen())
    {
        sf::Event event;
        while (global_window->pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
			{
				//if (renderThread)
				//	tempRenderThread.terminate();
				std::cout << "Window closed." << std::endl;
                global_window->close();
				return;
			}
			else if (event.type == sf::Event::GainedFocus)
				(*global_hasFocus) = true;
			else if (event.type == sf::Event::LostFocus)
				(*global_hasFocus) = false;
        }

		//if the map should start
		if (m == NULL && *global_hasFocus)
		{
			if (menuOption == 2)
				IPinput(&IP);

			int dir = 0;
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
				dir -= 1;
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
				dir += 1;
			if (dir != 0 && endState == DATA_NONE)
			{
				if (!lastArrow)
				{
					lastArrow = true;

					if (menuOption == 0 && dir == -1)
						menuOption = 5;
					else if (menuOption == 5 && dir == 1)
						menuOption = 0;
					else
						menuOption += dir;
					global_sound->play(4);
				}
			}
			else
				lastArrow = false;

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
			{
				if (!lastSpace)
				{
					lastSpace = true;

					if (endState != DATA_NONE)
						endState = DATA_NONE;
					else
					{
						switch (menuOption)
						{
						case 0: //single
							m = new map(NULL, true);
							break;
						case 1: //server
							{
								//you're a server
								std::cout << "Waiting for connection on " << sf::IpAddress::getPublicAddress() << " ("
																		<< sf::IpAddress::getLocalAddress() << ")..." << std::endl;
								//*lastIP = sf::IpAddress::getPublicAddress().toString();
								sf::TcpListener listener;
								listener.listen(NETWORK_PORT_TCP);

								//start the render thread already so it can show the waiting for connection screen
								//*global_renderThreadDone = false;
								//global_window->setActive(false);
								//tempRenderThread.launch();
								//renderThread = true;
								std::stringstream ss;
								ss << "WAITING FOR CONNECTION ON " << sf::IpAddress::getPublicAddress();
								renderText(global_drawer, ss.str(),
											INTERFACE_ACTIVECOLOR, SCREENWIDTH / 2, SCREENHEIGHT / 2, true);
								global_drawer->render(global_window);

								listener.accept(socket);
								std::cout << socket.getRemoteAddress() << " connected." << std::endl;

								m = new map(&socket, true);
							}
							break;
						case 2: //client
							{
								//you're a client
								std::cout << "Attempting to connect TCP..." << std::endl;
								if (socket.connect(IP, NETWORK_PORT_TCP) != sf::Socket::Done)
									std::cout << "Connection error." << std::endl;
								else
									m = new map(&socket, false);
							}
							break;
						case 3: //music volume
							if (*global_musicVol == 100)
								*global_musicVol = 0;
							else
								*global_musicVol += 20;
							songVolAdjust();
							break;
						case 4: //sound volume
							if (*global_soundVol == 100)
								*global_soundVol = 0;
							else
								*global_soundVol += 20;
							break;
						case 5: //exit
							global_window->close();
							return;
						}
			
						global_sound->play(5);

						/*if (m != NULL && !renderThread)
						{
							*global_renderThreadDone = false;
							global_window->setActive(false);
							tempRenderThread.launch();
							renderThread = true;
						}*/
					}
				}
			}
			else
				lastSpace = false;
		}

		sf::Time currentTime = clock.getElapsedTime();
		float elapsed = currentTime.asSeconds() - lastTime.asSeconds();
		lastTime = currentTime;
		if (endState != DATA_NONE)
		{
			//draw the end state
			renderText(global_drawer, global_data->getLine(endState), INTERFACE_ACTIVECOLOR, SCREENWIDTH / 2, SCREENHEIGHT / 2, true, SCREENWIDTH * 3 / 4);
			global_drawer->render(global_window);
		}
		else if (m == NULL)
		{
			//draw the menu
			for (unsigned int i = 0; i < 6; i++)
			{
				sf::Color c;
				if (menuOption == i)
					c = INTERFACE_ACTIVECOLOR;
				else
					c = INTERFACE_INACTIVECOLOR;
				std::string name;
				switch(i)
				{
				case 0:
					name = "SINGLEPLAYER";
					break;
				case 1:
					name = "HOST A SERVER";
					break;
				case 2:
					name = "JOIN SERVER WITH IP: " + IP;
					break;
				case 3:
					name = "MUSIC VOLUME " + std::to_string((long long) (*global_musicVol)) + "%";
					break;
				case 4:
					name = "SOUND VOLUME " + std::to_string((long long) (*global_soundVol)) + "%";
					break;
				case 5:
					name = "EXIT";
					break;
				}
				renderText(global_drawer, name, c, SCREENWIDTH / 2, SCREENHEIGHT * ((i + 0.5f) / 6), true);
			}
			global_drawer->render(global_window);
		}
		else
		{
			if (m->endState() == DATA_NONE)
			{
				//it's still ongoing
				m->update(elapsed);

				if (!madeThread && m->ready() && m->isNetworked())
				{
					madeThread = true;
					//threads
					networkThread.launch();
				}
				/*if (*global_renderThreadDone)
				{
					if (renderThread)
					{
						std::cout << "Switching to single-thread rendering." << std::endl;
						tempRenderThread.terminate();
						renderThread = false;
						global_window->setActive(true);
					}*/
					m->render(global_drawer);
					global_drawer->render(global_window);
				//}
			}
			else// if (!renderThread || *global_renderThreadDone)
			{
				std::cout << "Game ended." << std::endl;

				//it's ended for one reason or another
				endState = m->endState();
				if (madeThread)
					networkThread.terminate();
				/*if (renderThread)
				{
					std::cout << "Terminating render thread." << std::endl;
					renderThread = false;
					global_window->setActive(true);
					tempRenderThread.terminate();
				}*/
				socket.disconnect();
				delete m;
				madeThread = false;
				m = NULL;
				global_map = NULL;
				menuOption = 0;
				playSong(MENUMUSIC);
			}
		}
    }

	if (m != NULL)
		delete m;
}