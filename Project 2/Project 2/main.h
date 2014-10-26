#include <string>
#include <vector>

void trimLeft(std::string& str);
void trimRight(std::string& str);

void readInput(std::vector<std::string>& vecRef);

void printVector(std::vector<std::string>& vecRef);

int getNumThreads(std::string str);
int getNumIterations(std::string str);

void processPragmas();