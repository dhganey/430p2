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
bool processCritical();
bool processSingle();

void parallelHelper(int start, int end);
void parallelForHelper(int start, int end);
void criticalHelper(int start, int end);
void singleHelper(int start, int end);

void insertAfterIncludes(strvec& vecRef);

strvec createStartEndStruct();

std::string fixForLine(std::string forline);

void redeclareVars(strvec& varList, strvec& outList);
void refineGlobalVars();

void redeclareVarsInMain(strvec& varList);