#include "stdafx.h"
#include "tixmlutil.h"

namespace TixmlUtil {

    // returns a string from an attribute which MUST EXIST. This function throws an exception if the
    // attribute specfied does not exist
    std::string GetStringAttribute(const TiXmlElement* elem, std::string attributename)
    {
        const char* attribute = elem->Attribute(attributename.c_str());
        if (attribute == NULL)
        {
            std::string elemname = elem->ValueStr();
            throw QException(std::to_string(elem->Row()) + attributename + " attribute expected in " + elemname);
        }
        return std::string(attribute);
    }

    // returns a string from an attribute which MUST EXIST. This function throws an exception if the
    // attribute specfied does not exist
    // This function also checks the length of the string - which must be equal to len - and throws an 
    // exception if not.
    std::string GetStringAttributeLen(const TiXmlElement* elem, std::string attributename, int len)
    {
        const char* attribute = elem->Attribute(attributename.c_str());
        if (attribute == NULL)
        {
            std::string elemname = elem->ValueStr();
            throw QException(std::to_string(elem->Row()) + "  "+ attributename + " attribute expected in " + elemname);
        }
        std::string result(attribute);
        if (result.length() != len)
        {
            throw QException(std::to_string(elem->Row()) + "  " + "value for attribute " + attributename + " must be " + std::to_string(len) + " characters.");
        }
        return std::string(attribute);
    }

    // returns an integer from an attribute which MUST EXIST. This function throws an exception if the
    // attribute specfied does not exist
    int GetIntAttribute(const TiXmlElement* elem, std::string attributename)
    {
        const char* attribute = elem->Attribute(attributename.c_str());
        if (attribute == NULL)
        {
            std::string elemname = elem->ValueStr();
            throw QException(std::to_string(elem->Row()) + "  " + attributename + " attribute expected in " + elemname);
        }
        std::string satt(attribute);
        for (size_t i = 0; i < satt.size(); ++i)
        {
            if (!isdigit(satt[i]))
            {
                throw QException(std::to_string(elem->Row()) + "  " + "invalid character '" + satt[i] + std::string("' in number in attribute ") + attributename);
            }
        }
        return std::stoi(satt);
    }

    // an integer attribute that is optional - if it does not exist then got will be set to false and 
    // the return value will be undefined:
    int GetOptIntAttribute(bool& got, const TiXmlElement* elem, std::string attributename)
    {
        int result = 0;
        const char* attribute = elem->Attribute(attributename.c_str());
        if (attribute == NULL)
        {
            got = false;
        }
        else
        {
            std::string satt(attribute);
            for (size_t i = 0; i < satt.size(); ++i)
            {
                if (!isdigit(satt[i]))
                {
                    throw QException(std::to_string(elem->Row()) + "  " + "invalid character '" + satt[i] + std::string("' in number in attribute ") + attributename);
                }
            }
            got = true;
            result = std::stoi(satt);
        }
        return result;
    }

    int GetZeroOneAttribute(const TiXmlElement* elem, std::string attributename)
    {
        const char* attribute = elem->Attribute(attributename.c_str());
        if (attribute == NULL)
        {
            std::string elemname = elem->ValueStr();
            throw QException(std::to_string(elem->Row()) + "  " + attributename + " attribute expected in " + elemname);
        }
        std::string satt(attribute);
        if (satt.length() != 1 || (satt[0] != '0' && satt[0] != '1'))
        {
            throw QException(std::to_string(elem->Row()) + "  " + "invalid number in attribute " + attributename + " - must be zero or one.");
        }
        return satt[0] - '0';
    }


}