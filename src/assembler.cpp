#include <iostream>
#include "assembler.h"

bool assembler::assembledFile(std::string pathIn, std::string pathOut, std::string &errorReturn)
{
	std::vector<std::string> AsmStringBuff;
    std::vector<std::pair<unsigned int, std::string>> AsmCleanStringBuff;
    std::vector<token> tokenList;
    
    if (!lexer::readFile(pathIn, AsmStringBuff))
	{
		std::cout<<"Unable to read source\n";
        errorReturn += "Unable to read source";
		return false;
	}
	lexer::sanitizeAssembly(AsmStringBuff, AsmCleanStringBuff);

    for (int i = 0; i < AsmCleanStringBuff.size(); i++)
	{
		std::string error;
		if (!lexer::lexcialAnalyzer(tokenList, AsmCleanStringBuff[i], error))
		{
			std::cout << error << "\n";
			std::cout << "Failed to tokenize, exiting with message: \n" << error;
            errorReturn += "Failed to tokenize, exiting with message: \n";
            errorReturn += error;
			return false;
		}
	}

    std::stringstream errorStream;
	if (!syntax::Assemble(tokenList, pathOut, errorStream))
	{
		std::cout << errorStream.str();
		std::cout << "Failed to assemble\n";
        errorReturn += errorStream.str();
        errorReturn += "Failed to assemble\n";
        return false;
	}

    return true;
}