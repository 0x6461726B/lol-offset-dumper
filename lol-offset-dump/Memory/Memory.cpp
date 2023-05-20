#include "Memory.h"
#pragma warning(disable:4996)
#include <fstream> 

CMemory::CMemory()
{

}

CMemory::~CMemory()
{
	if (!hFileModule)
		CloseHandle(hFileModule);

	VirtualFree(rangeStart, NULL, MEM_RELEASE);
}
std::string ReadString(int64_t dwAddress, std::ifstream& file)
{
	file.seekg(dwAddress);

	std::string buffer;
	std::getline(file, buffer, '\0');

	return buffer;
}

bool GetNextByte(char** pszString, unsigned char& rByte, bool& isWhiteSpace)
{
	do
	{
		if (*(*pszString) == '?')
		{
			rByte = 0;
			isWhiteSpace = true;
			*(*pszString)++;

			if (*(*pszString) == '?')
				*(*pszString)++;

			return true;
		}
		else if (isxdigit(**pszString))
		{
			isWhiteSpace = false;
			rByte = (unsigned char)(strtoul(*pszString, pszString, 16) & 0xFF);
			return true;
		}
	} while (*(*pszString)++);

	return false;
}

//(pszString) IN IDA Signature, (pbArray) OUT Byte Array, (pszMask) OUT Mask
int Text2Hex(const char* pszString, unsigned char* pbArray, char* pszMask)
{
	int Count = 0;
	bool isWhiteSpace = false;

	if (pszMask)
		*pszMask = 0;

	if (GetNextByte(const_cast<char**>(&pszString), pbArray[Count], isWhiteSpace))
	{
		do
		{
			Count++;

			if (pszMask)
				strncat(pszMask, (isWhiteSpace) ? "?" : "x", sizeof(pszMask));

		} while (GetNextByte(const_cast<char**>(&pszString), pbArray[Count], isWhiteSpace));
	}

	return Count;
}

std::pair<std::vector<BYTE>, std::string> IDAToCode(std::string in_ida_sig)
{
	// variables
	char mask[MAXCHAR];
	std::vector<BYTE> vec_of_byte;
	unsigned char byte_array[MAXCHAR];

	// get ByteArray and Mask
	int count = Text2Hex(in_ida_sig.c_str(), byte_array, mask);

	// create array of bytes
	for (int i = 0; i < count; i++)
		vec_of_byte.push_back(byte_array[i]);

	return std::pair<std::vector<BYTE>, std::string>({ vec_of_byte, mask });
}

BOOL DataCompare(BYTE* pData, BYTE* bMask, char* szMask)
{
	for (; *szMask; ++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask)
			return FALSE;

	return (*szMask == NULL);
}


