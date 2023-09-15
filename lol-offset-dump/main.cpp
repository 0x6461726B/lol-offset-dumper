#include <iostream>
#include <fstream>
#include <chrono> //std::chrono
#include <ctime>  //ctime()
#include <string>
#include <filesystem>

#include "CSV/rapidcsv.h"


#include "Memory/Memory.h"

#define PATTERN_FILE "Patterns.txt"
#define DUMP_FILE "Offsets.h"



std::ofstream output;

enum InputFields
{
	FIELDS_TYPE,
	FIELDS_NAME,
	FIELDS_PATTERN,
	FIELDS_OFFSET,
	FIELDS_TYPESIZE
};

std::string ltrim(const std::string& s)
{
	size_t start = s.find_first_not_of(" ");
	return (start == std::string::npos) ? "" : s.substr(start);
}


bool ParseFileToStruct(std::vector<PatternStruct> &vector)
{

	auto pathAndFile = std::filesystem::current_path().append(PATTERN_FILE).string();
	rapidcsv::Document doc;
	try {
		doc = rapidcsv::Document(pathAndFile, rapidcsv::LabelParams::LabelParams(0, -1), rapidcsv::SeparatorParams(), rapidcsv::ConverterParams(true));
	}
	catch (std::ios::failure) {
;			printf("Patterns.txt not found!\nMake sure its here: %s\n", pathAndFile.c_str());  //theres no way to check if document exists,
			return 0;
	}
	  
	for (int i = -1; i < (int)doc.GetRowCount(); i++)
	{
		std::string type = doc.GetCell<std::string>(0, i);
		if (type != "OFFSET" && type != "ADDRESS" && type != "FUNCTION") {
			continue;
		}

		std::string name = ltrim(doc.GetCell<std::string>(1, i)); //trim space so its not ugly in console
		std::string pattern = doc.GetCell<std::string>(2, i);
		int64_t offset;
		int64_t typeSize;
		
		try {
			offset = doc.GetCell<__int64>(3, i);
		}
		catch (std::out_of_range) {
			offset = 0;
			}
		

		try {
			typeSize = doc.GetCell<__int64>(4, i);   //again... theres no way to check if a cell exists with this library.
		}
		catch (std::out_of_range) {
			typeSize = 4;
		}
		
		PatternStruct ps;
		ps.type_size = typeSize;
		ps.name = name;
		ps.pattern = pattern;
		ps.offset = offset;
		
	    if (type == "OFFSET") ps.type = InputType::Offset;
		else if (type == "ADDRESS") ps.type = InputType::Address;
		else if (type == "FUNCTION") ps.type = InputType::AddressFunction;
		

		vector.push_back(ps);
	}

	return true;
}

void CreateDumpFile()
{
	
	output.open(DUMP_FILE);

	
	auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	char str_time[MAXCHAR];
	ctime_s(str_time, MAXCHAR, &now);

	
    
	output << "#pragma once" << std::endl << std::endl
		<< "/*" << std::endl
		<< "Original dumper by @Ph4nton (https://www.unknowncheats.me/forum/members/780190.html)\n"
		<< "Updated by @Dark (https://www.unknowncheats.me/forum/members/1269962.html)" << std::endl
		<< str_time
		<< "*/" << std::endl << std::endl
		<< "#define BASEADDRESS GetModuleHandle(NULL)" << std::endl;
}

int main(int argc, const char* argv[])
{
	CMemory Memory;
	std::vector<PatternStruct> pattern_struct;

	if (argc < 2) {
		std::cout << "Correct usage: " << argv[0] << " \"League of Legends.exe\"" << std::endl;
		system("pause");
		return 0;
	}
	
	if (!ParseFileToStruct(pattern_struct) || !Memory.Initialize(argv[1])) {
		system("pause");
		return 0;
	}



	CreateDumpFile();

	std::optional<InputType> previousType;
	for (auto& obj : pattern_struct) {
		auto type = obj.type;
		if (!previousType.has_value() || type != *previousType) {
			if (type == InputType::Address) {
				output << "//---Addresses---" << std::endl;
				printf("%s ---Addresses--- %s\n", COLOR_BLUE, COLOR_RESET);
			}
			else if (type == InputType::AddressFunction) {
				output << "//---Functions---" << std::endl;
				printf("%s ---Functions--- %s\n", COLOR_BLUE, COLOR_RESET);
			}
			else if (type == InputType::Offset) {
				output << "//---Offsets---" << std::endl;
				printf("%s ---Offsets--- %s\n", COLOR_BLUE, COLOR_RESET);
			}
			previousType = type;
		}

		auto address = Memory.Pattern(obj);
		auto color = (address == 0x0) ? COLOR_RED : COLOR_GREEN;
		output << "#define " << obj.name << " 0x" << std::hex << std::uppercase << address << "\t//" << obj.pattern << std::endl;
		printf("%s%s 0x%X\r\n%s", color, obj.name.c_str(), address, COLOR_RESET);
	}

	
	output.close();
	system("pause");
	return 0;
}