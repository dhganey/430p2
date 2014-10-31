#include <string>
#include <vector>

typedef std::vector<std::string> strvec;

void trimLeft(std::string& str);
void trimRight(std::string& str);

void readInput();

void printVector(strvec& vecRef);

int getNumThreads(std::string str);
int getNumIterations(std::string str);
strvec getConstructVars(std::string str, std::string criteria);

void processVariables();
void processGetThreadNum();
bool processParallel();
bool processParallelFor();
void parallelHelper(int start, int end);
void parallelForHelper(int start, int end);
strvec handlePrivateAndShared(strvec& privVars, strvec& sharedVars, strvec& newFunction);
void createNewFunction(strvec& newFunction, strvec& globalVars, int start, int end, strvec& privVars, strvec& sharedVars);

void insertAfterIncludes(strvec& vecRef);