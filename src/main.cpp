#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "assembler.h"
#include <sstream>

int main()
{
	std::string assemblyFilePath = "program.casm";
	std::string binaryOutputFilePath = "./program.bin";

	std::string errorList;
	assembler::assembledFile(assemblyFilePath, binaryOutputFilePath, errorList);
	return 0;
}