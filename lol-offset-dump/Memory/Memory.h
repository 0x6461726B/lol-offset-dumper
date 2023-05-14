#pragma once
#include <Windows.h>
#include <malloc.h>
#include <string>
#include <iostream>
#include <vector>
#include <ctype.h>

//enum InputType : INT
//{
//	TYPE_INVALID,
//	TYPE_OFFSET,
//	TYPE_ADDRESS,
//	TYPE_ADDRESS_FUNCTION
//};
enum class InputType
{
	Invalid,
	Address,
	AddressFunction,
	Offset
};


struct PatternStruct
{
	std::string name, pattern;
	int64_t offset, type_size;
	InputType type;
};

class CMemory
{
public:
	CMemory();
	~CMemory();

	bool Initialize(const char* path_to_exe);

	int64_t Pattern(PatternStruct Struct);
	int64_t findAddress(int64_t dwAddress, int64_t dwLen, BYTE* bMask, char* szMask, InputType dType, int64_t offset = 0);

private:
	HANDLE hFileModule;
	int64_t dwFileSize;
	PBYTE rangeStart;
	int64_t ImageBase;
};

