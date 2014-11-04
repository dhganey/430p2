#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iterator>
#include <map>
#include <set>

#include "main.h"

const std::string whitespace = " \t\f\v\n\r";

int currentFunction; //number used to differentiate newly created functions
int uniqueVarNum; //number used to ensure created loop vars don't contradict
int uniqueStructNum; //number used to ensure created structs don't contradict
int numThreads; //ok for this to be global, assumptions state 1 of these only

strvec input; //primary vector holding input
strvec privVarBackup; //holds all private vars, which are redeclared in main at the end
strvec sharedVarBackup; //holds all shared vars
strvec globalVars; //holds shared vars which should be declared global
strvec totalPrivBackup; //backs up all private vars

std::map < std::string, std::string> varsAndTypes; //maps a variable name to the string of its type
std::map<std::string, int> varsAndLines; //maps a variable name to the line on which it was declared


int main()
{
	//set the global ints
	currentFunction = 1;
	uniqueVarNum = 1;
	uniqueStructNum = 1;
	numThreads = 0;

	readInput();

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

	//next, handle criticals
	foundConstruct = true;
	while (foundConstruct)
	{
		foundConstruct = processCritical();
	}

	//next, handle single
	foundConstruct = true;
	while (foundConstruct)
	{
		foundConstruct = processSingle();
	}

	//last, change all omp_get_thread_num calls to use the paramstruct
	processGetThreadNum();

	//put global vars at the top
	eliminateDuplicates(globalVars);
	insertAfterIncludes(globalVars);

	//put private vars back in main
	eliminateDuplicates(totalPrivBackup);
	declarePrivatesInMain(totalPrivBackup);

	//create and insert the parameter struct
	strvec newStruct = createStartEndStruct();
	insertAfterIncludes(newStruct);

	//create and add the mutex call
	strvec synchronization;
	synchronization.push_back("pthread_mutex_t theMutex = PTHREAD_MUTEX_INITIALIZER;");
	insertAfterIncludes(synchronization);

	//create and add new includes
	strvec includes;
	includes.push_back("#include <pthread.h>");
	includes.push_back("#include <algorithm>");
	includes.push_back("#include <cstring>");
	insertAfterIncludes(includes);

	//print output (to use for file redirection)
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

//Worker function. Start and end represent the #pragma line and the closing brace
//Drives all the work for a parallel
void parallelHelper(int start, int end)
{
	//grab the necessary values from the #pragma line
	std::string pragmaString = input.at(start);
	strvec privVars = getConstructVars(pragmaString, "private");
	strvec sharedVars = getConstructVars(pragmaString, "shared");
	int numThreads = getNumThreads(pragmaString);

	//save the values in a backup for redeclaration in an inner function
	privVarBackup.clear();
	for (int i = 0; i < privVars.size(); i++)
	{
		privVarBackup.push_back(privVars.at(i));
	}
	sharedVarBackup.clear();
	for (int i = 0; i < sharedVars.size(); i++)
	{
		sharedVarBackup.push_back(sharedVars.at(i));
	}

	//create a vector for the new pthreads void* function
	std::vector<std::string> newFunction;

	//add to the new function
	std::string newFuncName;
	std::string smallNewFuncName = std::string("func").append(std::to_string(currentFunction));
	newFuncName.append("void* func").append(std::to_string(currentFunction)).append("(void* paramStruct)");
	newFunction.push_back(newFuncName);
	currentFunction++;
	newFunction.push_back("{");

	//redeclare variables in the new function
	redeclareVars(privVars, newFunction);
	strvec privVarDeclarations;
	redeclareVars(privVars, privVarDeclarations);
	add(privVarDeclarations, totalPrivBackup);

	//stick shared vars in the global vector for declaration later
	redeclareVars(sharedVars, globalVars);

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

	//now create the pthreads
	for (int i = 0; i < numThreads; i++)
	{
		tempString = std::string("StartEnd paramStruct").append(std::to_string(i)).append(";");
		input.insert(input.begin() + newOffset++, tempString);

		tempString = std::string("paramStruct").append(std::to_string(i)).append(".threadNum = ").append(std::to_string(i)).append(";");
		input.insert(input.begin() + newOffset++, tempString);

		tempString = std::string("pthread_create(&threads[").append(std::to_string(i)).append("], NULL, ").append(smallNewFuncName).append(", (void*) &paramStruct").append(std::to_string(i)).append(");");
		input.insert(input.begin() + newOffset++, tempString);
	}

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
}

//Returns true if it processes something
//Returns false if it never finds a parallel for
bool processParallelFor()
{
	bool foundPragma = false;

	for (int i = 0; i < input.size(); i++)
	{
		std::string curStr = input.at(i);
		if (curStr.substr(0, 24).compare("#pragma omp parallel for") == 0 ||
			curStr.substr(0, 15).compare("#pragma omp for") == 0)
		{
			foundPragma = true;
			int j = i + 2; //move up to pragma and pass the for loop
			if (input.at(j).substr(0,1).compare("{") == 0) //if the for is contained in brackets. weird case also where trim did not remove tab after bracket
			{
				int brackets = 1;
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
			}
			else //if there are no brackets, then j is pointing to the contents of the for loop
			{
				i += 2;
				j += 2;
				input.insert(input.begin() + i, "{");
				input.insert(input.begin() + j, "}");
				i -= 2; //move i back to the pragma
			}
			//at this point, j points to the ending bracket, and i points to the pragma
			parallelForHelper(i, j);
		}
		//otherwise, move on--handle it elsewhere
	}

	return foundPragma;

}

//Worker function. Start and end represent the #pragma line and the closing brace
//Drives all the work for a parallel for
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
	newFuncName.append("void* func").append(std::to_string(currentFunction)).append("(void* paramStruct)");
	newFunction.push_back(newFuncName);
	currentFunction++;
	newFunction.push_back("{");

	//now handle the case when a parallel for is inside a parallel. in that case, the parfor will not have priv/shared declared
	//it will instead use the priv/shared declared above
	//those are backed up in global vars.
	if (privVars.size() == 0 && sharedVars.size() == 0) //if we determine nothing is private, copy the global backup to the local strvecs
	{
		for (int i = 0; i < privVarBackup.size(); i++)
		{
			privVars.push_back(privVarBackup.at(i));
		}
		for (int i = 0; i < sharedVarBackup.size(); i++)
		{
			sharedVars.push_back(sharedVarBackup.at(i));
		}
	}

	//redeclare privates in the new function
	redeclareVars(privVars, newFunction);
	strvec privVarDeclarations;
	redeclareVars(privVars, privVarDeclarations);
	add(privVarDeclarations, totalPrivBackup);

	//globals will appear later
	redeclareVars(sharedVars, globalVars);

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
	newFunction.push_back("}");

	//delete the #pragma section
	input.erase(input.begin() + start, input.begin() + end + 1);

	//record the offset as start since we're going to be changing the size of the vector
	int newOffset = start;

	//set up the pthreads code
	//first we need an array of pthread_t threadids with size = numthreads
	std::string tempString = std::string("pthread_t threads[").append(std::to_string(numThreads)).append("];");
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

		tempString = std::string("paramStruct").append(std::to_string(i)).append(".start = ").append(std::to_string(startIteration)).append(";");
		input.insert(input.begin() + newOffset++, tempString);
		startIteration += basicNum;

		if (i == numThreads - 1) //if we're on the last loop, give all remaining iterations to this thread.
		{
			endIteration = numIterations - 1; //-1 to fix off by one error
		}
		tempString = std::string("paramStruct").append(std::to_string(i)).append(".end = ").append(std::to_string(endIteration + 1)).append(";"); //plus 1 since for loop is < 
		input.insert(input.begin() + newOffset++, tempString);
		endIteration += basicNum;

		tempString = std::string("paramStruct").append(std::to_string(i)).append(".threadNum = ").append(std::to_string(i)).append(";");
		input.insert(input.begin() + newOffset++, tempString);

		tempString = std::string("pthread_create(&threads[").append(std::to_string(i)).append("], NULL, ").append(smallNewFuncName).append(", (void*) &paramStruct").append(std::to_string(i)).append(");");
		input.insert(input.begin() + newOffset++, tempString);
	}

	//now join the threads
	for (int i = 0; i < numThreads; i++)
	{
		tempString = std::string("pthread_join(threads[").append(std::to_string(i)).append("], NULL);");
		input.insert(input.begin() + newOffset++, tempString);
	}

	//insert the new stuff, in reverse order!!
	insertAfterIncludes(newFunction); //first, new function
}

