#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iterator>
#include <map>

#include "main.h"

const std::string whitespace = " \t\f\v\n\r";
int currentFunction; //number used to differentiate newly created functions
int uniqueVarNum; //number used to ensure created loop vars don't contradict
int uniqueStructNum; //number used to ensure created structs don't contradict
strvec input;
std::map < std::string, std::string> varsAndTypes;
std::map<std::string, int> varsAndLines;

int main()
{
	currentFunction = 1;
	uniqueVarNum = 1;
	uniqueStructNum = 1;

	readInput();

	//find variable declarations
	processVariables();

	//First, handle parallels
	bool foundConstruct = true;
	while (foundConstruct)
	{
		foundConstruct = processParallel();
	}

	//next, handle parallel for
	foundConstruct = true;
	while (foundConstruct)
	{
		foundConstruct = processParallelFor();
	}

	//last, change all omp_get_thread_num calls to pthread_self()
	processGetThreadNum();

	insertAfterIncludes(createStartEndStruct());

	strvec includes;
	includes.push_back("#include <pthread.h>");
	includes.push_back("#include <algorithm>");
	includes.push_back("#include <cstring>");
	insertAfterIncludes(includes);

	printVector(input);
}

//Returns true if it processes something
//Returns false if it never finds a parallel
bool processParallel()
{
	bool foundPragma = false;

	for (int i = 0; i < input.size(); i++)
	{
		std::string curStr = input.at(i);
		if ((curStr.substr(0, 20).compare("#pragma omp parallel") == 0) &&
			 curStr.substr(0, 24).compare("#pragma omp parallel for") != 0)
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
	//grab the necessary values from the #pragma line
	std::string pragmaString = input.at(start);
	strvec privVars = getConstructVars(pragmaString, "private");
	strvec sharedVars = getConstructVars(pragmaString, "shared");
	int numThreads = getNumThreads(pragmaString);

	//create a vector for the new pthreads void* function
	std::vector<std::string> newFunction;

	std::string newFuncName;
	std::string smallNewFuncName = std::string("func").append(std::to_string(currentFunction));
	newFuncName.append("void* func").append(std::to_string(currentFunction)).append("(void* param)");
	newFunction.push_back(newFuncName);
	currentFunction++;
	newFunction.push_back("{");

	//redeclare all private vars in the new function
	for (int i = 0; i < privVars.size(); i++)
	{
		std::string typeStr = varsAndTypes[privVars.at(i)];
		std::string newLine = typeStr.append(" ").append(privVars.at(i)).append(";");
		newFunction.push_back(newLine);
	}

	//redeclare all global vars at the top
	strvec globalVars;
	for (int i = 0; i < sharedVars.size(); i++)
	{
		std::string typeStr = varsAndTypes[sharedVars.at(i)];
		globalVars.push_back(typeStr.append(" ").append(sharedVars.at(i)).append(";"));
	}

	//remove global var declarations from main
	for (int i = 0; i < sharedVars.size(); i++)
	{
		int declLine = varsAndLines[sharedVars.at(i)];
		std::string curStr = input.at(declLine);
		std::string fullLine = varsAndTypes[sharedVars.at(i)];
		fullLine = fullLine.append(" ").append(sharedVars.at(i)).append(";");

		if (curStr.compare(fullLine) == 0) //easy case: one declaration per line
		{
			input.at(declLine) = ""; //remove the declaration. could also comment it
		}
		else
		{
			//TODO: must be multiple vars declared here
		}
	}

	//copy the function code as-is
	for (int i = start + 2; i < end; i++) //move start up to pass first bracket, < end to not include last bracket.
	{
		newFunction.push_back(input.at(i));
	}
	newFunction.push_back("}");

	//delete the #pragma section
	input.erase(input.begin() + start, input.begin() + end + 1);

	//record the offset as start since we're going to be changing the size of the vector
	int newOffset = start;

	//set up the pthreads code
	//first we need an array of pthread_t threadids with size = numthreads
	std::string tempString = std::string("pthread_t threads[").append(std::to_string(numThreads)).append("];"); //TODO do we need to populate this?
	input.insert(input.begin() + newOffset++, tempString);
	//populate the threads[] array
	std::string loopVar = std::string("uniqueVar").append(std::to_string(uniqueVarNum));
	uniqueVarNum++;
	tempString = std::string("for (int ").append(loopVar).append(" = 0; ").append(loopVar).append(" < ").append(std::to_string(numThreads)).append("; ").append(loopVar).append("++)");
	input.insert(input.begin() + newOffset++, tempString);
	input.insert(input.begin() + newOffset++, "{");
	tempString = std::string("threads[").append(loopVar).append("] = ").append(loopVar).append(";");
	input.insert(input.begin() + newOffset++, tempString);
	input.insert(input.begin() + newOffset++, "}");

	//now set up a for loop to dispatch a pthread for each thread
	loopVar = std::string("uniqueVar").append(std::to_string(uniqueVarNum));
	uniqueVarNum++;
	tempString = std::string("for (int ").append(loopVar).append(" = 0; ").append(loopVar).append(" < ").append(std::to_string(numThreads)).append("; ").append(loopVar).append("++)");
	input.insert(input.begin() + newOffset++, tempString);
	input.insert(input.begin() + newOffset++, "{");
	tempString = std::string("pthread_create(&threads[").append(loopVar).append("], NULL, ").append(smallNewFuncName).append(", NULL);");
	input.insert(input.begin() + newOffset++, tempString);
	input.insert(input.begin() + newOffset++, "}");

	//now set up a for loop to join the pthreads
	loopVar = std::string("uniqueVar").append(std::to_string(uniqueVarNum));
	uniqueVarNum++;
	tempString = std::string("for (int ").append(loopVar).append(" = 0; ").append(loopVar).append(" < ").append(std::to_string(numThreads)).append("; ").append(loopVar).append("++)");
	input.insert(input.begin() + newOffset++, tempString);
	input.insert(input.begin() + newOffset++, "{");
	tempString = std::string("pthread_join(threads[").append(loopVar).append("], NULL);");
	input.insert(input.begin() + newOffset++, tempString);
	input.insert(input.begin() + newOffset++, "}");

	//insert the new stuff, in reverse order!!
	insertAfterIncludes(newFunction); //first, new function
	insertAfterIncludes(globalVars);
}

//Returns true if it processes something
//Returns false if it never finds a parallel for
bool processParallelFor()
{
	bool foundPragma = false;

	for (int i = 0; i < input.size(); i++)
	{
		std::string curStr = input.at(i);
		if (curStr.substr(0, 24).compare("#pragma omp parallel for") == 0)
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
			parallelForHelper(i, j);
		}
		//otherwise, move on--handle it elsewhere
	}

	return foundPragma;

}

