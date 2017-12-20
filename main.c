#include <windows.h>
#include <stdio.h>
#include <wininet.h>

#define CONSOLE_TITLE L"EasyMine Miner Monitor v0"
#define DEFAULT_HASH L"0MH/s"
#define DEFAULT_OTHER L"0.0 VTC"
#define MINER_WEB_AGENT L"NtZeroFear Manager"
#define JSON_PARSER_OFFSET 2

typedef struct _STRING {
	USHORT Length;
	USHORT MaximumLength;
	PCHAR  Buffer;
} ANSI_STRING, *PANSI_STRING;

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef VOID(NTAPI *RTLINITUNICODESTRING) (PUNICODE_STRING, PWCHAR);
typedef VOID(NTAPI *RTLINITANSISTRING)(PANSI_STRING, PCHAR);
typedef VOID(NTAPI *RTLANSISTRINGTOUNICODESTRING)(PUNICODE_STRING, PANSI_STRING, BOOLEAN);
typedef VOID(NTAPI *RTLFREEUNICODESTRING)(PUNICODE_STRING);

typedef struct __AGENT_INFO{
	WCHAR Wallet[MAX_PATH];
	WCHAR Balance[MAX_PATH];
	WCHAR HashRate[MAX_PATH];
	WCHAR EstimatedRewardCurrent[MAX_PATH];
	WCHAR EstimatedRewardDaily[MAX_PATH];
	WCHAR QueryAddress[1024];
	HANDLE hFile;
	WCHAR FilePath[MAX_PATH];
	COORD Coord;
	DWORD dwX;
	DWORD dwY;
}AGENT_INFO, *PAGENT_INFO;

HANDLE hConsole;
LPWSTR JsonQuery = L"https://vertcoin.easymine.online/json/miner.php?address=";

RTLANSISTRINGTOUNICODESTRING RtlAnsiStringToUnicodeString;
RTLINITANSISTRING RtlInitAnsiString;
RTLINITUNICODESTRING RtlInitUnicodeString;
RTLFREEUNICODESTRING RtlFreeUnicodeString;

VOID SetConsoleHeaderUi(VOID);
VOID SeparatorUi(VOID);
VOID ClearUi(VOID);
VOID UpdateAgentInfo(PAGENT_INFO Agent);
VOID SetStatusCoord(LPWSTR lpString, DWORD dwY);

BOOL GetEstimates(PAGENT_INFO Info, LPVOID lpBuffer);
BOOL GetBalance(PAGENT_INFO Info, LPVOID lpBuffer);
BOOL GetHashRate(PAGENT_INFO Info, LPVOID lpBuffer);

