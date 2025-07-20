#include <iostream>
#include "assembler.h"
bool assembler::assembledFile(std::vector<std::string> pathsIn, std::string pathOut, std::string &errorReturn)
{

	std::vector<std::vector<std::string>> AsmStringBuff(pathsIn.size());
    std::vector<std::vector<std::pair<unsigned int, std::string>>> AsmCleanStringBuff(pathsIn.size());
    std::vector<std::vector<token>> tokenList(pathsIn.size());
    
	for (int currentAsmIndex=0; currentAsmIndex<pathsIn.size();currentAsmIndex++)
	{
    	if (!lexer::readFile(pathsIn[currentAsmIndex], AsmStringBuff[currentAsmIndex]))
		{
			std::cout<<"Unable to read source\n";
    	    errorReturn += "Unable to read source";
			return false;
		}
		lexer::sanitizeAssembly(AsmStringBuff[currentAsmIndex], AsmCleanStringBuff[currentAsmIndex]);

    	for (int i = 0; i < AsmCleanStringBuff[currentAsmIndex].size(); i++)
		{
			std::string error;
			if (!lexer::lexcialAnalyzer(tokenList[currentAsmIndex], AsmCleanStringBuff[currentAsmIndex][i], error))
			{
				std::cout << error << "\n";
				std::cout << "Failed to tokenize, exiting with message: \n" << error;
    	        errorReturn += "Failed to tokenize, exiting with message: \n";
    	        errorReturn += error;
				return false;
			}
			for (int tokenIndex = 0; tokenIndex < tokenList[currentAsmIndex].size();tokenIndex++)
			{
				tokenList[currentAsmIndex][tokenIndex].filePath = pathsIn[currentAsmIndex];
			}
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