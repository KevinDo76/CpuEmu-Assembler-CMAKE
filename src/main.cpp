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
	std::vector<std::string> AsmStringBuff;
	std::vector<std::pair<unsigned int, std::string>> AsmCleanStringBuff;

	if (!lexer::readFile(assemblyFilePath, AsmStringBuff))
	{
		std::cout<<"Unable to read source\n";
		return 0;
	}
	lexer::sanitizeAssembly(AsmStringBuff, AsmCleanStringBuff);

	std::vector<token> tokenList;

	for (int i = 0; i < AsmCleanStringBuff.size(); i++)
	{
		std::string error;
		if (!lexer::lexcialAnalyzer(tokenList, AsmCleanStringBuff[i], error))
		{
			std::cout << error << "\n";
			std::cout << "Failed to tokenize, exiting\n";
			return 1;
		}
	}

	std::stringstream errorStream;
	if (!syntax::Assemble(tokenList, binaryOutputFilePath, errorStream))
	{
		std::cout << errorStream.str();
		std::cout << "Failed to assemble\n";
	}

	return 0;
}