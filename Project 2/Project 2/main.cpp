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
	//printVector(input);

	std::vector<std::string> output;
	bool foundPragma;
	do
	{
		foundPragma = processPragmas(input, output);
	} while (foundPragma);

	printVector(output);
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

bool processPragmas(std::vector<std::string>& input, std::vector<std::string>& output)
{
	bool foundPragma = false;

	for (int i = 0; i < input.size(); i++)
	{
		std::string curStr = input.at(i);
		if (curStr.length() == 0) //blank line
		{
			output.insert(output.begin() + i, curStr);
			continue;
		}

		if (curStr.at(0) == '#') //preprocessor directive
		{
			if (curStr.substr(0, 4).compare("#inc") == 0) //include
			{
				output.insert(output.begin() + i, curStr); //put includes in the output
			}
			else //must be a pragma omp!
			{
				foundPragma = true;

				if (curStr.substr(0, 24).compare("#pragma omp parallel for") == 0)
				{
					std::cout << "parallel for" << std::endl;
					processParallelFor(input, output, i);
				}
				else if (curStr.substr(0, 20).compare("#pragma omp parallel") == 0)
				{
					//TODO
					std::cout << "regular parallel" << std::endl;
				}
				else if (curStr.substr(0, 20).compare("#pragma omp critical") == 0)
				{
					//TODO
					std::cout << "critical" << std::endl;
				}
				else if (curStr.substr(0, 18).compare("#pragma omp single") == 0)
				{
					//TODO
					std::cout << "single " << std::endl;
				}

				//TODO:
				output.insert(output.begin() + i, curStr);
			}
		}
		else //put everything else in the output
		{
			output.insert(output.begin() + i, curStr);
		}
	}

	return foundPragma;
}

void processParallelFor(std::vector<std::string>& input, std::vector<std::string>& output, int start)
{
	std::string curStr = input.at(start);
	int threads = getNumThreads(curStr);
	int iterations = getNumIterations(input.at(start + 1));

	int diff = iterations % threads;
	//TODO FINISH THIS
}

int getNumThreads(std::string str)
{
	//#pragma omp parallel for num_threads(xxxx)
	int i = 0;
	int j = 0;
	for (i; i < str.length(); i++)
	{
		if (str.at(i) == '(')
		{
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
}