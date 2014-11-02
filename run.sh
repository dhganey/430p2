g++ main.cpp main.h -std=c++11 -o pj02
./pj02 < input.cc > output.cc
g++ output.cc -lpthread -o out
./out
