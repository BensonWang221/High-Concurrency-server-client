enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR
};

struct DataHeader
{
	DataHeader() {}
	DataHeader(short length, short c) : dataLength(length), cmd(c) {}

	short dataLength;
	short cmd;
};

struct Login : public DataHeader
{
	Login() : DataHeader(sizeof(Login), CMD_LOGIN) {}

	char userName[32];
	char password[32];
};

struct LoginResult : public DataHeader
{
	LoginResult() : DataHeader(sizeof(LoginResult), CMD_LOGIN_RESULT) {}
	int result;
};

struct Logout : public DataHeader
{
	Logout() : DataHeader(sizeof(Logout), CMD_LOGOUT) {}
	char userName[32];
};

struct LogoutResult : public DataHeader
{
	LogoutResult() : DataHeader(sizeof(LogoutResult), CMD_LOGOUT_RESULT) {}
	int result;
};

struct NewUserJoin : public DataHeader
{
	NewUserJoin() : DataHeader(sizeof(NewUserJoin), CMD_NEW_USER_JOIN) {}
	int sock;
};
