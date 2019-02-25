#pragma warning disable 0169, 0414
using TemplateLibrary;

namespace Client_Login
{
    [Desc("失败返回 Generic.Error. 成功返回 Login_Client.EnterLobby 或 EnterGame1")]
    class Auth
    {
        string username;
        string password;
    }
}