//Returns true if it processes something
//Returns false if it never finds a critical
bool processCritical()
{
	bool foundPragma = false;

	for (int i = 0; i < input.size(); i++)
	{
		std::string curStr = input.at(i);
		if (curStr.substr(0, 20).compare("#pragma omp critical") == 0)
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
			criticalHelper(i, j);
		}
		//otherwise, move on--handle it elsewhere
	}

	return foundPragma;

}

//Worker function. Start and end represent the #pragma line and the closing brace
//Drives all the work for a critical
void criticalHelper(int start, int end)
{
	//simply lock at the beginning and the end
	input.at(start) = "pthread_mutex_lock( &theMutex );";
	input.insert(input.begin() + end + 1, "pthread_mutex_unlock (&theMutex );");
}

//Returns true if it processes something
//Returns false if it never finds a single
bool processSingle()
{
	bool foundPragma = false;

	for (int i = 0; i < input.size(); i++)
	{
		std::string curStr = input.at(i);
		if (curStr.substr(0, 18).compare("#pragma omp single") == 0)
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
			singleHelper(i, j);
		}
		//otherwise, move on--handle it elsewhere
	}

	return foundPragma;
}

//Worker function. Start and end represent the #pragma line and the closing brace
//Drives all the work for a single
void singleHelper(int start, int end)
{
	//for single, we just want to make sure only one thread executes it
	//so, with this solution, just make it accessible only to thread 0
	//just replace the #pragma line with an if statement
	std::string newIf = "if (((StartEnd*) paramStruct)->threadNum == 0) //arbitrarily restrict it to the only guaranteed thread, 0";
	input.at(start) = newIf;
}