void parallelForHelper(int start, int end)
{
	//grab the necessary values from the #pragma line
	std::string pragmaString = input.at(start);
	strvec privVars = getConstructVars(pragmaString, "private");
	strvec sharedVars = getConstructVars(pragmaString, "shared");
	int numThreads = getNumThreads(pragmaString);

	//create a vector for the new pthreads void* function
	std::vector<std::string> newFunction;

	std::string newFuncName;
	std::string smallNewFuncName = std::string("func").append(std::to_string(currentFunction));
	newFuncName.append("void* func").append(std::to_string(currentFunction)).append("(void* param)");
	newFunction.push_back(newFuncName);
	currentFunction++;
	newFunction.push_back("{");

	//redeclare all private vars in the new function
	for (int i = 0; i < privVars.size(); i++)
	{
		std::string typeStr = varsAndTypes[privVars.at(i)];
		std::string newLine = typeStr.append(" ").append(privVars.at(i)).append(";");
		newFunction.push_back(newLine);
	}

	//redeclare all global vars at the top
	strvec globalVars;
	for (int i = 0; i < sharedVars.size(); i++)
	{
		std::string typeStr = varsAndTypes[sharedVars.at(i)];
		globalVars.push_back(typeStr.append(" ").append(sharedVars.at(i)).append(";"));
	}

	//remove global var declarations from main
	for (int i = 0; i < sharedVars.size(); i++)
	{
		int declLine = varsAndLines[sharedVars.at(i)];
		std::string curStr = input.at(declLine);
		std::string fullLine = varsAndTypes[sharedVars.at(i)];
		fullLine = fullLine.append(" ").append(sharedVars.at(i)).append(";");

		if (curStr.compare(fullLine) == 0) //easy case: one declaration per line
		{
			input.at(declLine) = ""; //remove the declaration. could also comment it
		}
		else
		{
			//TODO: must be multiple vars declared here
		}
	}

	//move the code to the new function
	//first modify the for loop to use the start and end from the new struct
	int numIterations = getNumIterations(input.at(start + 1));
	input.at(start + 1) = fixForLine(input.at(start + 1));

	//then copy the rest of the stuff
	for (int i = start + 1; i < end; i++)
	{
		newFunction.push_back(input.at(i));
	}
	newFunction.push_back("}");

	//delete the #pragma section
	input.erase(input.begin() + start, input.begin() + end + 1);

	//record the offset as start since we're going to be changing the size of the vector
	int newOffset = start;

	//set up the pthreads code
	//first we need an array of pthread_t threadids with size = numthreads
	std::string tempString = std::string("pthread_t threads[").append(std::to_string(numThreads)).append("];"); //TODO do we need to populate this? populated above, did not seem to work
	input.insert(input.begin() + newOffset++, tempString);

	//create the pthreads code
	int uneven = numIterations - ((numIterations / numThreads) * numThreads); //figure out how many don't go in evenly
	int basicNum = numIterations / numThreads;
	//now distribute the iterations among the pthreads
	//example: 18 iterations among 4 threads
	//break it down as follows:
	//0-3 thread 1
	//4-7 thread 2
	//8-11 thread 3
	//12-17 thread 4
	//or 12-14 thread 4

	int startIteration = 0;
	int endIteration = basicNum - 1;
	for (int i = 0; i < numThreads; i++)
	{
		tempString = std::string("StartEnd paramStruct").append(std::to_string(i)).append(";");
		input.insert(input.begin() + newOffset++, tempString);

		tempString = std::string("paramStruct").append(std::to_string(i)).append("->start = ").append(std::to_string(startIteration));
		input.insert(input.begin() + newOffset++, tempString);
		startIteration += basicNum;

		if (i == numThreads - 1) //if we're on the last loop, give all remaining iterations to this thread.
		{
			endIteration = numIterations;
		}
		tempString = std::string("paramStruct").append(std::to_string(i)).append("->end = ").append(std::to_string(endIteration));
		input.insert(input.begin() + newOffset++, tempString);
		endIteration += basicNum;

		tempString = std::string("pthread_create(threads[").append(std::to_string(i)).append("], NULL, ").append(smallNewFuncName).append(", (void*) paramStruct").append(std::to_string(i)).append(");");
		input.insert(input.begin() + newOffset++, tempString);
	}

	//insert the new stuff, in reverse order!!
	insertAfterIncludes(newFunction); //first, new function
	insertAfterIncludes(globalVars);

}

