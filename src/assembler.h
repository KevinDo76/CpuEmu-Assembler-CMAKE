#pragma once
#include <stdint.h>
#include <vector>
#include <string>
#include <array>
#include <sstream>
#include <unordered_set>
#include <unordered_map>

//externs for global configs
extern uint32_t BINARY_ORIGIN;

class token
{
public:
	enum tokenType {
		instruction,
		label,
		inlineLabel,
		oprand,
		stringChunk,
		directive
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
	std::string filePath;

	token();
};

class syntaxBlock
{
public:
	uint32_t lineNumber;
	std::string filePath;
	bool isLabel;
	bool isExternal;
	std::string mainLabelBlock;
	std::string dataSection;
	std::string instruction;
	std::vector<token> oprands;
	uint32_t memoryAddress;	
};

namespace syntax {
	uint32_t getInstructionCodeFromName(std::string);
	bool AssembleFromSyntaxBlock(uint32_t currentInstructionIndex,std::vector<std::unordered_set<std::string>>& declaredLabels, std::unordered_set<std::string>& exportedLabelList, std::vector<std::unordered_set<std::string>>& externalLabelList, syntaxBlock& syntaxObj, std::unordered_map<std::string, uint32_t>& labelMemoryMap, std::array<uint32_t, 4>& assembledBytes, std::string& error);
	bool mapSyntaxBlockToMemory(std::vector<std::vector<syntaxBlock>>& instructionLists, uint32_t startAddress, uint32_t& returnMemorySize, std::stringstream& errorStream);
	bool checkValidInstructionToken(std::string instructionName, std::vector<token>& tokenList, unsigned int& instructionIndex, syntaxBlock& syntaxObj);
	bool createInstructionSyntaxBlock(syntaxBlock& syntaxObj, std::vector<token>& tokenList, unsigned int& instructionIndex, std::string& error);
	bool checkOprand(std::vector<token>, unsigned int instructionIndex, unsigned int oprandCount, syntaxBlock& syntaxObj);
	void toLowerCase(std::string& word);
	uint32_t flipEndian(uint32_t n);
	void registerBuiltinLabels(std::vector<syntaxBlock>& labelList, std::vector<std::unordered_set<std::string>>& declaredLabels, std::unordered_map<std::string, uint32_t>& labelMemoryMap);
	bool Assemble(std::vector<std::vector<token>>& tokenList, std::string BinaryFilePath, std::stringstream& errorStream);
}

namespace lexer {
	bool readFile(std::string assemblyFilePath, std::vector<std::string>& stringVec);
	bool isAllNumber(std::string str);
	void sanitizeAssembly(std::vector<std::string>& originalVec, std::vector<std::pair<unsigned int, std::string>>& returnVec);
	void trimBeginString(std::string& toTrim);
	void trimEndString(std::string& toTrim);
	void removeComment(std::string& toRemove);
	bool lexcialAnalyzer(std::vector<token>& tokenList, std::pair<unsigned int, std::string>line, std::string& error);
	bool convertToken(unsigned int lineNumber, unsigned int wordIndex, bool inTextChunk, std::string word, token& returnToken, bool& errorFound, std::stringstream& errorMessage);
	void santizeHex(std::string& word);
	char hex2char(char n);
	uint32_t hex2int(std::string n);
}

namespace assembler {
	bool assembledFile(const std::vector<std::string> pathIn, std::string pathOut, std::string& error);
}
