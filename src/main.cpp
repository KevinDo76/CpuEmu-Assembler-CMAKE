#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "assembler.h"
#include <sstream>

int main()
{
	std::vector<std::string> assemblyPathList;
	assemblyPathList.push_back("program.casm");
	assemblyPathList.push_back("bios.casm");

	std::string binaryOutputFilePath = "./program.bin";

	std::string errorList;
	assembler::assembledFile(assemblyPathList, binaryOutputFilePath, errorList);
	return 0;
}