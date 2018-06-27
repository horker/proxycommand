#include <windows.h>
#ifndef _NDEBUG
#include <crtdbg.h>
#endif

////////////////////////////////////////////////////////////
// Memory functions

void Print(const wchar_t* line);

static void* AllocateUntyped(size_t size)
{
    void* m = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    if (m == NULL) {
        Print(L"Not enough memory\n");
        ExitProcess(-1);
    }

    return m;
}

template <class T>
inline
static T* Allocate(size_t size, T)
{
    return (T*)AllocateUntyped(sizeof(T) * size);
}

inline
static void Free(void* p)
{
    HeapFree(GetProcessHeap(), 0, p);
}

template <class T>
inline
void CopyData(T* dest, const T* src, size_t size)
{
    char* p = (char*)dest;
    char* pEnd = p + sizeof(T) * size;
    char* q = (char*)src;

    while (p < pEnd) {
        *p++ = *q++;
    }
}

// The compiler automatically replaces the ClearMemory() implementation below with that using memset() and causes CRT dependancy.
// To avoid this, we should use RtlZeroMemory() defined in kernel.dll.

#undef RtlZeroMemory
extern "C" void RtlZeroMemory(LPVOID lpdest, int n);

inline
static void ClearMemory(LPVOID lpdest, int n)
{
    RtlZeroMemory(lpdest, n);
}

/*
void ClearMemory(void* mem, size_t size)
{
    char* p = (char*)mem;
    while (size-- > 0) {
        *p++ = 0;
    }
}
*/

////////////////////////////////////////////////////////////
// String functions

template <class T>
static size_t GetStringLength(const T* s)
{
    size_t l = 0;
    while (*s) {
        ++l;
        ++s;
    }

    return l;
}

static char* ConvertWC2C(const wchar_t* s)
{
    size_t bufferSize = GetStringLength(s) * sizeof(wchar_t);

    char* p = Allocate(bufferSize, char());

    size_t size = WideCharToMultiByte(CP_ACP, 0, s, -1, p, (int)bufferSize - 1, NULL, NULL);
    *(p + size) = 0;

    return p;
}

////////////////////////////////////////////////////////////
// Stdout output

HANDLE StdErr = NULL;

inline
static void InitializeStdOut()
{
    StdErr = GetStdHandle(STD_ERROR_HANDLE);
}

static void Print(const wchar_t* line)
{
    char* p = ConvertWC2C(line);

    size_t l = GetStringLength(p);
    WriteFile(StdErr, p, (DWORD)l, NULL, NULL);
    FlushFileBuffers(StdErr);

    Free(p);
}

////////////////////////////////////////////////////////////
// class StringT

template <class T>
class StringT
{
private:

    static const size_t DEFAULT_CAPACITY = 128;

    T* buffer_;
    size_t length_;
    size_t capacity_;

public:

    StringT()
        : StringT(DEFAULT_CAPACITY)
    {
    }

    StringT(size_t capacity)
        : buffer_(Allocate(capacity, T())),
        length_(0),
        capacity_(capacity)
    {
        *buffer_ = 0;
    }

    StringT(const T* s)
    {
        length_ = GetStringLength(s);
        capacity_ = length_ + 10;
        buffer_ = Allocate(length_ + 10, T());

        CopyData(buffer_, s, length_ + 1); // including null
    }

    StringT(const StringT& s)
        : StringT(s.Buffer())
    {
    }

    ~StringT()
    {
        Free(buffer_);
    }

    StringT& operator=(const StringT& rhs)
    {
        if (this == &rhs) {
            return *this;
        }

        Clear();
        Append(rhs);

        return *this;
    }

    size_t Length() const
    {
        return length_;
    }

    size_t Capacity() const
    {
        return capacity_;
    }

    T* Buffer() const
    {
        return buffer_;
    }

    T operator[](size_t index) const
    {
        return *(buffer_ + index);
    }

    T Back(size_t offset = 0) const
    {
        return *(buffer_ + length_ - 1 - offset);
    }

    bool operator==(const T* other) const
    {
        const T* p = buffer_;
        while (*p == *other) {
            if (*p == 0) {
                return true;
            }
            ++p;
            ++other;
        }

        return false;
    }

    bool operator==(const StringT<T>& other) const
    {
        if (Length() != other.Length()) {
            return false;
        }

        return this == other.Buffer();
    }

    void Append(const T* s, size_t length)
    {
        EnsureCapacity(length);

        // Don't assume that s is null-terminated.
        CopyData(buffer_ + length_, s, length);
        *(buffer_ + length_ + length) = 0;
        length_ += length;
    }

    void Append(const T* s)
    {
        Append(s, GetStringLength(s));
    }

    void Append(const StringT<T>& s)
    {
        Append(s.Buffer(), s.Length());
    }

    void RemoveAt(size_t start, size_t length)
    {
        if (start == length_ - 1) {
            *(buffer_ + length_ - 1) = 0;
            --length_;
            return;
        }

        CopyData(buffer_ + start, buffer_ + start + length, length_ - start - length);
        *(buffer_ + length_ - length) = 0;
        length_ -= length;
    }

    void Clear()
    {
        length_ = 0;
        *buffer_ = 0;
    }

    StringT<T> Substring(size_t start, size_t length = (size_t)-1)
    {
        if (length == (size_t)-1) {
            length = Length() - start;
        }

        StringT<T> s(length + 10);

        T* p = s.Buffer();
        T* q = buffer_ + start;

        while (length-- > 0) {
            *p++ = *q++;
        }
        *p = 0;

        return s;
    }

private:

