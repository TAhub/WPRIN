#include "stdafx.h"
using namespace std;

database::database()
{
	ifstream file;
	file.open("data.txt");
	ifstream fileL;
	fileL.open("lines.txt");

	if (file.is_open() && fileL.is_open())
	{
		//load lines
		string line;
		vector<string> lineNames;
		while (getline(fileL, line))
			if (line.at(0) != '/')
			{
				string word;
				istringstream iss(line);

				iss >> word;
				word = '#' + word;
				lineNames.push_back(word);

				string compLine = "";
				while (!iss.eof())
				{
					if (compLine.length() != 0)
						compLine += " ";
					iss >> word;
					compLine += word;
				}

				lines.push_back(compLine);
			}

		//load data
		vector<vector<vector<string>>> references;
		vector<vector<string>> names;
		vector<string> prefixes;
		while (getline(file, line))
		{
			if (line.at(0) == '/') //it's a comment
			{
				//cout << line << endl;
			}
			else if (line.at(0) == '*') //it's a category
			{
				//add new data entries
				vector<vector<int>> dataC;
				vector<string> nameC;
				vector<vector<string>> refC;
				names.push_back(nameC);
				data.push_back(dataC);
				references.push_back(refC);

				//get the prefix
				istringstream iss(line);
				string word;
				iss >> word;
				iss >> word;
				prefixes.push_back(word);
			}
			else //it's an entry
			{
				//make entries
				vector<int> dataC;
				vector<string> refC;

				//break it up
				istringstream iss(line);
				string word;
				iss >> word;
				names[names.size() - 1].push_back(word); //the name of this entry
				while (!iss.eof())
				{
					iss >> word;
					
					if (word.compare("none") == 0)
					{
						//it's an empty reference
						refC.push_back("");
						dataC.push_back(DATA_NONE);
					}
					else if (word.compare("true") == 0)
					{
						//it's true!
						refC.push_back("");
						dataC.push_back(1);
					}
					else if (word.compare("false") == 0)
					{
						//it's false!
						refC.push_back("");
						dataC.push_back(0);
					}
					else if (word.at(0) == '#')
					{
						//it's a string
						refC.push_back("");
						unsigned int lineN = DATA_INVALID;
						for (unsigned int i = 0; i < lineNames.size(); i++)
							if (lineNames[i].compare(word) == 0)
							{
								lineN = i;
								break;
							}
						if (lineN == DATA_INVALID)
						{
							cout << "Invalid line " << word << "." << endl;
							dataC.push_back(0);
						}
						else
							dataC.push_back(lineN);
					}
					else
					{
						char *p;
						int asNumber = strtol(word.c_str(), &p, 10);
						if (*p)
						{
							//it's a reference
							refC.push_back(word);
							dataC.push_back(0);
						}
						else
						{
							//it's a number
							refC.push_back("");
							dataC.push_back(asNumber);
						}
					}
				}

				//push the entries
				data[data.size() - 1].push_back(dataC);
				references[references.size() - 1].push_back(refC);
			}
		}

		//resolve references
		for (unsigned int i = 0; i < data.size(); i++)
			for (unsigned int j = 0; j < data[i].size(); j++)
				for (unsigned int k = 0; k < data[i][j].size(); k++)
					if (references[i][j][k].length() > 0)
					{
						istringstream iss(references[i][j][k]);
						string prefix;
						string name;
						getline(iss, prefix, '_');
						getline(iss, name, '_');
						
						//get the category
						unsigned int cat = DATA_INVALID;
						for (unsigned int l = 0; l < prefixes.size(); l++)
							if (prefixes[l].compare(prefix) == 0)
							{
								cat = l;
								break;
							}

						if (cat == DATA_INVALID)
							cout << "Prefix " << prefix << " not found." << endl;
						else
						{
							//get the item
							unsigned int item = DATA_INVALID;
							for (unsigned int l = 0; l < names[cat].size(); l++)
								if (names[cat][l].compare(name) == 0)
								{
									item = l;
									break;
								}

							if (item == DATA_INVALID)
								cout << "Item " << name << " not found with prefix " << prefix << "." << endl;
							else
								data[i][j][k] = item;
						}
					}

		valid = true;
	}
	else
		valid = false;

	file.close();
	fileL.close();
}

database::~database()
{
}

unsigned int database::getEntrySize(unsigned int category, unsigned int entry)
{
	return data[category][entry].size();
}

unsigned int database::getCategorySize(unsigned int category)
{
	return data[category].size();
}

int database::getValue(unsigned int category, unsigned int entry, unsigned int value)
{
	return data[category][entry][value];
}

string database::getLine(unsigned int lineNum)
{
	return lines[lineNum];
}