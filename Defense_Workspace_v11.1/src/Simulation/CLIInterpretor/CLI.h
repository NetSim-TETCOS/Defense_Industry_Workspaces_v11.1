#pragma once

#include "CLIInterface.h"

#pragma comment(lib,"NetworkStack.lib")
#ifndef NETSIM_ID
#define NETSIM_ID UINT
#endif

#define DEFAULT_PROMPT		"NetSim"
//Default command list
#define CMD_CHANGEPROMPT	"CHANGEPROMPT"
#define CMD_STOP			"STOP"
#define CMD_CONTINUE		"CONTINUE"
#define CMD_PAUSE			"PAUSE"
#define CMD_PAUSEAT			"PAUSEAT"
#define CMD_EXIT			"EXIT"
#define CMD_ACLCONFIG		"ACLCONFIG"

void free_commandArray(ptrCOMMANDARRAY c);

typedef enum enum_clientType
{
	CLIENTTYPE_NONE,
	CLIENTTYPE_FILE,
	CLIENTTYPE_SOCKET,
	CLIENTTYPE_STRING,
}CLIENTTYPE;

typedef enum
{
	HT_NONE,
	HT_DEVICE,
	HT_CMD,
}HT;
typedef struct stru_headingInfo
{
	char* name;
	HT headingType;
}HEADINGINFO,*ptrHEADINGINFO;

typedef struct stru_fileClientInfo
{
	char* inputFile;
	FILE* fpInputFile;
	char* outputFile;
	FILE* fpOutputFile;
}FILECLIENTINFO, *ptrFILECLIENTINFO;

typedef struct stru_sockClientInfo
{
	SOCKET clientSocket;

	//Receive Thread info
	HANDLE clientRecvHandle;
	DWORD clientRecvId;

	//Sync variable
	bool iswait;
	bool dontUseMe;
}SOCKCLIENTINFO, *ptrSOCKCLIENTINFO;

typedef struct stru_stringClientInfo
{
	char* buffer;
	UINT len;
}STRINGCLIENTINFO, *ptrSTRINGCLIENTINFO;

typedef struct stru_clientInfo
{
	CLIENTTYPE clientType;
	union client
	{
		SOCKCLIENTINFO sockClient;
		FILECLIENTINFO fileClient;
		STRINGCLIENTINFO stringClient;
	}CLIENT;

	NETSIM_ID deviceId;
	char* deviceName;

	char* promptString;

	bool isMultResp;
	bool(*multResp)(void*, char*, int, bool);
	void* multRespArg;
	struct stru_clientInfo* next;
}CLIENTINFO, *ptrCLIENTINFO;

typedef struct str_cli_handle
{
	ptrCOMMANDARRAY cmdArray;
	ptrCLIENTINFO clientInfo;
}*ptrCLIHANDLE;
#define CLIHANDLE ptrCLIHANDLE

void* add_new_socket_client(SOCKET s, char* name);

void process_command(ptrCLIENTINFO clientInfo, char* command, int len);
void send_to_socket(ptrCLIENTINFO info, char* buf, int len);
void write_to_file(ptrCLIENTINFO info, char* msg, int len);

//
void send_message(ptrCLIENTINFO info, char* msg, ...);

//Simulation process
void cli_stop_simulation(ptrCLIENTINFO info);
void cli_pause_simulation(ptrCLIENTINFO info);
void cli_continue_simulation(ptrCLIENTINFO info);
void cli_pause_simulation_at(ptrCLIENTINFO info, double time);

//Command
bool validate_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command);
void cli_clear_prompt(ptrCLIENTINFO info);
void execute_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, NETSIM_ID d);
void pass_to_SDNModule(ptrCLIENTINFO info, ptrCOMMANDARRAY command);

// Route Command
bool validate_route_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index);
void execute_route_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index, NETSIM_ID d);

//ACL Command
bool validate_acl_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index);
void execute_acl_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index, NETSIM_ID d);
void execute_aclconfig_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index, NETSIM_ID d);
bool validate_aclconfig_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index);
void execute_prompt_aclconfig_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index, NETSIM_ID d);

//Ping command
bool validate_ping_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index);
void execute_ping_command(ptrCLIENTINFO info, ptrCOMMANDARRAY command, int index, NETSIM_ID d);

//CLI Handle
CLIHANDLE FORM_CLI_HANDLE(ptrCOMMANDARRAY cmd,
						  ptrCLIENTINFO info);

int fn_NetSim_CLI_HandleTimerEvent();

ptrCOMMANDARRAY get_commandArray(char *text);
bool isCommandAsDeviceName(char* name);

//File Client
void set_fileClientInfo(ptrCLIENTINFO info);
ptrCLIENTINFO get_fileClientInfo();
void read_file_and_execute(FILE* fp);

//String client
void add_to_string(ptrCLIENTINFO info, char* sendMsg, int len);