#include <string>
#include <stdint.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include "assembler.h"

token::token()
	: type((tokenType)0), dataT((dataType)0), stringData(""), intData(0), lineNumber(0)
{}

bool lexer::isAllNumber(std::string str)
{
	for (int i = 0; i < str.size(); i++)
	{
		if (!(str[i] >= 48) || !(str[i] <= 57))
		{
			return false;
		}
	}
	return true;
}

void lexer::sanitizeAssembly(std::vector<std::string>& originalVec, std::vector<std::pair<unsigned int, std::string>>& returnVec)
{
	for (int i = 0; i < originalVec.size(); i++)
	{
		std::string sanitizedString = originalVec[i];
		lexer::removeComment(sanitizedString);
		lexer::trimEndString(sanitizedString);
		lexer::trimBeginString(sanitizedString);
		if (sanitizedString.size() == 0)
		{
			continue;
		}
		returnVec.push_back(std::pair<unsigned int, std::string>(i + 1, sanitizedString));
	}

}

void lexer::trimBeginString(std::string& toTrim)
{
	bool spaceTrimming = true;
	std::string tmp = "";
	for (int i = 0; i < toTrim.size(); i++)
	{
		if (toTrim[i] != ' ' && toTrim[i] != '\t')
		{
			spaceTrimming = false;
		}

		if (spaceTrimming)
		{
			continue;
		}

		tmp += toTrim[i];
	}
	toTrim = tmp;
}

void lexer::trimEndString(std::string& toTrim)
{
	bool spaceTrimming = true;
	std::string tmp = "";
	for (int i = (int)toTrim.size() - 1; i >= 0; i--)
	{
		if (toTrim[i] != ' ' && toTrim[i] != '\t')
		{
			spaceTrimming = false;
		}
		if (spaceTrimming)
		{
			continue;
		}

		tmp = toTrim[i] + tmp;
	}
	toTrim = tmp;
}

void lexer::removeComment(std::string& toRemove)
{
	std::string tmp = "";
	for (int i = 0; i < toRemove.size(); i++)
	{
		if (toRemove[i] == '#')
		{
			break;
		}
		tmp += toRemove[i];
	}
	toRemove = tmp;
}

bool lexer::lexcialAnalyzer(std::vector<token>& tokenList, std::pair<unsigned int, std::string>line, std::string& error)
{
	std::string currentIngest = "";
	bool textChunk = false;
	bool textChunkEnded = false;
	unsigned int wordIndex = 0;
	for (int i=0;i<line.second.size();i++)
	{
		if (line.second[i] == '"')
		{
			textChunk = !textChunk;
			if (!textChunk)
			{
				textChunkEnded = true;
			}
			continue;
		}
		if (line.second[i] == ' ' && !textChunk)
		{
			token Token;
			if (convertToken(line.first, wordIndex, textChunkEnded, currentIngest, Token))
			{
				tokenList.push_back(Token);
			}
			textChunkEnded = false;
			wordIndex++;
			currentIngest = "";
			continue;
		}
		if (textChunkEnded)
		{	
			std::stringstream errorStream;
			errorStream << "Invalid characters after quotation, on line " << line.first << ", near column " << i;
			error = errorStream.str();
			return false;
		}
		currentIngest += line.second[i];
	}

	token Token;
	if (convertToken(line.first, wordIndex, textChunkEnded, currentIngest, Token))
	{
		tokenList.push_back(Token);
	}
	error = "none";
	return true;
}

bool lexer::convertToken(unsigned int lineNumber, unsigned int wordIndex, bool inTextChunk, std::string word, token& returnToken)
{
	returnToken.lineNumber = lineNumber;
	if (word.size() == 0)
	{
		return false;
	}
	if (word[word.size() - 1] == ':')
	{
		returnToken.type = token::tokenType::label;
		returnToken.dataT = token::dataType::string;
		returnToken.stringData = word;
		return true;
	}
	if (word[0] == '0' && word[1] == 'x')
	{
		returnToken.type = token::tokenType::oprand;
		returnToken.dataT = token::dataType::hex;
		santizeHex(word);
		returnToken.intData = hex2int(word);
		return true;
	}
	if (isAllNumber(word))
	{
		returnToken.type = token::tokenType::oprand;
		returnToken.dataT = token::dataType::integer;
		returnToken.intData = atoi(word.c_str());
		return true;
	}
	if (inTextChunk || word[word.size() - 1] == '"')
	{
		returnToken.type = token::tokenType::stringChunk;
		returnToken.dataT = token::dataType::string;
		returnToken.stringData = word;
		return true;
	}
	if (wordIndex == 0)
	{
		returnToken.type = token::tokenType::instruction;
		returnToken.dataT = token::dataType::string;
		returnToken.stringData = word;
		return true;
	}
	else {
		returnToken.type = token::tokenType::inlineLabel;
		returnToken.dataT = token::dataType::string;
		returnToken.stringData = word;
		return true;
	}
	returnToken.type = token::tokenType::label;
	returnToken.dataT = token::dataType::string;
	returnToken.stringData = word;
	return true;
}

bool lexer::readFile(std::string assemblyFilePath, std::vector<std::string>& stringVec)
{
	std::string stringBuff;
	std::ifstream AsmFile(assemblyFilePath);

	if (!AsmFile.is_open())
	{
		return false;
	}

	while (std::getline(AsmFile, stringBuff)) {
		// Output the text from the file
		stringVec.push_back(stringBuff);
	}

	// Close the file
	AsmFile.close();
	return true;
}


void lexer::santizeHex(std::string& word)
{
	std::string output = "";
	bool inZeroPadding = true;
	for (int i = 0; i < word.size(); i++)
	{
		if (word[i] == 'x')
		{
			continue;
		}
		if (word[i] == '0' && inZeroPadding)
		{
			continue;
		}
		else
		{
			inZeroPadding = false;
		}
		unsigned char nibble = (unsigned char)word[i];
		if (nibble > 70)
		{
			nibble -= 32;
		}

		if ((nibble >= 0x30 &&
			nibble <= 0x39) ||
			(nibble >= 0x41 &&
				nibble <= 0x46))
		{
			output += nibble;
		}
		else
		{
			std::cout << "INVALID HEX CHARACTER\n";
			throw;
		}
	}
	word = output;
}



char lexer::hex2char(char n)
{
	n = (n > 70) ? n - 32 : n;

	if (n >= '0' && n <= '9')
	{
		return n - '0';
	}
	else if (n >= 'A' && n <= 'F')
	{
		return n - 'A' + 10;
	}
	return 0;
}

uint32_t lexer::hex2int(std::string n)
{
	uint32_t number = 0;
	for (int i = 0; i < n.size(); i++)
	{
		number = number << 4 | hex2char(n[i]);
	}
	return number;
}
