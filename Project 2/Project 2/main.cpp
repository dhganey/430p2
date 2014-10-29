#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <map>

#include "main.h"

const std::string whitespace = " \t\f\v\n\r";
int currentFunction; //number used to differentiate newly created functions
int uniqueVarNum; //number used to ensure created loop vars don't contradict
int uniqueStructNum; //number used to ensure created structs don't contradict
std::vector<std::string> input;
std::map < std::string, std::string> varsAndTypes;

int main()
{
	currentFunction = 1;
	uniqueVarNum = 1;
	uniqueStructNum = 1;

	readInput();
	//printVector(input);

	//find variable declarations
	processVariables();

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
	//create a vector for the new pthreads void* function
	std::vector<std::string> newFunction;

	std::string newFuncName;
	newFuncName.append("void* func").append(std::to_string(currentFunction)).append("(void* param)");
	newFunction.push_back(newFuncName);
	currentFunction++;

	//grab the necessary values from the #pragma line
	std::string pragmaString = input.at(start);
	std::vector<std::string> privVars = getConstructVars(pragmaString, "private");
	std::vector<std::string> sharedVars = getConstructVars(pragmaString, "shared");
	int numThreads = getNumThreads(pragmaString);

	//TODO shared vars

	//process private variables
	std::vector<std::string> newStruct;
	if (privVars.size() > 0)
	{
		//for each privateVar, we need to save it to a struct. first, create the struct
		std::string structStr = std::string("struct Struct").append(std::to_string(uniqueStructNum++));
		newStruct.push_back(structStr);
		newStruct.push_back("{");
		//now for each private variable, create an attribute in the struct
		for (int i = 0; i < privVars.size(); i++)
		{
			//first, determine the type of the variable using our varmap
			std::string varType = varsAndTypes[privVars.at(i)];
			//now, add it to the newStruct
			structStr = varType.append(" ").append(privVars.at(i)).append(";");
			newStruct.push_back(structStr);
		}
		newStruct.push_back("}");
	}

	//copy the code from the parallel to the new function
	for (int i = start + 1; i <= end; i++) //move start up to hit bracket, <= end to include last bracket
	{
		newFunction.push_back(input.at(i));
	}

	//delete the #pragma section
	input.erase(input.begin() + start, input.begin() + end + 1);

	//set up the pthreads code
	//record the offset as start since we're going to be changing the size of the vector
	//first we need an array of pthread_t threadids with size = numthreads
	int newOffset = start;
	std::string tempString = std::string("pthread_t threads[").append(std::to_string(numThreads)).append("];");
	input.insert(input.begin() + newOffset++, tempString);

	//now set up a for loop to dispatch a pthread for each thread
	std::string loopVar = std::string("uniqueVar").append(std::to_string(uniqueVarNum));
	uniqueVarNum++;
	tempString = std::string("for (int ").append(loopVar).append(" = 0; ").append(loopVar).append(" < ").append(std::to_string(numThreads)).append("; ").append(loopVar).append("++)");
	input.insert(input.begin() + newOffset++, tempString);
	tempString = "{";
	input.insert(input.begin() + newOffset++, tempString);
	//TODO create the tempString based on the params
	//tempString = std::string("pthread_create(&threads[").append(loopVar).append("], NULL, func").append(std::to_string(currentFunction)).append(", (void*)")

	//insert the new stuff, in reverse order!!
	insertAfterIncludes(newFunction); //first, new function

	if (newStruct.size() > 0) //above that, new struct (if necessary)
	{
		insertAfterIncludes(newStruct);
	}

	printVector(input);
}

void insertAfterIncludes(std::vector<std::string>& vecRef)
{
	//move past any includes
	int i;
	for (i = 0; i < input.size(); i++)
	{
		if (input.at(i).length() > 0 && input.at(i).at(0) != '#')
		{
			break;
		}
	}

	//once we're here, we're past the includes and we can copy the new function
	//for some reason, we need to insert this in reverse order
	for (int j = vecRef.size()-1; j >= 0; j--)
	{
		input.insert(input.begin() + i, vecRef.at(j));
	}
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

void processVariables()
{
	const std::string intStr = "int";
	const std::string doubleStr = "double";

	for (int i = 0; i < input.size(); i++)
	{
		std::string curStr = input.at(i);
		if (curStr.substr(0, 3).compare("int") == 0) //integer declaration
		{
			if (curStr.substr(0, curStr.length()).compare("int main()") == 0)
			{
				//do nothing for main
			}
			else
			{
				std::string varName = curStr.substr(4, (curStr.length() - 5)); //5 to account for "int " plus the semicolon
				varsAndTypes[varName] = intStr;
			}
		}
		else if (curStr.substr(0, 6).compare("double") == 0) //double declaration
		{
			std::string varName = curStr.substr(7, (curStr.length() - 8));
			varsAndTypes[varName] = doubleStr;
		}
	}
}