#include "assembler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#define FORCE_INSTRUCTION_ALIGNMENT 0

bool syntax::Assemble(std::vector<std::vector<token>>& tokenList, std::string BinaryFilePath, std::stringstream& errorStream)
{
    std::vector<std::vector<syntaxBlock>> instructionList(tokenList.size());
    std::vector<std::unordered_set<std::string>> declaredLabels(tokenList.size());
    std::unordered_set<std::string> exportedLabelList;
    std::vector<std::unordered_set<std::string>> externalLabelList(tokenList.size());
    std::unordered_map<std::string, uint32_t> labelMemoryMap;
    std::vector<syntaxBlock> labelList;
    bool errorFound = false;
    
    for (uint32_t currentTokenListIndex = 0; currentTokenListIndex < tokenList.size();currentTokenListIndex++)
    {
        std::string mainLabelBlock;
        std::string currentSection = "TEXT";
        std::unordered_set<std::string> currentFileExportList;
        for (unsigned int i = 0; i < tokenList[currentTokenListIndex].size(); i++)
        {
            if (tokenList[currentTokenListIndex][i].type == token::tokenType::instruction)
            {

                syntaxBlock instructionBlock;
                std::string error;
                instructionBlock.mainLabelBlock=mainLabelBlock;
                instructionBlock.dataSection = currentSection;
                instructionBlock.isExternal = false;
                instructionBlock.isLabel = false;
                instructionBlock.filePath = tokenList[currentTokenListIndex][i].filePath;
                if (!createInstructionSyntaxBlock(instructionBlock, tokenList[currentTokenListIndex], i, error))
                {
                    errorStream << error << "\n";
                    errorFound = true;
                    continue;
                }
                instructionList[currentTokenListIndex].push_back(instructionBlock);
            }
            
            if (tokenList[currentTokenListIndex][i].type == token::tokenType::directive)
            {
                bool oprandError = false;
                bool directiveFound = false;
                if (tokenList[currentTokenListIndex][i].stringData=="%GLOBAL")
                {
                    directiveFound = true;
                    if (i+1<tokenList[currentTokenListIndex].size() && (tokenList[currentTokenListIndex][i+1].dataT == token::dataType::string))
                    {
                        exportedLabelList.insert(tokenList[currentTokenListIndex][i+1].stringData);
                        currentFileExportList.insert(tokenList[currentTokenListIndex][i+1].stringData);
                    }
                    else
                    {
                        oprandError = true;
                    }
                }

                if (tokenList[currentTokenListIndex][i].stringData=="%EXTERN")
                {
                    directiveFound = true;
                    if (i+1<tokenList[currentTokenListIndex].size() && (tokenList[currentTokenListIndex][i+1].dataT == token::dataType::string))
                    {
                        externalLabelList[currentTokenListIndex].insert(tokenList[currentTokenListIndex][i+1].stringData);
                    }
                    else
                    {
                        oprandError = true;
                    }
                }
                if (tokenList[currentTokenListIndex][i].stringData=="%ORG")
                {
                    directiveFound = true;
                    if (i+1<tokenList[currentTokenListIndex].size() && (tokenList[currentTokenListIndex][i+1].dataT == token::dataType::integer || tokenList[currentTokenListIndex][i+1].dataT == token::dataType::hex))
                    {
                        BINARY_ORIGIN = tokenList[currentTokenListIndex][i+1].intData;
                    }
                    else
                    {
                        oprandError = true;
                    }
                }
                if (tokenList[currentTokenListIndex][i].stringData=="%SECTION")
                {
                    directiveFound = true;
                    if (i+1<tokenList[currentTokenListIndex].size() && tokenList[currentTokenListIndex][i+1].dataT == token::dataType::string)
                    {
                        currentSection = tokenList[currentTokenListIndex][i+1].stringData;
                    }
                }
                if (!directiveFound)
                {
                    errorStream << "Invalid directive "<<tokenList[currentTokenListIndex][i].stringData<<" on line " <<tokenList[currentTokenListIndex][i].lineNumber<<"\n";
                }
                if (oprandError)
                {
                    errorStream << "Invalid directive parameter on line "<<tokenList[currentTokenListIndex][i].lineNumber<<"\n";
                }
                if (oprandError || !directiveFound)
                {
                    return false;
                }
            }

            if (tokenList[currentTokenListIndex][i].type == token::tokenType::label)
            {
                syntaxBlock labelBlock;
                labelBlock.isLabel = true;
                labelBlock.isExternal = false;
                labelBlock.lineNumber = tokenList[currentTokenListIndex][i].lineNumber;
                labelBlock.dataSection = currentSection;
                labelBlock.filePath = tokenList[currentTokenListIndex][i].filePath;
                bool isSubLabel = false;
                for (int j = 0; j < tokenList[currentTokenListIndex][i].stringData.size(); j++)
                {
                    if (tokenList[currentTokenListIndex][i].stringData[j] == '.' && j == 0)
                    {
                        isSubLabel = true;
                        labelBlock.instruction += mainLabelBlock;
                    }
                    if (tokenList[currentTokenListIndex][i].stringData[j] != ':')
                    {
                        labelBlock.instruction += tokenList[currentTokenListIndex][i].stringData[j];
                    }
                    if (!((tokenList[currentTokenListIndex][i].stringData[j] >= 48 && tokenList[currentTokenListIndex][i].stringData[j] <= 58) ||
                        (tokenList[currentTokenListIndex][i].stringData[j] >= 65 && tokenList[currentTokenListIndex][i].stringData[j] <= 90) ||
                        (tokenList[currentTokenListIndex][i].stringData[j] == 95) ||
                        (tokenList[currentTokenListIndex][i].stringData[j] >= 97 && tokenList[currentTokenListIndex][i].stringData[j] <= 122)||
                        (tokenList[currentTokenListIndex][i].stringData[j] == 46))) 
                    {
                        errorStream << "Invalid character for labels on line "<< tokenList[currentTokenListIndex][i].lineNumber<< ". Character: \"" << tokenList[currentTokenListIndex][i].stringData[j] << "\"" <<"\n";
                        return false;
                    }
                }
                if (!isSubLabel)
                {
                    mainLabelBlock = labelBlock.instruction;
                }
                declaredLabels[currentTokenListIndex].insert(labelBlock.instruction);
                instructionList[currentTokenListIndex].push_back(labelBlock);
            }
        }

        for (const std::string& externLabel : currentFileExportList) {
            if (declaredLabels[currentTokenListIndex].find(externLabel) == declaredLabels[currentTokenListIndex].end())
            {
                errorStream << "Attempted to export an undefined label \"" << externLabel << "\" in " << tokenList[currentTokenListIndex][0].filePath << "\n";
                return false;
            }
        }

        if (errorFound)
        {
            return false;
        }
    }



    std::vector<syntaxBlock> unifiedInstructionList;
    for (int i=0;i<instructionList.size();i++)
    {
        unifiedInstructionList.insert(unifiedInstructionList.end(), instructionList[i].begin(), instructionList[i].end());
    }

    uint32_t memorySize = 0;
    std::stringstream memoryMapErrorStream;
    if (!mapSyntaxBlockToMemory(instructionList, BINARY_ORIGIN, memorySize, memoryMapErrorStream))
    {
        errorStream << memoryMapErrorStream.str();
        return false;
    }

    std::unordered_set<std::string> existingLabel;
    char* memoryBuff = new char[memorySize] {0};

    for (uint32_t currentInstructionIndex = 0; currentInstructionIndex < tokenList.size();currentInstructionIndex++)
    {
        for (int i = 0; i < instructionList[currentInstructionIndex].size(); i++)
        {
            if (instructionList[currentInstructionIndex][i].isLabel)
            {
                if (existingLabel.find(instructionList[currentInstructionIndex][i].instruction)!=existingLabel.end())
                {
                    errorStream<<"duplicate label "<<instructionList[currentInstructionIndex][i].instruction<<", on line " << instructionList[currentInstructionIndex][i].lineNumber << " " + instructionList[currentInstructionIndex][i].filePath + "\n";
                    return false;
                }
                existingLabel.insert(instructionList[currentInstructionIndex][i].instruction);

                labelMemoryMap[instructionList[currentInstructionIndex][i].instruction] = instructionList[currentInstructionIndex][i].memoryAddress;
                labelList.push_back(instructionList[currentInstructionIndex][i]);
                std::cout<<instructionList[currentInstructionIndex][i].instruction<<"\n";
            }
        }
    }
    //creating label entries for registers
    registerBuiltinLabels(labelList, declaredLabels, labelMemoryMap);


    for (uint32_t currentInstructionIndex = 0; currentInstructionIndex < tokenList.size();currentInstructionIndex++) {
        for (int i = 0; i < instructionList[currentInstructionIndex].size(); i++)
        {
            std::string error;
            std::array<uint32_t, 4>assembledBytes = { 0 };
            uint32_t startAddress = instructionList[currentInstructionIndex][i].memoryAddress;

            if (instructionList[currentInstructionIndex][i].isLabel)
            {
                continue;
            }

            if (instructionList[currentInstructionIndex][i].instruction == "string")
            {
                uint32_t offset = 0;
                for (int oprandIndex = 0; oprandIndex < instructionList[currentInstructionIndex][i].oprands.size(); oprandIndex++)
                {
                    if (instructionList[currentInstructionIndex][i].oprands[oprandIndex].type == token::tokenType::stringChunk)
                    {
                        for (int stringIndex = 0; stringIndex < instructionList[currentInstructionIndex][i].oprands[oprandIndex].stringData.size(); stringIndex++)
                        {
                            memoryBuff[startAddress + offset] = instructionList[currentInstructionIndex][i].oprands[oprandIndex].stringData[stringIndex];
                            offset++;
                        }
                    }  
                    if (instructionList[currentInstructionIndex][i].oprands[oprandIndex].dataT == token::dataType::hex) 
                    {
                        memoryBuff[startAddress + offset] = (char)instructionList[currentInstructionIndex][i].oprands[oprandIndex].intData;
                        offset++;
                    }
                    if (instructionList[currentInstructionIndex][i].oprands[oprandIndex].dataT == token::dataType::integer)
                    {
                        memoryBuff[startAddress + offset] = (char)instructionList[currentInstructionIndex][i].oprands[oprandIndex].intData;
                        offset++;
                    }
                }
                continue;
            }

            if (instructionList[currentInstructionIndex][i].instruction == "integer")
            {  
                memoryBuff[startAddress] = instructionList[currentInstructionIndex][i].oprands[0].intData & 0xff;
                memoryBuff[startAddress + 1] = (instructionList[currentInstructionIndex][i].oprands[0].intData & 0xff00) >> 8;
                memoryBuff[startAddress + 2] = (instructionList[currentInstructionIndex][i].oprands[0].intData & 0xff0000) >> 16;
                memoryBuff[startAddress + 3] = (instructionList[currentInstructionIndex][i].oprands[0].intData & 0xff000000) >> 24;
                continue;
            }

            if (instructionList[currentInstructionIndex][i].instruction == "array")
            {
                continue;//because it's zero fill so just continue so that the memory block isn't processed
            }

            if (!AssembleFromSyntaxBlock(currentInstructionIndex, declaredLabels, exportedLabelList, externalLabelList, instructionList[currentInstructionIndex][i], labelMemoryMap, assembledBytes, error))
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

int32_t getElementIndex(std::string name, std::vector<std::string>& vectorList)
{
    for (int i = 0;i< vectorList.size(); i++) 
    {
        if (vectorList[i] == name)
        {
            return i;
        }
    }
    return -1;
}

bool syntax::mapSyntaxBlockToMemory(std::vector<std::vector<syntaxBlock>>& instructionLists, uint32_t startAddress, uint32_t& returnMemorySize, std::stringstream& errorStream)
{
    std::vector<std::string> validSectionAndPrority = {"TEXT", "DATA"};
    std::vector<uint32_t> sectorMemoryOffset(validSectionAndPrority.size());

    for (int instructionListIndex = 0; instructionListIndex < instructionLists.size(); instructionListIndex++)
    {
        for (int i = 0; i < instructionLists[instructionListIndex].size(); i++) 
        {
            int32_t memoryBankIndex = getElementIndex(instructionLists[instructionListIndex][i].dataSection, validSectionAndPrority);
            if (memoryBankIndex==-1)
            {
                errorStream << "Invalid section "<<instructionLists[instructionListIndex][i].dataSection<<"\n";
                return false;
            }
            
            if (instructionLists[instructionListIndex][i].instruction == "string")
            {
                instructionLists[instructionListIndex][i].memoryAddress = sectorMemoryOffset[memoryBankIndex];
                uint32_t dataBlockSize = 0;
                for (int oprandsIndex = 0; oprandsIndex < instructionLists[instructionListIndex][i].oprands.size(); oprandsIndex++)
                {
                    if (instructionLists[instructionListIndex][i].oprands[oprandsIndex].dataT == token::dataType::string)
                    {
                        dataBlockSize += instructionLists[instructionListIndex][i].oprands[oprandsIndex].stringData.size();
                    }
                    if (instructionLists[instructionListIndex][i].oprands[oprandsIndex].dataT == token::dataType::hex)
                    {
                        dataBlockSize++;
                    }
                    if (instructionLists[instructionListIndex][i].oprands[oprandsIndex].dataT == token::dataType::integer)
                    {
                        dataBlockSize++;
                    }
                }
                sectorMemoryOffset[memoryBankIndex] += dataBlockSize;
                continue;
            }

            if (instructionLists[instructionListIndex][i].instruction == "integer")
            {
                instructionLists[instructionListIndex][i].memoryAddress = sectorMemoryOffset[memoryBankIndex];
                sectorMemoryOffset[memoryBankIndex]+=0x4;
                continue;
            }

            if (instructionLists[instructionListIndex][i].instruction == "array")
            {
                instructionLists[instructionListIndex][i].memoryAddress = sectorMemoryOffset[memoryBankIndex];
                sectorMemoryOffset[memoryBankIndex]+=instructionLists[instructionListIndex][i].oprands[0].intData;
                continue;
            }

            instructionLists[instructionListIndex][i].memoryAddress = sectorMemoryOffset[memoryBankIndex];

            if (!instructionLists[instructionListIndex][i].isLabel)
            {
                sectorMemoryOffset[memoryBankIndex] += 0x10;
            }
        }
    
    }

    for (int instructionListIndex = 0; instructionListIndex < instructionLists.size(); instructionListIndex++) {
        for (int i=0;i < instructionLists[instructionListIndex].size(); i++)
        {
            int32_t memoryBankIndex = getElementIndex(instructionLists[instructionListIndex][i].dataSection, validSectionAndPrority);
            if (memoryBankIndex == 0)
            {
                continue;
            }
            instructionLists[instructionListIndex][i].memoryAddress += sectorMemoryOffset[memoryBankIndex-1] + startAddress;
        }
    }

    for (int i=0;i<validSectionAndPrority.size();i++)
    {
        returnMemorySize += sectorMemoryOffset[i];
    }
    return true;
}

bool syntax::checkValidInstructionToken(std::string instructionName, std::vector<token>& tokenList, unsigned int& instructionIndex, syntaxBlock& syntaxObj)
{
    const std::pair<std::string, unsigned int> INSTRUCTION_LIST[] ={{"mov",2}, 
                                                                    {"out",2}, 
                                                                    {"add", 2}, 
                                                                    {"and", 2}, 
                                                                    {"xor", 2}, 
                                                                    {"sub", 2}, 
                                                                    {"or", 2}, 
                                                                    {"readptr1", 2}, 
                                                                    {"jmpimm", 1},
                                                                    {"jmpif",1}, 
                                                                    {"cmp",2}, 
                                                                    {"halt",0}, 
                                                                    {"ret",0}, 
                                                                    {"call", 1}, 
                                                                    {"push", 1}, 
                                                                    {"pop", 1}, 
                                                                    {"inc", 1}, 
                                                                    {"dec", 1}, 
                                                                    {"pushreg", 0}, 
                                                                    {"popreg", 0}, 
                                                                    {"writeimm4", 2}, 
                                                                    {"writeimm2", 2}, 
                                                                    {"writeimm1", 2},
                                                                    {"div", 3},
                                                                    {"clhi", 0},
                                                                    {"sthi", 0}, 
                                                                    {"hiret", 0}};

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

        if (instructionName == "integer")
        {
            if (instructionIndex+1<tokenList.size())
            {
                if (tokenList[instructionIndex+1].dataT != token::dataType::integer && tokenList[instructionIndex+1].dataT != token::dataType::hex)
                {
                    return false;
                }
                syntaxObj.instruction = instructionName;
                syntaxObj.oprands.push_back(tokenList[instructionIndex+1]);
                return true;
            }
            return false;
        }

        if (instructionName == "array")
        {
            if (instructionIndex+1 < tokenList.size() && (tokenList[instructionIndex+1].dataT == token::dataType::integer || tokenList[instructionIndex+1].dataT == token::dataType::hex))
            {
                syntaxObj.instruction = instructionName;
                syntaxObj.oprands.push_back(tokenList[instructionIndex+1]);
                return true;
            }
            return false;
        }

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
    if (name == "writeimm4") { return 0x0b; }
    if (name == "writeimm2") { return 0x0c; }
    if (name == "writeimm1") { return 0x0d; }
    if (name == "div") { return 0x12; }
    if (name == "clhi") { return 0x31;}
    if (name == "sthi") { return 0x32;}
    if (name == "hiret") { return 0x33;}

    return 0;
}

bool syntax::AssembleFromSyntaxBlock(uint32_t currentInstructionIndex, std::vector<std::unordered_set<std::string>>& declaredLabels, std::unordered_set<std::string>& exportedLabelList, std::vector<std::unordered_set<std::string>>& externalLabelList, syntaxBlock& syntaxObj, std::unordered_map<std::string, uint32_t>& labelMemoryMap, std::array<uint32_t, 4>& assembledBytes, std::string& error)
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
            std::string fullLabelName;
            
            if (declaredLabels[currentInstructionIndex].find(syntaxObj.mainLabelBlock+syntaxObj.oprands[i].stringData) != declaredLabels[currentInstructionIndex].end())
            {
                found = true;
                fullLabelName = syntaxObj.mainLabelBlock+syntaxObj.oprands[i].stringData;
            } 
            else if (declaredLabels[currentInstructionIndex].find(syntaxObj.oprands[i].stringData) != declaredLabels[currentInstructionIndex].end())
            {
                found = true;
                fullLabelName = syntaxObj.oprands[i].stringData;
            } else if (exportedLabelList.find(syntaxObj.oprands[i].stringData) != exportedLabelList.end() && externalLabelList[currentInstructionIndex].find(syntaxObj.oprands[i].stringData) != externalLabelList[currentInstructionIndex].end())
            {
                found = true;
                fullLabelName = syntaxObj.oprands[i].stringData;
            }


            //for (int labelIndex = 0; labelIndex < labelList.size(); labelIndex++)
            //{
            //    if ((labelList[labelIndex].instruction == (syntaxObj.mainLabelBlock+syntaxObj.oprands[i].stringData)) || 
            //        (labelList[labelIndex].instruction == syntaxObj.oprands[i].stringData))
            //    {
            //        found = true;
            //        assembledBytes[i+1] = (labelList[labelIndex].memoryAddress);
            //        break;
            //    }
            //}


            if (!found)
            {
                std::stringstream returnError;
                returnError << "Unknown label, \"" + syntaxObj.oprands[i].stringData + "\", on line " << syntaxObj.lineNumber << " " + syntaxObj.filePath;
                error = returnError.str();
                return false;
            }
            else 
            {
                assembledBytes[i+1] = labelMemoryMap[fullLabelName];
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

void syntax::registerBuiltinLabels(std::vector<syntaxBlock> &labelList, std::vector<std::unordered_set<std::string>>& declaredLabels, std::unordered_map<std::string, uint32_t>& labelMemoryMap)
{
    std::vector<std::pair<std::string, uint32_t>> builtinLabels = {
        {"ra", 0},
        {"rb", 1},
        {"rc", 2},
        {"rd", 3},
        {"cmpreg", 4},
        {"sp", 5},
        {"bp", 6},
        {"rf", 7},
        {"hireg", 8},
        
        {"eq", 0},
        {"ne", 1},
        {"lt", 2},
        {"gt", 3},
        {"le", 4},
        {"ge", 5},
    };
    syntaxBlock registerLabel;
    registerLabel.isLabel = true;
    for (int i=0;i<builtinLabels.size();i++)
    {
        registerLabel.instruction = builtinLabels[i].first;
        registerLabel.memoryAddress = builtinLabels[i].second;
        labelList.push_back(registerLabel);
        labelMemoryMap[builtinLabels[i].first] = builtinLabels[i].second;
        for (int j = 0; j < declaredLabels.size(); j ++)
        {
            declaredLabels[j].insert(builtinLabels[i].first);
        }
    }
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
