#ifndef _MESSAGE_HEADER_INCLUDED
#define _MESSAGE_HEADER_INCLUDED
enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_C2S_HEART,
	CMD_S2C_HEART,
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
	char data[32];
};

struct LoginResult : public DataHeader
{
	LoginResult() : DataHeader(sizeof(LoginResult), CMD_LOGIN_RESULT) {}
	int result;
	char data[92];
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

struct HeartCheckC2S : public DataHeader
{
	HeartCheckC2S() : DataHeader(sizeof(HeartCheckC2S), CMD_C2S_HEART) {}
};

struct HeartCheckS2C : public DataHeader
{
	HeartCheckS2C() : DataHeader(sizeof(HeartCheckS2C), CMD_S2C_HEART) {}
};

#endif
