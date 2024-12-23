#include "assembler.h"
#include <iostream>
#include <fstream>
#include <sstream>

#define FORCE_INSTRUCTION_ALIGNMENT 0

bool syntax::Assemble(std::vector<token>& tokenList, std::string BinaryFilePath, std::stringstream& errorStream)
{
    std::vector<syntaxBlock> instructionList;
    std::vector<syntaxBlock> labelList;
    bool errorFound = false;

    for (unsigned int i = 0; i < tokenList.size(); i++)
    {
        if (tokenList[i].type == token::tokenType::instruction)
        {
            syntaxBlock instructionBlock;
            std::string error;
            if (!createInstructionSyntaxBlock(instructionBlock, tokenList, i, error))
            {
                errorStream << error << "\n";
                errorFound = true;
                continue;
            }
            instructionList.push_back(instructionBlock);
        }

        if (tokenList[i].type == token::tokenType::label)
        {
            syntaxBlock labelBlock;
            labelBlock.isLabel = true;
            for (int j = 0; j < tokenList[i].stringData.size(); j++)
            {
                if (tokenList[i].stringData[j] != ':')
                {
                    labelBlock.instruction += tokenList[i].stringData[j];
                }
            }
            instructionList.push_back(labelBlock);
        }
    }

    if (errorFound)
    {
        return false;
    }

    uint32_t memorySize = mapSyntaxBlockToMemory(instructionList);
    char* memoryBuff = new char[memorySize] {0};
    for (int i = 0; i < instructionList.size(); i++)
    {
        if (instructionList[i].isLabel)
        {
            labelList.push_back(instructionList[i]);
        }
    }

    //creating label entries for registers
    syntaxBlock registerLabel;
    registerLabel.instruction = "ra";
    registerLabel.isLabel = true;
    registerLabel.memoryAddress = 0x0;
    labelList.push_back(registerLabel);
    registerLabel.instruction = "rb";
    registerLabel.memoryAddress = 0x1;
    labelList.push_back(registerLabel);
    registerLabel.instruction = "rc";
    registerLabel.memoryAddress = 0x2;
    labelList.push_back(registerLabel);
    registerLabel.instruction = "rd";
    registerLabel.memoryAddress = 0x3;
    labelList.push_back(registerLabel);
    registerLabel.instruction = "cmpreg";
    registerLabel.memoryAddress = 0x4;
    labelList.push_back(registerLabel);
    registerLabel.instruction = "sp";
    registerLabel.memoryAddress = 0x5;
    labelList.push_back(registerLabel);
    registerLabel.instruction = "bp";
    registerLabel.memoryAddress = 0x6;
    labelList.push_back(registerLabel);

    for (int i = 0; i < instructionList.size(); i++)
    {
        std::string error;
        std::array<uint32_t, 4>assembledBytes = { 0 };
        uint32_t startAddress = instructionList[i].memoryAddress;

        if (instructionList[i].isLabel)
        {
            continue;
        }

        if (instructionList[i].instruction == "string")
        {
            uint32_t offset = 0;
            for (int oprandIndex = 0; oprandIndex < instructionList[i].oprands.size(); oprandIndex++)
            {
                if (instructionList[i].oprands[oprandIndex].type == token::tokenType::stringChunk)
                {
                    for (int stringIndex = 0; stringIndex < instructionList[i].oprands[oprandIndex].stringData.size(); stringIndex++)
                    {
                        memoryBuff[startAddress + offset] = instructionList[i].oprands[oprandIndex].stringData[stringIndex];
                        offset++;
                    }
                }  
                if (instructionList[i].oprands[oprandIndex].dataT == token::dataType::hex) 
                {
                    memoryBuff[startAddress + offset] = (char)instructionList[i].oprands[oprandIndex].intData;
                    offset++;
                }
                if (instructionList[i].oprands[oprandIndex].dataT == token::dataType::integer)
                {
                    memoryBuff[startAddress + offset] = (char)instructionList[i].oprands[oprandIndex].intData;
                    offset++;
                }
            }
            continue;
        }

        if (!AssembleFromSyntaxBlock(instructionList[i], labelList, assembledBytes, error))
        {
            errorStream << error << "\n";
            errorFound = true;
            continue;
        }
        
        for (int j = 0; j < 4; j++)
        {
            
            //((uint32_t* )memoryBuff)[(startAddress/4) + j] = assembledBytes[j];
            memoryBuff[startAddress + j * 0x04 + 0] = assembledBytes[j] & 0xff;
            memoryBuff[startAddress + j * 0x04 + 1] = (assembledBytes[j] & 0xff00) >> 8;
            memoryBuff[startAddress + j * 0x04 + 2] = (assembledBytes[j] & 0xff0000) >> 16;
            memoryBuff[startAddress + j * 0x04 + 3] = (assembledBytes[j] & 0xff000000) >> 24;
        }
    }

    if (errorFound)
    {
        return false;
    }

    std::ofstream binaryFile(BinaryFilePath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!binaryFile.is_open())
    {
        errorStream << "Unable to open file path\"" << BinaryFilePath << "\"\n";
        return false;
    }

    binaryFile.write(memoryBuff, memorySize);
    binaryFile.close();
    delete[] memoryBuff;


    return true;
}

