#pragma once

namespace MWD {
    inline std::string GetCurrentDir()
    {
        static const int MaxPath = 2048;
        std::vector<char> cdir(MaxPath);
        GetCurrentDirectory(cdir.size(), cdir.data());
        std::string result(cdir.begin(), cdir.end());
        return result;
    }

    inline void SetCurrentDir(std::string path)
    {
        SetCurrentDirectory(path.c_str());
    }
}

class ManageWorkingDir
{
    std::string initialDir_;
    std::stack<std::string> dirStack_;
    ManageWorkingDir()
    {
        initialDir_ = MWD::GetCurrentDir();
    }
public:
    static ManageWorkingDir& GetInstance()
    {
        static ManageWorkingDir instance;
        return instance;
    }

    void PushD(std::string dir)
    {
        dirStack_.push(MWD::GetCurrentDir());
        MWD::SetCurrentDir(dir);
    }

    void PopD()
    {
        MWD::SetCurrentDir(dirStack_.top());
        dirStack_.pop();
    }

    void BackToInitial()
    {
        MWD::SetCurrentDir(initialDir_);
    }
    virtual ~ManageWorkingDir(){}
};