//Pushes the vecRef into input after the include statements (at the top)
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
	//we need to insert this in reverse order
	for (int j = vecRef.size()-1; j >= 0; j--)
	{
		input.insert(input.begin() + i, vecRef.at(j));
	}
}

//Reads the input file line by line
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

//Removes whitespace from the beginning of the line
void trimLeft(std::string& str)
{
	int start = str.find_first_not_of(whitespace);
	str.erase(0, start);
}

//Removes whitespace from the end of the line
void trimRight(std::string& str)
{
	//strange case--#pragma omp for is failing!
	if (str.substr(0, 15).compare("#pragma omp for") == 0) //not already trimmed right, so substr
	{
		return; //no need
	}

	int end = str.find_last_not_of(whitespace);
	if (end < 1)
	{
		return;
	}
	str.erase(str.begin() + end, str.end()-1);
}

//Just outputs vecRef
void printVector(strvec& vecRef)
{
	for (int i = 0; i < vecRef.size(); i++)
	{
		std::cout << vecRef.at(i) << std::endl;
	}
}

//Grabs the number of threads from the str (#pragma line)
int getNumThreads(std::string str)
{
	if (numThreads != 0)
	{
		return numThreads;
	}
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
						numThreads = atoi(str.substr(i + 1, (j - i - 1)).c_str());
						return numThreads;
					}
				}
			}
		}
	}

	return -1; //caller should check for this error case
}

//Grabs the number of threads from the str (for loop line)
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

//Grabs the iteration start from the str (for loop line)
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