bool CMemory::Initialize(const char* path_to_exe)
{
	//std::ifstream file(path_to_exe, std::ios::binary);

	//printf("dog test %s", ReadString((int64_t)(0x51DFC10 + ImageBase), file).c_str());

	HANDLE hFileMapping;
	LPVOID lpFileBase;
	PIMAGE_DOS_HEADER dosHeader;
	PIMAGE_NT_HEADERS peHeader;
	PIMAGE_SECTION_HEADER sectionHeader;

	

	hFileModule = CreateFile(path_to_exe, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFileModule == INVALID_HANDLE_VALUE) {
		std::cout << "Invalid HANDLE of file, Error code: " << GetLastError() << std::endl;
		return false;
	}
	
	hFileMapping = CreateFileMapping(hFileModule, NULL, PAGE_READONLY, 0, 0, NULL);

	if (hFileMapping == 0) {
		std::cout << "\n CreateFileMapping failed \n";
		CloseHandle(hFileModule);
		return false;
	}

	lpFileBase = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);

	if (lpFileBase == 0) {
		std::cout << "\n MapViewOfFile failed \n";
		CloseHandle(hFileMapping);
		CloseHandle(hFileModule);
		return false;
	}
	
	

	dosHeader = (PIMAGE_DOS_HEADER)lpFileBase;

	
	if (dosHeader->e_magic == IMAGE_DOS_SIGNATURE)
	{
		peHeader = (PIMAGE_NT_HEADERS)((uint8_t*)dosHeader + dosHeader->e_lfanew);

		if (peHeader->Signature != IMAGE_NT_SIGNATURE)
			//return false;

		ImageBase = static_cast<int64_t>(peHeader->OptionalHeader.ImageBase);
	}

	/*std::ifstream file(path_to_exe, std::ios::binary);

	printf("Testing %s", ReadString((int64_t)(0x051DFC10 + ImageBase), file).c_str());*/

	dwFileSize = GetFileSize(hFileModule, nullptr);

	if (dwFileSize == INVALID_FILE_SIZE) {
		std::cout << "Invalid file size, Error code: " << GetLastError() << std::endl;
		CloseHandle(hFileModule);
		return false;
	}

	rangeStart = (PBYTE)VirtualAlloc(nullptr, GetFileSize(hFileModule, nullptr), MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if (rangeStart == nullptr) {
		std::cout << "Failed to alloc virtual address space, Error code: " << GetLastError() << std::endl;
		CloseHandle(hFileModule);
		return false;
	}

	DWORD dwBytesRead = 0;
	if (!ReadFile(hFileModule, rangeStart, dwFileSize, &dwBytesRead, nullptr)) {
		std::cout << "Failed to read file, Error code: " << GetLastError() << std::endl;
		CloseHandle(hFileModule);
		VirtualFree(rangeStart, NULL, MEM_RELEASE);
		return false;
	}


	auto ret = IDAToCode("48 8D 15 ?? ?? ?? ?? 48 2B D1 66 0F 1F 44 00 00");

	auto versionAddress = findAddress((int64_t)rangeStart, dwFileSize, ret.first.data(), ret.second.data(), InputType::Address, 3);
	std::ifstream file(path_to_exe, std::ios::binary);

	gameVersion =  ReadString((int64_t)(versionAddress + ImageBase), file);
	
	file.close();

	CloseHandle(hFileModule);
	
	return (dwBytesRead > 0);
}




int64_t CMemory::findAddress(int64_t dwAddress, int64_t dwLen, BYTE* bMask, char* szMask, InputType dType, int64_t offset)
{
	

	for (int64_t i = 0; i < dwLen; i++)
	{
		if (DataCompare((BYTE*)(dwAddress + i), bMask, szMask))
		{

			if (bMask[0] == 0xE8)
				return (INT)(dwAddress + i + 1) + *(INT*)(dwAddress + i + 1) + sizeof(INT) - (INT)rangeStart;

			switch (dType)
			{
			case InputType::Address:
				return (INT)(dwAddress + i + offset) + *(INT*)(dwAddress + i + offset) + sizeof(INT) - (INT)rangeStart;
			case InputType::AddressFunction:
				return (INT)(dwAddress + i + offset) - (INT)rangeStart;
			case InputType::Offset:
				return dwAddress + i + offset;
			}
		}
	}
	return 0;
}


constexpr int64_t limit = 0xFFFFFFF;
constexpr int64_t limit2 = 0xFFFFFF;
constexpr int64_t limit3 = 0xFF;

int64_t CMemory::Pattern(PatternStruct Struct)
{
	auto ret = IDAToCode(Struct.pattern);
	
	

	auto address = findAddress((int64_t)rangeStart, dwFileSize, ret.first.data(), ret.second.data(), Struct.type, Struct.offset);
	
	if (!address)
		return 0;

	if (Struct.type != InputType::Offset) {
		while (address > limit || address < -1) {
			//printf("%sinvalid address for %s %i %s\n", COLOR_RED, Struct.name, Struct.offset, COLOR_RESET);
			address = findAddress((int64_t)rangeStart, dwFileSize, ret.first.data(), ret.second.data(), Struct.type, Struct.offset++); //logic: increments offset until it gets an address thats 7 digit, if its higher or lower = incorrect, 
		}
	}
	else {

     	if (*(int32_t*)address < limit2 && *(int32_t*)address > -1)
			address = *(int32_t*)address;

		else if (*(int8_t*)address < limit3 && *(int8_t*)address > -1)
			address = *(int8_t*)address;
		else
			address = 0;

	}
	return address;
}



const char* CMemory::getGameVersion()
{
	return gameVersion.c_str();
}