uint32_t syntax::mapSyntaxBlockToMemory(std::vector<syntaxBlock>& instructionList)
{
    uint32_t memoryCounter = 0;
    for (int i = 0; i < instructionList.size(); i++)
    {

        if (instructionList[i].instruction == "string")
        {
            instructionList[i].memoryAddress = memoryCounter;
            uint32_t dataBlockSize = 0;
            for (int oprandsIndex = 0; oprandsIndex < instructionList[i].oprands.size(); oprandsIndex++)
            {
                if (instructionList[i].oprands[oprandsIndex].dataT == token::dataType::string)
                {
                    dataBlockSize += instructionList[i].oprands[oprandsIndex].stringData.size();
                }
                if (instructionList[i].oprands[oprandsIndex].dataT == token::dataType::hex)
                {
                    dataBlockSize++;
                }
                if (instructionList[i].oprands[oprandsIndex].dataT == token::dataType::integer)
                {
                    dataBlockSize++;
                }
            }
            memoryCounter += dataBlockSize;
            continue;
        }
        
        if (memoryCounter % 0x10 != 0 && FORCE_INSTRUCTION_ALIGNMENT)
        {
            memoryCounter = memoryCounter - (memoryCounter % 0x10) + 0x10;
        }
        instructionList[i].memoryAddress = memoryCounter;

        if (!instructionList[i].isLabel)
        {
            memoryCounter += 0x10;
        }

    }
    return memoryCounter;
}

bool syntax::checkValidInstructionToken(std::string instructionName, std::vector<token>& tokenList, unsigned int& instructionIndex, syntaxBlock& syntaxObj)
{
    const std::pair<std::string, unsigned int> INSTRUCTION_LIST[] = { {"mov",2}, {"out",2}, {"add", 2}, {"and", 2}, {"xor", 2}, {"sub", 2}, {"or", 2}, {"readptr1", 2}, {"jmpimm", 1},{"jmpif",2}, {"cmp",3}, {"halt",0}, {"ret",0}, {"call", 1}, {"push", 1}, {"pop", 1}, {"inc", 1}, {"dec", 1}, {"pushreg", 0}, {"popreg", 0} };

    for (int i = 0; i < sizeof INSTRUCTION_LIST / sizeof INSTRUCTION_LIST[0]; i++)
    {
        if (instructionName == "string")
        {
            unsigned int offset = 1;
            while ((instructionIndex + offset) < tokenList.size() && tokenList[instructionIndex + offset].lineNumber == tokenList[instructionIndex].lineNumber)
            {
                syntaxObj.instruction = instructionName;
                syntaxObj.oprands.push_back(tokenList[instructionIndex + offset]);
                offset++;
            }
            return true;
        }
        //std::cout << instructionList[i].first << "\n";
        if (instructionName == INSTRUCTION_LIST[i].first)
        {
            syntaxObj.instruction = instructionName;
            return checkOprand(tokenList, instructionIndex, INSTRUCTION_LIST[i].second, syntaxObj);
        }
    }
    return false;
}

