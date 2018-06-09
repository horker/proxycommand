#define _CRT_SECURE_NO_WARNINGS 1
#include <windows.h>
#include <string.h>
#include <stdio.h>

// Show an error message and exit
void ExitOnError(const char* message)
{
	// ref:
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms680582(v=vs.85).aspx

	DWORD errorCode = GetLastError();
	char* buff = 0;

	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&buff,
		0, NULL);

	printf("%s: %s\n", message, buff);

	LocalFree(buff);

	exit(1);
}

static wchar_t executableName[MAX_PATH + 1];

void InitializeExecutableName()
{
	if (GetModuleFileName(NULL, executableName, MAX_PATH) == 0) {
		ExitOnError("Failed to get the executable name");
	}
}

void ReadDataStream(wchar_t* out, DWORD length, const wchar_t* streamName)
{
	wchar_t name[MAX_PATH * 2];

	wcscpy(name, executableName);
	wcscat(name, L":");
	wcscat(name, streamName);

	HANDLE h = CreateFile(
		name,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		INVALID_HANDLE_VALUE);

	if (h == INVALID_HANDLE_VALUE) {
		ExitOnError("Failed open the alternative data stream");
	}

	DWORD lengthRead = -1;

	if (ReadFile(h, out, length - 1, &lengthRead, NULL) == 0) {
		ExitOnError("Failed to read the alternative data stream");
	}

	size_t len = lengthRead / sizeof(wchar_t);
	wchar_t* p = out + len - 1;

	while (iswspace(*p)) {
		--p;
	}
	*(p + 1) = 0;
}

bool ExistsDataStream(const wchar_t* streamName)
{
	wchar_t name[MAX_PATH * 2];

	wcscpy(name, L":");
	wcscat(name, streamName);
	wcscat(name, L":$DATA");

	WIN32_FIND_STREAM_DATA streamData;

	HANDLE h = FindFirstStreamW(executableName, FindStreamInfoStandard, &streamData, 0);

	if (h == INVALID_HANDLE_VALUE) {
		return false;
	}

	do {
		if (wcscmp(streamData.cStreamName, name) == 0) {
			FindClose(h);
			return true;
		}
	} while (FindNextStreamW(h, &streamData));

	FindClose(h);

	return false;
}

void EncloseWithDoubleQuotes(wchar_t* out, const wchar_t* in)
{
	const wchar_t* p = in;
	if (in[0] == 0xfeff) {
		// Skip BOM
		++p;
	}

	wchar_t * q = out;
	*q++ = L'"';
	while (*p) {
		*q++ = *p++;
	}
	*q++ = L'"';
	*q = 0;
}

void ReplaceCommandLineWithTargetPath(wchar_t* out, const wchar_t* commandLine, const wchar_t* targetPath)
{
	// Skip the command name portion
	const wchar_t* p = commandLine;
	if (*p == L'"') {
		do {
			++p;
		} while (*p && (*p != L'"' || *(p + 1) == L'"'));
		if (*p) {
			++p;
		}
	}
	else {
		while (*p && !iswspace(*p)) {
			++p;
		}
	}

	wcscpy(out, targetPath);
	wcscat(out, p);
}

int wmain()
{

	InitializeExecutableName();

	// Find the Async flag

	bool async = ExistsDataStream(L"Async");

	// Build a commnad line

	wchar_t targetPath[MAX_PATH * 2];
	ReadDataStream(targetPath, MAX_PATH * 2, L"TargetPath");

	wchar_t target[MAX_PATH * 2];
	EncloseWithDoubleQuotes(target, targetPath);

	wchar_t* line = (wchar_t*)malloc((65536 + MAX_PATH + 1) * sizeof(wchar_t));
	ReplaceCommandLineWithTargetPath(line, GetCommandLineW(), target);

	// Invoke the target command

	STARTUPINFO startupInfo;
	memset(&startupInfo, 0, sizeof(STARTUPINFO));
	startupInfo.cb = sizeof(startupInfo);

	PROCESS_INFORMATION processInformation;

	if (!CreateProcessW(
		NULL,                     // lpApplicationName
		line,                     // lpCommandLine
		NULL,                     // lpProcessAttributes
		NULL,                     // lpThreadAttributes
		true,                     // bInheritHandles
		NORMAL_PRIORITY_CLASS,    // dwCreationFlags
		NULL,                     // lpEnvironment
		NULL,                     // lpCurrentDirectory
		&startupInfo,             // lpStartupInfo
		&processInformation)) {   // lpProcessInformation
		ExitOnError("Failed to create a process");
	}

	// Wait the child process to exit

	if (!async) {
		HANDLE childProcess = processInformation.hProcess;
		WaitForSingleObject(childProcess, INFINITE);

		DWORD exitCode;
		if (!GetExitCodeProcess(childProcess, &exitCode)) {
			ExitOnError("Failed to get the child process's exit code");
		}

		return exitCode;
	}

	return 0;
}
