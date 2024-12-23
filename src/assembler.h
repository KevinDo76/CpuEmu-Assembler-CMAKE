#pragma once
#include <stdint.h>
#include <vector>
#include <string>
#include <array>
#include <sstream>
class token
{
public:
	enum tokenType {
		instruction,
		label,
		inlineLabel,
		oprand,
		stringChunk,
	};

	enum dataType {
		hex,
		integer,
		string,
	};

	tokenType type;
	dataType dataT;
	std::string stringData;
	uint32_t intData;
	uint32_t lineNumber;

	token();
};

class syntaxBlock
{
public:
	uint32_t lineNumber;
	bool isLabel;
	std::string instruction;
	std::vector<token> oprands;
	uint32_t memoryAddress;	
};

namespace syntax {
	uint32_t getInstructionCodeFromName(std::string);
	bool AssembleFromSyntaxBlock(syntaxBlock& syntaxObj, std::vector<syntaxBlock>& labelList, std::array<uint32_t, 4>& assembledBytes, std::string& error);
	uint32_t mapSyntaxBlockToMemory(std::vector<syntaxBlock>& instructionList);
	bool checkValidInstructionToken(std::string instructionName, std::vector<token>& tokenList, unsigned int& instructionIndex, syntaxBlock& syntaxObj);
	bool createInstructionSyntaxBlock(syntaxBlock& syntaxObj, std::vector<token>& tokenList, unsigned int& instructionIndex, std::string& error);
	bool checkOprand(std::vector<token>, unsigned int instructionIndex, unsigned int oprandCount, syntaxBlock& syntaxObj);
	void toLowerCase(std::string& word);
	uint32_t flipEndian(uint32_t n);

	bool Assemble(std::vector<token>& tokenList, std::string BinaryFilePath, std::stringstream& error);
}

namespace lexer {
	bool readFile(std::string assemblyFilePath, std::vector<std::string>& stringVec);
	bool isAllNumber(std::string str);
	void sanitizeAssembly(std::vector<std::string>& originalVec, std::vector<std::pair<unsigned int, std::string>>& returnVec);
	void trimBeginString(std::string& toTrim);
	void trimEndString(std::string& toTrim);
	void removeComment(std::string& toRemove);
	bool lexcialAnalyzer(std::vector<token>& tokenList, std::pair<unsigned int, std::string>line, std::string& error);
	bool convertToken(unsigned int lineNumber, unsigned int wordIndex, bool inTextChunk, std::string word, token& returnToken);
	void santizeHex(std::string& word);
	char hex2char(char n);
	uint32_t hex2int(std::string n);
}
