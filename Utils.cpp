#include "Utils.h"

#include <algorithm>

std::string GetLowerString(const std::string& sString)
{
	std::string sReturn(sString);
	LowerString(sReturn);
	return sReturn;
}

void LowerString(std::string& sString)
{
	std::transform(sString.begin(), sString.end(), sString.begin(), ::tolower);
}
