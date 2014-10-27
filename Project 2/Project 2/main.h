#include <string>
#include <vector>

void trimLeft(std::string& str);
void trimRight(std::string& str);

void readInput();

void printVector(std::vector<std::string>& vecRef);

int getNumThreads(std::string str);
int getNumIterations(std::string str);
std::vector<std::string> getConstructVars(std::string str, std::string criteria);

void processVariables();
bool processParallel();
void parallelHelper(int start, int end);

void insertAfterIncludes(std::vector<std::string>& vecRef);