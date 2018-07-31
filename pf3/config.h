#pragma once
#include "tixmlutil.h"

namespace Config
{
    class Directories
    {
        std::map<std::string, std::string> directories_;
    public:
        void AddDirectory(std::string label, std::string dir)
        {
            directories_[label] = dir;
        }
        std::string GetDirectory(std::string label)
        {
            std::string result;
            auto p = directories_.find(label);
            if (p != directories_.end())
            {
                result = p->second;
            }
            else
            {
                result = "";
            }
            return result;
        }
    };

    extern Directories directories;
    
    class Config
    {
        std::set<std::string> argset_;
        std::vector<std::string> arglist_;

        // private default and copy constructors:
        Config() = default;
        Config(const Config&) = default;
    public:
        //----------------------------------------------------------------------------
        //
        // Name: GetInstance
        //
        // Description: Meyers Singleton GetInstance function returning the one
        //              and only instance of this class for the entire application.
        //
        //----------------------------------------------------------------------------
        static Config& GetInstance()
        {
            static Config instance_;
            return instance_;
        }

        //----------------------------------------------------------------------------
        //
        // Name: StoreArgv
        //
        // Description: Store the command line parameters argv and argc, both as a 
        //              std::vector and a std::set. The set provides easy lookup of
        //              options.
        //
        //----------------------------------------------------------------------------
        void StoreArgv(int argc, char*argv[])
        {
            for (auto i = 0; i < argc; i++)
            {
                arglist_.push_back(argv[i]);
                argset_.insert(argv[i]);
            }
        }

        //----------------------------------------------------------------------------
        //
        // Name: GetCommandLineParam
        //
        // Description: return a command line param given its index. passing 0 would
        //              return the program name.
        //
        //----------------------------------------------------------------------------
        std::string GetCommandLineParam(size_t index)
        {
            return arglist_.at(index);
        }

        //----------------------------------------------------------------------------
        //
        // Name: CheckArg
        //
        // Description: return true if the string passed is in the command line set
        //              This is useful for checking option such as
        //              if (CheckArg("-filter")) {...}
        //
        //----------------------------------------------------------------------------
        bool CheckArg(std::string argname)
        {
            return argset_.find(argname) != argset_.end();
        }

        //----------------------------------------------------------------------------
        //
        // Name: Read
        //
        // Description: Read the main configuration file (pfconfig.xml). Store the
        //              folder mappings in the directories structure
        //
        //----------------------------------------------------------------------------
        void Read(std::string filename)
        {
            TiXmlDocument doc(filename.c_str());
            bool loadOK = doc.LoadFile();
            if (loadOK)
            {
                auto configData = doc.FirstChildElement("config");
                if (configData == nullptr)
                {
                    throw QException("Cannot find config XML element in config file.");
                }
                int count = 0;

                for (const TiXmlElement* pDirectories = configData->FirstChildElement("directories");
                pDirectories; pDirectories = pDirectories->NextSiblingElement("directories"))
                {
                    count++;
                    if (count > 1)
                    {
                        throw QException("Error reading configuration file " + filename + " more than one Directories element");
                    }
                    for (const TiXmlElement* pDir = pDirectories->FirstChildElement("dir");
                    pDir; pDir = pDir->NextSiblingElement("dir"))
                    {
                        auto label = TixmlUtil::GetStringAttribute(pDir, "name");
                        auto dir = TixmlUtil::GetStringAttribute(pDir, "value");
                        directories.AddDirectory(label, dir);
                    }
                }

                for (const TiXmlElement* pNetwork = configData->FirstChildElement("network");
                pNetwork; pNetwork = pNetwork->NextSiblingElement("network"))
                {
                    count++;
                    if (count > 1)
                    {
                        throw QException("Error reading configuration file " + filename + " more than one Directories element");
                    }

                    auto pEndpoint = pNetwork->FirstChildElement("endpoint");
                    auto port = TixmlUtil::GetIntAttribute(pNetwork, "port");
                    auto address = TixmlUtil::GetStringAttribute(pNetwork, "address");

                }

            }
        }
    };

}
