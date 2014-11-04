#include <string>
#include <vector>

typedef std::vector<std::string> strvec;

//input functions
void trimLeft(std::string& str);
void trimRight(std::string& str);
void readInput();

//output functions
void printVector(strvec& vecRef);

//line analysis functions
int getNumThreads(std::string str);
int getNumIterations(std::string str);
strvec getConstructVars(std::string str, std::string criteria);

//these iterate through the whole program
void processVariables();
void processGetThreadNum();

//primary functions called from main
bool processParallel();
bool processParallelFor();
bool processCritical();
bool processSingle();

//work functions called from primary functions
void parallelHelper(int start, int end);
void parallelForHelper(int start, int end);
void criticalHelper(int start, int end);
void singleHelper(int start, int end);

//processing done in main to add content to input
void insertAfterIncludes(strvec& vecRef);
strvec createStartEndStruct();

//adjust the for in a #pragma omp for to use the struct instead of its loopvar
std::string fixForLine(std::string forline);

//variable manipulation--redeclares a varlist by pushing it into outlist
void redeclareVars(strvec& varList, strvec& outList);
void eliminateDuplicates(strvec& vecRef);
void declarePrivatesInMain(strvec& varList);

//simple vector move function--pushes everything in from to to
void add(strvec& from, strvec& to);

