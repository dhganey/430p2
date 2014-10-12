#include <string>
#include <vector>

void trimLeft(std::string& str);
void trimRight(std::string& str);

void readInput(std::vector<std::string>& vecRef);

void printVector(std::vector<std::string>& vecRef);

bool processPragmas(std::vector<std::string>& input, std::vector<std::string>& output);

void processParallelFor(std::vector<std::string>& input, std::vector<std::string>& output, int start);
int getNumThreads(std::string str);
int getNumIterations(std::string str);