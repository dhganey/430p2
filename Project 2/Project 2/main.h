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
void processGetThreadNum();
bool processParallel();
bool processParallelFor();
void parallelHelper(int start, int end);
void parallelForHelper(int start, int end);

void insertAfterIncludes(std::vector<std::string>& vecRef);

std::vector<std::string> getNewStructPriv(std::vector<std::string>& privVars, std::string& newStructName);