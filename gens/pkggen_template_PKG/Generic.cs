#pragma warning disable 0169, 0414
using TemplateLibrary;

// 一些通用结构

namespace Generic
{
    class Success
    {
    }

    class Error
    {
        int number;
        string text;
    }

    class ServerInfo
    {
        string name;
    }

    class UserInfo
    {
        long id;
        string name;
    }
}
