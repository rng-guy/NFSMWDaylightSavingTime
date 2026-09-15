
// Compatibility checks -----------------------------------------------------------------------------------------------------------------------------

#ifndef _MSC_VER
#error "DST requires MSVC."

#elif (_MSC_VER < 1930)
#error "DST requires Visual Studio 2022 or newer."

#elif ((not defined(_WIN32)) or defined(_WIN64))
#error "DST requires 32-bit Windows."

#elif ((not defined(_MSVC_LANG)) or (_MSVC_LANG < 202002L))
#error "DST requires C++20 or newer."

#endif





// Project includes ---------------------------------------------------------------------------------------------------------------------------------

#include <Windows.h>

#include "Headers/MemoryTools.hpp"





// Aliases ------------------------------------------------------------------------------------------------------------------------------------------

using MemoryTools::address;

using MemoryTools::AsReference;
using MemoryTools::AsFunction;





// Auxiliary functions ------------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] static float __cdecl GetTimeScale()
{
	const auto IsPaused = AsFunction<bool __cdecl ()>(0x468390);
	if (IsPaused()) return 0.f; // do not progress time of day

	const address simSystem = AsReference<address>(0x9885E0);
	return (simSystem) ? AsReference<float>(simSystem + 0x24) : 1.f;
}





// Assembly detours ---------------------------------------------------------------------------------------------------------------------------------

// Updates time of day, accounting for pausing and the Speedbreaker's time dilation
ASSEMBLY_DETOUR(TimeOfDayUpdate, /* begin = */ 0x7693AF, /* end = */ 0x7693B6)
{
	__asm
	{
		call GetTimeScale
		fmulp st(1), st(0)

		mov ecx, dword ptr [esi + 0x4] // restore value

		// Execute original code and resume
		fadd dword ptr [esi + 0x8]
		fst dword ptr [esp + 0x18]

		EXIT_ASSEMBLY_DETOUR(TimeOfDayUpdate)
	}
}





// DLL hook boilerplate -----------------------------------------------------------------------------------------------------------------------------

BOOL WINAPI DllMain
(
	const HINSTANCE hinstDLL,
	const DWORD     fdwReason,
	const LPVOID    lpvReserved
) {
	if (fdwReason != DLL_PROCESS_ATTACH) return TRUE;

	if (MemoryTools::GetEntryPoint() != 0x3C4040) // .exe-dependent entry point
	{
		MessageBoxA(NULL, "This .exe isn't compatible with DST.\nSee DST's README for help.", "NFSMW DaylightSavingTime", MB_ICONERROR);

		return FALSE; // should never happen (assuming the user has actually read the README, which... yeah...)
	}

	PATCH_ASSEMBLY_DETOUR(TimeOfDayUpdate);

	return TRUE;
}