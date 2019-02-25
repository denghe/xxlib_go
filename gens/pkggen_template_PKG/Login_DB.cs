#pragma warning disable 0169, 0414
using TemplateLibrary;

namespace Login_DB
{
    [Desc("校验. 成功返回 DB_Login.Auth_Success 内含 userId. 失败返回 Generic.Error")]
    class Auth
    {
        string username;
        string password;
    }
}
