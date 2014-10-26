#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include "main.h"

const std::string whitespace = " \t\f\v\n\r";
int currentFunction; //number used to differentiate newly created functions
std::vector<std::string> input;
std::vector<std::string> output;

int main()
{
	currentFunction = 1;

	std::vector<std::string> input;
	readInput();
	printVector(input);

	//First, handle parallels
	bool foundParallel = true;
	while (foundParallel)
	{
		foundParallel = processParallel();
	}
}

//Returns true if it processes something
//Returns false if it never finds a parallel
bool processParallel()
{
	bool foundPragma = false;

	for (int i = 0; i < input.size(); i++)
	{
		std::string curStr = input.at(i);
		if (curStr.substr(0, 20).compare("#pragma omp parallel") == 0)
		{
			foundPragma = true;
			int j = i + 2; //move past the opening bracket
			int brackets = 1; //OK to assume we have an opening bracket
			//find the end of the parallel region
			while (j < input.size())
			{
				j++;

				std::string tempStr = input.at(j);
				if (tempStr.length() <= 0)
				{
					continue;
				}

				if (tempStr.at(0) == '{')
				{
					brackets++;
				}
				if (tempStr.at(0) == '}')
				{
					brackets--;
				}
				if (brackets == 0)
				{
					break;
				}
			}
			//at this point, j points to the ending bracket, and i points to the pragma
			parallelHelper(i, j);
		}
		//otherwise, move on--handle it elsewhere
	}

	return foundPragma;
}

void parallelHelper(int start, int end)
{
	std::vector<std::string> newFunction;
	std::string newFuncName;
	std::string temp = "void* func";
	newFuncName.append(temp).append(std::to_string(currentFunction)).append("(void* param)");
	newFunction.push_back(newFuncName);

	std::string pragmaString = input.at(start);
	std::vector<std::string> privVars = getConstructVars(pragmaString, "private");
	std::vector<std::string> sharedVars = getConstructVars(pragmaString, "shared");

	//TODO: do something with these
}

void readInput()
{
	std::string temp;
	std::stringstream sstream;
	while (std::getline(std::cin, temp))
	{
		trimLeft(temp);
		trimRight(temp);
		input.push_back(temp);
	}
}

void trimLeft(std::string& str)
{
	int start = str.find_first_not_of(whitespace);
	str.erase(0, start);
}

void trimRight(std::string& str)
{
	int end = str.find_last_not_of(whitespace);
	if (end < 1)
	{
		return;
	}
	str.erase(str.begin() + end, str.end()-1);
}

void printVector(std::vector<std::string>& vecRef)
{
	for (int i = 0; i < vecRef.size(); i++)
	{
		std::cout << vecRef.at(i) << std::endl;
	}
}

int getNumThreads(std::string str)
{
	const int NUM_THREADS_OFFSET = 11;
	int i = 0;
	int j = 0;
	for (i; i < str.length(); i++)
	{
		if (str.at(i) == 'n') //possibly num_threads
		{
			if (str.substr(i, NUM_THREADS_OFFSET).compare("num_threads") == 0)
			{
				i += NUM_THREADS_OFFSET;
				j = i;
				for (j; j < str.length(); j++)
				{
					if (str.at(j) == ')')
					{
						//by this point, i and j point to the parentheses around the thread num
						//convert it to an int
						return atoi(str.substr(i + 1, (j - i - 1)).c_str());
					}
				}
			}
		}
	}

	return -1; //caller should check for this error case
}

int getNumIterations(std::string str)
{
	//for( i = 0; i < 16; i++ )

	bool firstClause = false;
	for (int i = 0; i < str.length(); i++)
	{
		if (str.at(i) == ';')
		{
			if (!firstClause)
			{
				firstClause = true;
			}
			else //we're on the second clause. go backwards until we find a space or operator
			{
				for (int j = i; j > 0; j--)
				{
					if (str.at(j) == ' ' || str.at(j) == '<' || str.at(j) == '>')
					{
						return atoi(str.substr(j + 1, (i - j - 1)).c_str());
					}
				}
			}
		}
	}

	return -1; //caller should check for this error case
}

std::vector<std::string> getConstructVars(std::string str, std::string criteria)
{
	const int OFFSET = criteria.length();
	std::vector<std::string> privVars;

	std::size_t priv = str.find(criteria);
	if (priv == std::string::npos) //if no private construct
	{
		return privVars;
	}

	//otherwise, find what's in it
	int i = (int)priv + OFFSET;
	int j = i;
	while (str.at(j) != ')')
	{
		j++;
	}
	//now i and j both point to parentheses

	//move i and x into the construct
	for (int x = ++i; x <= j; x++)
	{
		if (str.at(x) == ',' || x == j) //if we hit a comma or we're at the end
		{
			privVars.push_back(str.substr(i, (x - i)));
			i = x + 1;
		}
	}

	return privVars;
}