void insertAfterIncludes(strvec& vecRef)
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

void printVector(strvec& vecRef)
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

int getStartNum(std::string str)
{
	for (int i = 0; i < str.length(); i++)
	{
		if (str.at(i) == ';')
		{
			for (int j = i; j > 0; j--) //go backwards until we find space or =
			{
				if (str.at(j) == ' ' || str.at(j) == '=')
				{
					return atoi(str.substr(j + 1, (i - j - 1)).c_str());
				}
			}
		}
	}
}

strvec getConstructVars(std::string str, std::string criteria)
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
		std::string typeStr;
		int offset;

		//determine what is being declared
		if (curStr.substr(0, 3).compare("int") == 0) //integer declaration
		{
			if (curStr.substr(0, 8).compare("int main") == 0) //special case for this int!
			{
				continue;
			}
			typeStr = intStr;
			offset = 4;
		}
		else if (curStr.substr(0, 6).compare("double") == 0) //double declaration
		{
			typeStr = doubleStr;
			offset = 7;
		}
		else
		{
			continue; //not a var declaration
		}


		if (curStr.find(",") == std::string::npos) //if no commas
		{
			std::string varName = curStr.substr(offset, (curStr.length() - offset - 1)); //-1 to leave semicolon off
			varsAndTypes[varName] = typeStr;
			varsAndLines[varName] = i;
		}
		else //multiple vars/line
		{
			std::istringstream buf(curStr);
			std::istream_iterator<std::string> beg(buf), end;
			std::vector<std::string> tokens(beg, end);

			for (int j = 1; j < tokens.size(); j++)
			{
				std::string varName = tokens.at(j).substr(0, tokens.at(j).length() - 1); //leave off the comma or semicolon
				varsAndTypes[varName] = typeStr;
				varsAndLines[varName] = i;
			}
		}
	}
}

void processGetThreadNum()
{
	std::vector<std::string> newThreadIdFunc;
	newThreadIdFunc.push_back("int gettid()");
	newThreadIdFunc.push_back("{");
	newThreadIdFunc.push_back("pthread_t ptid = pthread_self();");
	newThreadIdFunc.push_back("int threadId = 0;");
	newThreadIdFunc.push_back("std::memcpy(&threadId, &ptid, std::min(sizeof(threadId), sizeof(ptid)));");
	newThreadIdFunc.push_back("return threadId;");
	newThreadIdFunc.push_back("}");


	std::string replacement = "id = gettid();";

	for (int i = 0; i < input.size(); i++)
	{
		std::string curStr = input.at(i);
		std::size_t pos = curStr.find("omp_get");
		if (pos != std::string::npos) //if we find one
		{
			//replace with a call to gettid()
			input.at(i) = replacement;
		}
	}

	insertAfterIncludes(newThreadIdFunc);
}

strvec createStartEndStruct()
{
	strvec newStruct;
	newStruct.push_back("struct StartEnd");
	newStruct.push_back("{");
	newStruct.push_back("int start;");
	newStruct.push_back("int end;");
	newStruct.push_back("};");

	return newStruct;
}

std::string fixForLine(std::string forline)
{
	//determine the loop variable name
	//by assumptions it should come right after the ( before a space or =
	for (int i = 0; i < forline.length(); i++)
	{
		if (forline.at(i) == '(')
		{
			int j = i + 1;
			//move past any initial spaces
			while (forline.at(j) == ' ')
			{
				j++;
			}

			//now move through var until we hit a space or =
			while (forline.at(j) != ' ' && forline.at(j) != '=')
			{
				j++;
			}
			//now we can obtain the name of the loop var
			std::string loopVar = forline.substr(i + 1, (j - i));

			std::string newstr = std::string("for ( ").append(loopVar).append(" = ((StartEnd*)param)->start; ").append(loopVar).append(" < ((StartEnd*)param)->end; ").append(loopVar).append("++)");
			return newstr;
		}
	}

	return "";
}