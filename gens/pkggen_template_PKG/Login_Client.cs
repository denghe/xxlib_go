#pragma warning disable 0169, 0414
using TemplateLibrary;

namespace Login_Client
{
    [Desc("校验成功")]
    class AuthSuccess
    {
        [Desc("连接大厅后要发送的 token")]
        string token;
    }
}