    void EnsureCapacity(size_t required)
    {
        _ASSERT(capacity_ >= length_ + 1);
        size_t c = capacity_;
        if (c - (length_ + 1) >= required) {
            return;
        }

        while (c - (length_ + 1) < required) {
            c *= 2;
        }

        T* m = Allocate(c, T());

        CopyData(m, buffer_, length_ + 1); // including null

        Free(buffer_);
        buffer_ = m;
        capacity_ = c;
    }
};

typedef StringT<wchar_t> String;

////////////////////////////////////////////////////////////
// ExitOnError()
//
// Show an error message and exit
//

static void ExitOnError(const wchar_t* message)
{
    // ref:
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms680582(v=vs.85).aspx

    DWORD errorCode = GetLastError();
    wchar_t* buff = 0;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&buff,
        0, NULL);

    String s(L"[ProxyCommand] ");
    s.Append(message);
    s.Append(L": ");
    s.Append(buff);
    s.Append(L"\n");

    LocalFree(buff);

    Print(s.Buffer());

    ExitProcess(1);
}

////////////////////////////////////////////////////////////
// InitializeExecutableName()

static wchar_t executableName[MAX_PATH + 1];

inline
static void InitializeExecutableName()
{
    if (GetModuleFileName(NULL, executableName, MAX_PATH) == 0) {
        ExitOnError(L"Failed to get the executable name");
    }
}

////////////////////////////////////////////////////////////
// ReadDataStream()

static String ReadDataStream(const wchar_t* streamName)
{
    String name(executableName);
    name.Append(L":");
    name.Append(streamName);

    HANDLE h = CreateFile(
        name.Buffer(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        INVALID_HANDLE_VALUE);

    if (h == INVALID_HANDLE_VALUE) {
        ExitOnError(L"Failed open the alternative data stream");
    }

    String result;

    const int BUFFER_SIZE = 256;
    wchar_t buffer[BUFFER_SIZE];
    DWORD lengthRead = -1;

    do {
        if (ReadFile(h, buffer, BUFFER_SIZE - 1, &lengthRead, NULL) == 0) {
            ExitOnError(L"Failed to read the alternative data stream");
        }

        if (lengthRead > 0) {
            result.Append(buffer, lengthRead / sizeof(wchar_t));
        }
    } while (lengthRead == BUFFER_SIZE - 1);

    CloseHandle(h);

    while (result.Back() == L'\r' || result.Back() == L'\n') {
        result.RemoveAt(result.Length() - 1, 1);
    }

    if (result[0] == 0xfeff) {
        // Remove BOM
        result.RemoveAt(0, 1);
    }

    return result;
}

////////////////////////////////////////////////////////////
// ExistsDataStream()

static bool ExistsDataStream(const wchar_t* streamName)
{
    WIN32_FIND_STREAM_DATA streamData;

    HANDLE h = FindFirstStreamW(executableName, FindStreamInfoStandard, &streamData, 0);

    if (h == INVALID_HANDLE_VALUE) {
        return false;
    }

    String name(L":");
    name.Append(streamName);
    name.Append(L":$DATA");

    do {
        if (name == streamData.cStreamName) {
            FindClose(h);
            return true;
        }
    } while (FindNextStreamW(h, &streamData));

    FindClose(h);

    return false;
}

////////////////////////////////////////////////////////////
// GetArgumentPortion()

static const wchar_t* GetArgumentPortion(const wchar_t* p)
{
    // Skip the command name portion
    if (*p == '"') {
        do {
            ++p;
        } while (*p && (*p != '"' || *(p + 1) == '"'));
        if (*p) {
            ++p;
        }
    }
    else {
        while (*p && *p != L' ' && *p != L'\t') {
            ++p;
        }
    }

    return p;
}

////////////////////////////////////////////////////////////
// main()

int main()
{
    InitializeExecutableName();
    InitializeStdOut();

    // Find the Async flag

    bool async = ExistsDataStream(L"Async");

    // Find a commnad line

    String prepend;

    bool argExists = ExistsDataStream(L"Prepend");
    if (argExists) {
        prepend = ReadDataStream(L"Prepend");
    }

    // Build a commnad line

    String targetPath = ReadDataStream(L"TargetPath");

    String line(L"\"");
    line.Append(targetPath);
    line.Append(L"\" ");

    if (argExists) {
        line.Append(prepend.Buffer());
        line.Append(L" ");
    }

    const wchar_t* p = GetArgumentPortion(GetCommandLine());
    line.Append(p);

    // Invoke the target command

    STARTUPINFO startupInfo;
    ClearMemory(&startupInfo, sizeof(STARTUPINFO));
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInformation;

    if (!CreateProcess(
        NULL,                     // lpApplicationName
        line.Buffer(),            // lpCommandLine
        NULL,                     // lpProcessAttributes
        NULL,                     // lpThreadAttributes
        true,                     // bInheritHandles
        NORMAL_PRIORITY_CLASS,    // dwCreationFlags
        NULL,                     // lpEnvironment
        NULL,                     // lpCurrentDirectory
        &startupInfo,             // lpStartupInfo
        &processInformation)) {   // lpProcessInformation
        ExitOnError(L"Failed to create a process");
    }

    // Wait for the child process to exit

    if (!async) {
        HANDLE childProcess = processInformation.hProcess;
        WaitForSingleObject(childProcess, INFINITE);

        DWORD exitCode;
        if (!GetExitCodeProcess(childProcess, &exitCode)) {
            ExitOnError(L"Failed to get the child process's exit code");
        }

        TerminateProcess(GetCurrentProcess(), exitCode);
    }

    TerminateProcess(GetCurrentProcess(), 0);
}