uint32_t syntax::getInstructionCodeFromName(std::string name)
{
    if (name == "mov") { return 0x19; }
    if (name == "jmp") { return 0x1b; }
    if (name == "jmpimm") { return 0x1c; }
    if (name == "out") { return 0x20; }
    if (name == "add") { return 0x11; }
    if (name == "readptr1") { return 0x0a; }
    if (name == "jmpif") { return 0x1d; }
    if (name == "cmp") { return 0x1a; }
    if (name == "halt") { return 0x23; }
    if (name == "ret") { return 0x2; }
    if (name == "call") { return 0x1; }
    if (name == "pop") { return 0x03; }
    if (name == "push") { return 0x04; }
    if (name == "inc") { return 0x24; }
    if (name == "dec") { return 0x25; }
    if (name == "pushreg") { return 0x26; }
    if (name == "popreg") { return 0x27; }

    return 0;
}

bool syntax::AssembleFromSyntaxBlock(syntaxBlock& syntaxObj, std::vector<syntaxBlock>& labelList, std::array<uint32_t, 4>& assembledBytes, std::string& error)
{
    if (syntaxObj.oprands.size() > 3)
    {
        error = "Syntax Error: Incorrect oprands count";
        return false;
    }

    assembledBytes[0] = getInstructionCodeFromName(syntaxObj.instruction);

    for (int i = 0; i < syntaxObj.oprands.size(); i++)
    {
        //std::cout << syntaxObj.instruction << "\n";

        if (syntaxObj.oprands[i].type == token::tokenType::inlineLabel)
        {
            bool found = false;
            for (int labelIndex = 0; labelIndex < labelList.size(); labelIndex++)
            {
                if (labelList[labelIndex].instruction != syntaxObj.oprands[i].stringData)
                {
                    continue;
                }
                found = true;
                assembledBytes[i+1] = (labelList[labelIndex].memoryAddress);
            }
            if (!found)
            {
                std::stringstream returnError;
                returnError << "Unknown label, \"" + syntaxObj.oprands[i].stringData + "\", on line " << syntaxObj.lineNumber;
                error = returnError.str();
                return false;
            }
            continue;
        }

        assembledBytes[i + 1] = (syntaxObj.oprands[i].intData);

    }
    return true;
}

bool syntax::createInstructionSyntaxBlock(syntaxBlock& syntaxObj, std::vector<token>& tokenList, unsigned int& instructionIndex, std::string& error)
{
    std::stringstream errorStream;
    std::string instructionName = tokenList[instructionIndex].stringData;
    toLowerCase(instructionName);
    syntaxObj.isLabel = false;
    errorStream << "Invalid oprands: wrong count, on line " << tokenList[instructionIndex].lineNumber;
    error = errorStream.str();
    syntaxObj.lineNumber = tokenList[instructionIndex].lineNumber;
    if (checkValidInstructionToken(instructionName, tokenList, instructionIndex, syntaxObj))
    {
        error = "";
        return true;
    }

    errorStream.str("");
    errorStream << "Invalid instruction, on line " << tokenList[instructionIndex].lineNumber << ": \"" << instructionName << "\" Unknown instruction";
    error = errorStream.str();
    return false;
}

bool syntax::checkOprand(std::vector<token>tokenList, unsigned int instructionIndex, unsigned int oprandCount, syntaxBlock& syntaxObj)
{
    for (int i = 1; i <= oprandCount; i++)
    {
        if ((instructionIndex + i) > (tokenList.size() - 1))
        {
            return false;
        }
        if (tokenList[instructionIndex + i].type != token::tokenType::oprand && tokenList[instructionIndex + i].type != token::tokenType::inlineLabel)
        {
            return false;
        }
        if (tokenList[instructionIndex].lineNumber != tokenList[instructionIndex + i].lineNumber)
        {
            return false;
        }
    }

    for (int i = 1; i <= oprandCount; i++)
    {
        syntaxObj.oprands.push_back(tokenList[instructionIndex + i]);
    }

    return true;
}

uint32_t syntax::flipEndian(uint32_t n)
{
    return (n << 24) | ((n << 8) & 0x00ff0000) | ((n >> 8) & 0x0000ff00) | ((n >> 8) & 0x0000ff00) | ((n >> 24) & 0x000000ff);
}

void syntax::toLowerCase(std::string& word)
{
    std::string temp = "";
    for (int i = 0; i < word.size(); i++)
    {
        if (word[i] >= 65 && word[i] <= 90)
        {
            temp += (char)(word[i]+32);
            continue;
        }
        temp += word[i];
    }
    word = temp;
}
