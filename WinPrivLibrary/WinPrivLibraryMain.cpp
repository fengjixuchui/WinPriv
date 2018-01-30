#include <windows.h>
#include <detours.h>

#pragma comment(lib,"ntdll.lib")
#pragma comment(lib,"detours.lib")

EXTERN_C VOID WINAPI DllExtraAttach();
EXTERN_C VOID WINAPI DllExtraDetach();

static CHAR sDetourLibrary[MAX_PATH + 1] = "";

decltype(&GetProcAddress) TrueGetProcAddress = GetProcAddress;
decltype(&CreateProcessA) TrueCreateProcessA = CreateProcessA;
decltype(&CreateProcessW) TrueCreateProcessW = CreateProcessW;

EXTERN_C BOOL WINAPI DetourCreateProcessA(_In_opt_ LPCSTR lpApplicationName, _Inout_opt_ LPSTR lpCommandLine,
	_In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes, _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
	_In_ BOOL bInheritHandles, _In_ DWORD dwCreationFlags, _In_opt_ LPVOID lpEnvironment,
	_In_opt_ LPCSTR lpCurrentDirectory, _In_ LPSTARTUPINFOA lpStartupInfo, _Out_ LPPROCESS_INFORMATION lpProcessInformation
)
{
	return DetourCreateProcessWithDllExA(lpApplicationName, lpCommandLine, lpProcessAttributes,
		lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory,
		lpStartupInfo, lpProcessInformation, sDetourLibrary, TrueCreateProcessA);
}

EXTERN_C BOOL WINAPI DetourCreateProcessW(_In_opt_ LPCWSTR lpApplicationName, _Inout_opt_ LPWSTR lpCommandLine,
	_In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes, _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
	_In_ BOOL bInheritHandles, _In_ DWORD dwCreationFlags, _In_opt_ LPVOID lpEnvironment,
	_In_opt_ LPCWSTR lpCurrentDirectory, _In_ LPSTARTUPINFOW lpStartupInfo, _Out_ LPPROCESS_INFORMATION lpProcessInformation
)
{
	return DetourCreateProcessWithDllExW(lpApplicationName, lpCommandLine, lpProcessAttributes,
		lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, 
		lpStartupInfo, lpProcessInformation, sDetourLibrary, TrueCreateProcessW);
}

EXTERN_C BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
	if (DetourIsHelperProcess())
	{
		return TRUE;
	}

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		DetourRestoreAfterWith();
		GetModuleFileNameA(hinst, sDetourLibrary, ARRAYSIZE(sDetourLibrary));
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DllExtraAttach();
		DetourAttach(&(PVOID&)TrueCreateProcessA, DetourCreateProcessA);
		DetourAttach(&(PVOID&)TrueCreateProcessW, DetourCreateProcessW);
		DetourTransactionCommit();
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DllExtraDetach();
		DetourDetach(&(PVOID&)TrueCreateProcessA, DetourCreateProcessA);
		DetourDetach(&(PVOID&)TrueCreateProcessW, DetourCreateProcessW);
		DetourTransactionCommit();
	}

	return TRUE;
}