//Grabs construct vars for the criteria parameter (shared or private)
strvec getConstructVars(std::string str, std::string criteria)
{
	const int OFFSET = criteria.length();
	std::vector<std::string> privVars;

	std::size_t priv = str.find(criteria);
	if (priv == std::string::npos) //if no private/shared construct
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

//Iterates through the whole program looking for variable declarations
//Supports ONLY int and double declarations
//NOTE: This DELETES all declarations (converts the line to ""), since they are added in appropriate locations later
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
		else //TODO could include more primitives here
		{
			continue; //not a var declaration
		}

		//everything below here applies only to var declarations, since we continued above
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

		//now that the value is preserved in the hashmap, remove the declaration
		input.at(i) = "";
	}
}

//Iterates through the whole program looking for omp_get_thread_num calls
//Replaces them with a call to the paramStruct
void processGetThreadNum()
{
	std::string replacement = "id = ((StartEnd*) paramStruct)->threadNum;";

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
}

//Simply creates the standard StartEnd struct
strvec createStartEndStruct()
{
	strvec newStruct;
	newStruct.push_back("struct StartEnd");
	newStruct.push_back("{");
	newStruct.push_back("int start;");
	newStruct.push_back("int end;");
	newStruct.push_back("int threadNum;");
	newStruct.push_back("};");

	return newStruct;
}

//Converts the for loop line to use the paramStruct instead of the loop variable
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

			std::string newstr = std::string("for ( ").append(loopVar).append(" = ((StartEnd*)paramStruct)->start; ").append(loopVar).append(" < ((StartEnd*)paramStruct)->end; ").append(loopVar).append("++)");
			return newstr;
		}
	}

	return "";
}

//Redeclares the vars in varlist by pushing them to outlist
//Note: Array types make this function confusing. If a variable is declared as int b[5], its entry in the varList will just be "b"
//All variables should be in the hashmap, so if we don't find "b", assume we're looking for "b["
void redeclareVars(strvec& varList, strvec& outList)
{
	for (int i = 0; i < varList.size(); i++)
	{
		std::string typeStr;
		std::string newLine;

		if (varsAndTypes.find(varList.at(i)) == varsAndTypes.end()) //if the var not there, should be array type
		{
			std::string varNameStr = varList.at(i);
			varNameStr = varNameStr.append("["); //add this to look for array types
			for (std::map<std::string, std::string>::iterator it = varsAndTypes.begin(); it != varsAndTypes.end(); it++)
			{
				if (it->first.substr(0, varNameStr.length()).compare(varNameStr) == 0) //if varnamestr is b[ and the entry in the map starts with b[, we've found it
				{
					//we want to declare it as int b[5], so we still need the type
					typeStr = it->second;
					std::string varNameStr = it->first;
					newLine = typeStr.append(" ").append(varNameStr).append(";");
				}
			}
		}
		else //if it's in there, append like always
		{
			typeStr = varsAndTypes[varList.at(i)];
			newLine = typeStr.append(" ").append(varList.at(i)).append(";");
		}

		outList.push_back(newLine);
	}
}

//Eliminates duplicates from the global vars
void eliminateDuplicates(strvec& vecRef)
{
	std::set<std::string> tempSet(vecRef.begin(), vecRef.end()); //sets only allow unique elements! just stuff the whole vector in, then:
	vecRef.clear();
	vecRef.assign(tempSet.begin(), tempSet.end());
}

//Simple vector move function, pushes from from to to
void add(strvec& from, strvec& to)
{
	for (int i = 0; i < from.size(); i++)
	{
		to.push_back(from.at(i));
	}
}

//Declares the varlist in the main function
void declarePrivatesInMain(strvec& varList)
{
	int mainLine = 0;
	for (mainLine; mainLine < input.size(); mainLine++)
	{
		std::string curStr = input.at(mainLine);
		if (curStr.compare("int main()") == 0)
		{
			break;
		}
	}

	mainLine += 2; //move past brackets
	for (int i = 0; i < varList.size(); i++)
	{
		input.insert(input.begin() + mainLine++, varList.at(i));
	}
}