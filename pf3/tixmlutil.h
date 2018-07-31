#pragma once
#define TIXML_USE_STL
#include <tinyxml.h>

namespace TixmlUtil
{
    std::string GetStringAttribute(const TiXmlElement* elem, std::string attributename);
    std::string GetStringAttributeLen(const TiXmlElement* elem, std::string attributename, int len);
    int GetIntAttribute(const TiXmlElement* elem, std::string attributename);
    int GetZeroOneAttribute(const TiXmlElement* elem, std::string attributename);
    int GetOptIntAttribute(bool& got, const TiXmlElement* elem, std::string attributename);
};