int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
	DWORD dwError = ERROR_SUCCESS, dwX = ERROR_SUCCESS, dwY = ERROR_SUCCESS;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	HINTERNET Connection = NULL;
	HINTERNET Address = NULL;
	LPWSTR *szAgent;
	INT nAgents;
	AGENT_INFO Info[20] = { 0 };
	LPVOID lpHeap;

	HMODULE hNtdll = LoadLibrary(L"ntdll.dll");
	if (hNtdll == NULL) 
		goto FAILURE;

	RtlAnsiStringToUnicodeString = (RTLANSISTRINGTOUNICODESTRING)GetProcAddress(hNtdll, "RtlAnsiStringToUnicodeString");
	RtlInitAnsiString = (RTLINITANSISTRING)GetProcAddress(hNtdll, "RtlInitAnsiString");
	RtlInitUnicodeString = (RTLINITUNICODESTRING)GetProcAddress(hNtdll, "RtlInitUnicodeString");
	RtlFreeUnicodeString = (RTLFREEUNICODESTRING)GetProcAddress(hNtdll, "RtlFreeUnicodeString");

	if (!RtlAnsiStringToUnicodeString || !RtlInitAnsiString || !RtlInitUnicodeString || !RtlFreeUnicodeString)
		goto FAILURE;
	
	szAgent = CommandLineToArgvW(GetCommandLineW(), &nAgents);
	if (szAgent == NULL) goto FAILURE;

	lpHeap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (MAX_PATH * sizeof(WCHAR)));
	if (lpHeap == NULL) goto FAILURE;

	if (GetEnvironmentVariable(L"LOCALAPPDATA", lpHeap, (MAX_PATH * sizeof(WCHAR))) == 0) goto FAILURE;
	
	if (!SetConsoleTitle(CONSOLE_TITLE)) goto FAILURE;

	SetConsoleHeaderUi();
	
	for(dwError = 1, dwX = 0; dwError < (DWORD)nAgents; dwError++, dwX++)
	{
		wcsncpy_s(Info[dwX].FilePath, MAX_PATH, lpHeap, wcslen(lpHeap) * sizeof(WCHAR)); // \\filename
		swprintf(Info[dwX].FilePath, MAX_PATH, L"%ws\\Wallet%ld", Info[dwX].FilePath, dwX);

		Info[dwX].hFile = CreateFileW(Info[dwX].FilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, NULL);
		if (Info[dwX].hFile == INVALID_HANDLE_VALUE)
			goto FAILURE;

		wcsncpy_s(Info[dwX].Wallet, MAX_PATH, szAgent[dwError], wcslen(szAgent[dwError]) * sizeof(WCHAR));
		wcsncpy_s(Info[dwX].HashRate, MAX_PATH, DEFAULT_HASH, wcslen(DEFAULT_HASH) * sizeof(WCHAR));
		wcsncpy_s(Info[dwX].EstimatedRewardCurrent, MAX_PATH, DEFAULT_OTHER, wcslen(DEFAULT_OTHER) * sizeof(WCHAR));
		wcsncpy_s(Info[dwX].EstimatedRewardDaily, MAX_PATH, DEFAULT_OTHER, wcslen(DEFAULT_OTHER) * sizeof(WCHAR));
		wcsncpy_s(Info[dwX].Balance, MAX_PATH, DEFAULT_OTHER, wcslen(DEFAULT_OTHER) * sizeof(WCHAR));
		swprintf(Info[dwX].QueryAddress, 1024, L"%ws%ws", JsonQuery, Info[dwX].Wallet);

		Info[dwX].dwY = dwError + 9;
		Info[dwX].dwX = 0;

		UpdateAgentInfo(&Info[dwX]);
	}
	SeparatorUi();
	dwY = dwError + 10;

	LocalFree(szAgent);
	HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, lpHeap);

	Connection = InternetOpen(MINER_WEB_AGENT, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (Connection == NULL)
		goto FAILURE;

	while (1)
	{
		WCHAR DataReceived[1024];
		DWORD dwReceived = 0, dwWritten = 0;
		CHAR Block[4096] = { 0 };

		for (dwX = 0; Info[dwX].dwY != 0; dwX++) 
		{ 
			CHAR Clone[4096] = { 0 };

			Address = InternetOpenUrl(Connection, Info[dwX].QueryAddress, NULL, 0, INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_KEEP_CONNECTION, 0);
			if (Address == NULL)
				goto FAILURE;
			else
				RtlZeroMemory(DataReceived, 1024);

			while (InternetReadFile(Address, DataReceived, 1024, &dwReceived) == TRUE && dwReceived > 0)
			{
				if (!WriteFile(Info[dwX].hFile, DataReceived, dwReceived, &dwWritten, NULL)) goto FAILURE;
			}

			InternetCloseHandle(Address);

			dwError = GetFileSize(Info[dwX].hFile, NULL);
			if (dwError == 0) goto FAILURE;

			if (SetFilePointer(Info[dwX].hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) goto FAILURE;

			if (!ReadFile(Info[dwX].hFile, Block, dwError, &dwReceived, NULL)) goto FAILURE;

			strncpy(Clone, Block, strlen(Block));
			if (!GetBalance(&Info[dwX], Clone))
				goto FAILURE;
			else
				RtlZeroMemory(Clone, 4096);

			strncpy(Clone, Block, strlen(Block));
			if (!GetEstimates(&Info[dwX], Clone))
				goto FAILURE;
			else
				RtlZeroMemory(Clone, 4096);

			strncpy(Clone, Block, strlen(Block));
			if (!GetHashRate(&Info[dwX], Clone))
				goto FAILURE;
			else
				RtlZeroMemory(Clone, 4096);

			if (SetFilePointer(Info[dwX].hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) goto FAILURE;

			if (!SetEndOfFile(Info[dwX].hFile)) goto FAILURE;

		}
		
		for (dwReceived = 0; dwReceived < 5; dwReceived++)
		{
			RtlZeroMemory(DataReceived, 1024);
			swprintf(DataReceived, 1024, L"Updating Data in %ld minutes..", (5 - dwReceived));
			SetStatusCoord(DataReceived, dwY);
			Sleep(60000);
		}

		SetStatusCoord(L"Updating data...", dwY);


	}
	
	for (dwError = 0; Info[dwError].dwY != 0; dwError++){ CloseHandle(Info[dwError].hFile); }

	if (Address)
		InternetCloseHandle(Address);

	if (Connection)
		InternetCloseHandle(Connection);

	FreeLibrary(hNtdll);

	return ERROR_SUCCESS;
	
FAILURE:

	dwError = GetLastError();

	for (dwX = 0; Info[dwX].dwY != 0; dwX++){ CloseHandle(Info[dwX].hFile); }

	if(szAgent)
		LocalFree(szAgent);

	if(lpHeap)
		HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, lpHeap);
	
	if(Address)
		InternetCloseHandle(Address);
		
	if(Connection)
		InternetCloseHandle(Connection);

	FreeLibrary(hNtdll);
	
	return dwError;
}

VOID SetConsoleHeaderUi(VOID)
{
	
	printf("%35s**NtZeroFear EasyPool.online Miner Mon**\r\n\r\n", " ");
	//ClearUi();
	printf("Version: 0\r\nUtilizing: Win32API\r\nValiding: EasyPool.online\r\nCurrency: VertCoin\r\nBuilt with: Visual Studio 2017\r\n");
	SeparatorUi();
	printf("%20s %25s %28s %23s %13s\r\n", "Wallet", "HashRate", "Estimated_Current_Reward", "Estimated_Daily_Reward", "Balance");
	
}

VOID SeparatorUi(VOID)
{
	DWORD dwX = 0;
	for(;dwX < 120; dwX++)
		printf("-");
}

VOID ClearUi(VOID)
{
	DWORD dwX = 0;
	for(;dwX < 120; dwX++)
		printf(" ");
}

VOID UpdateAgentInfo(PAGENT_INFO Agent)
{
	if(Agent->Coord.Y == 0)
	{
		Agent->Coord.Y = (SHORT)Agent->dwY;
		Agent->Coord.X = 0;
	}
	
	SetConsoleCursorPosition(hConsole, Agent->Coord);
	
	printf("%ws %15ws %20ws %22ws %20ws\r\n", Agent->Wallet, Agent->HashRate, Agent->EstimatedRewardCurrent, Agent->EstimatedRewardDaily, Agent->Balance);
}

BOOL GetEstimates(PAGENT_INFO Info, LPVOID lpBuffer)
{
	PCHAR Token;
	PCHAR Sub;
	BOOL bFlag = FALSE;
	UNICODE_STRING uString;
	ANSI_STRING aString;
	DWORD dwLength = 0;

	Token = strtok(lpBuffer, ",");
	while (Token != NULL)
	{
		bFlag = TRUE;
		Sub = strstr(Token, "estimate");
		if (Sub != NULL)
		{
			dwLength = (DWORD)strlen(Sub);
			while (*Sub != ':')
				Sub++;

			Sub += JSON_PARSER_OFFSET;
			Sub[strlen(Sub) - 1] = 0;
			
			if (dwLength == 22)
			{
				RtlZeroMemory(Info->EstimatedRewardCurrent, MAX_PATH);
				RtlInitAnsiString(&aString, Sub);
				RtlAnsiStringToUnicodeString(&uString, &aString, TRUE);
				swprintf(Info->EstimatedRewardCurrent, MAX_PATH, L"%ws", uString.Buffer);
				RtlFreeUnicodeString(&uString);
				UpdateAgentInfo(Info);
			}
			else {

				RtlZeroMemory(Info->EstimatedRewardDaily, MAX_PATH);
				RtlInitAnsiString(&aString, Sub);
				RtlAnsiStringToUnicodeString(&uString, &aString, TRUE);
				swprintf(Info->EstimatedRewardDaily, MAX_PATH, L"%ws", uString.Buffer);
				RtlFreeUnicodeString(&uString);
				UpdateAgentInfo(Info);
			}
			
		}

		Token = strtok(NULL, ",");
	}
	
	return bFlag;
}

BOOL GetBalance(PAGENT_INFO Info, LPVOID lpBuffer)
{
	PCHAR Token;
	PCHAR Sub;
	BOOL bFlag = FALSE;
	UNICODE_STRING uString;
	ANSI_STRING aString;

	Token = strtok(lpBuffer, ",");
	while (Token != NULL)
	{
		bFlag = TRUE;
		Sub = strstr(Token, "balance");
		if (Sub != NULL)
		{
			while (*Sub != ':')
				Sub++;

			Sub += JSON_PARSER_OFFSET;
			Sub[strlen(Sub) - 1] = 0;

		
			RtlZeroMemory(Info->Balance, MAX_PATH);
			RtlInitAnsiString(&aString, Sub);
			RtlAnsiStringToUnicodeString(&uString, &aString, TRUE);
			swprintf(Info->Balance, MAX_PATH, L"%ws", uString.Buffer);
			RtlFreeUnicodeString(&uString);
			UpdateAgentInfo(Info);
		}

		Token = strtok(NULL, ",");
	}

	return bFlag;
}

BOOL GetHashRate(PAGENT_INFO Info, LPVOID lpBuffer)
{
	PCHAR Token;
	PCHAR Sub;
	BOOL bFlag = FALSE;
	UNICODE_STRING uString;
	ANSI_STRING aString;

	Token = strtok(lpBuffer, ",");
	while (Token != NULL)
	{
		bFlag = TRUE;
		Sub = strstr(Token, "hashrate");
		if (Sub != NULL)
		{
			while (*Sub != ':')
				Sub++;

			Sub += JSON_PARSER_OFFSET;
			Sub[strlen(Sub) - 1] = 0;

		
			RtlZeroMemory(Info->HashRate, MAX_PATH);
			RtlInitAnsiString(&aString, Sub);
			RtlAnsiStringToUnicodeString(&uString, &aString, TRUE);
			swprintf(Info->HashRate, MAX_PATH, L"%ws", uString.Buffer);
			RtlFreeUnicodeString(&uString);
			UpdateAgentInfo(Info);
		}

		Token = strtok(NULL, ",");
	}

	if (wcslen(Info->HashRate) < 3)
	{
		swprintf(Info->HashRate, MAX_PATH, L"%ws", L"0.0");
		UpdateAgentInfo(Info);
	}
		

	return bFlag;

}

VOID SetStatusCoord(LPWSTR lpString, DWORD dwY)
{
	COORD Block = { 0 };
	SYSTEMTIME Time = { 0 };
	WCHAR WriteData[MAX_PATH] = { 0 };
	Block.X = 0; Block.Y = (SHORT)dwY;

	GetLocalTime(&Time);
	SetConsoleCursorPosition(hConsole, Block);

	swprintf(WriteData, MAX_PATH, L"%d:%d:%d -- %ws", Time.wHour, Time.wMinute, Time.wSecond, lpString);

	printf("%ws", WriteData);
}