#pragma once


#include <string>
#include <Windows.h>
#include <sstream>
#include <iomanip>
void debugPrint(std::string str)
{
	OutputDebugStringA(str.c_str());
}

void debugPrintHex(size_t offset)
{
	std::stringstream ss;
	ss << std::hex << std::setw(8) << std::setfill('0') << offset << "\n";
	OutputDebugStringA(ss.str().c_str());
}