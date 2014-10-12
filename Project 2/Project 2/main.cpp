#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include "main.h"

const std::string whitespace = " \t\f\v\n\r";

int main()
{
	std::vector<std::string> input;
	readInput(input);
	printVector(input);
}

void readInput(std::vector<std::string>& vecRef)
{
	std::string temp;
	std::stringstream sstream;
	while (std::getline(std::cin, temp))
	{
		trimLeft(temp);
		trimRight(temp);
		vecRef.push_back(temp);
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