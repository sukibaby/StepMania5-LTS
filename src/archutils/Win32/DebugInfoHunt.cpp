#include "global.h"
#include "DebugInfoHunt.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "VideoDriverInfo.h"
#include "RegistryAccess.h"
#include "ErrorStrings.h"

#include <vector>

#include <windows.h>
#include <mmsystem.h>


static void LogVideoDriverInfo( VideoDriverInfo info )
{
	LOG->Info( "Video driver: %s [%s]", info.sDescription.c_str(), info.sProvider.c_str() );
	LOG->Info( "              %s, %s [%s]", info.sVersion.c_str(), info.sDate.c_str(), info.sDeviceID.c_str() );
}

static void GetMemoryDebugInfo()
{
	MEMORYSTATUSEX mem;
	mem.dwLength = sizeof(mem);
	if (GlobalMemoryStatusEx(&mem))
	{
		LOG->Info("Memory: %lluMB total, %lluMB swap (%lluMB swap avail)",
			mem.ullTotalPhys / (1024 * 1024),
			mem.ullTotalPageFile / (1024 * 1024),
			mem.ullAvailPageFile / (1024 * 1024));
	}
	else
	{
		LOG->Warn("GlobalMemoryStatusEx failed: %s", werr_ssprintf(GetLastError(), "GlobalMemoryStatusEx").c_str());
	}
}

static void GetDisplayDriverDebugInfo()
{
	RString sPrimaryDeviceName = GetPrimaryVideoName();

	if( sPrimaryDeviceName == "" )
		LOG->Info( "Primary display driver could not be determined." );

	bool LoggedSomething = false;
	for( int i=0; true; i++ )
	{
		VideoDriverInfo info;
		if( !GetVideoDriverInfo(i, info) )
			break;

		if( sPrimaryDeviceName == "" )	// failed to get primary display name (NT4)
		{
			LogVideoDriverInfo( info );
			LoggedSomething = true;
		}
		else if( info.sDescription == sPrimaryDeviceName )
		{
			LogVideoDriverInfo( info );
			LoggedSomething = true;
			break;
		}
	}
	if( !LoggedSomething )
	{
		LOG->Info( "Primary display driver: %s", sPrimaryDeviceName.c_str() );
		LOG->Warn("Couldn't find primary display driver; logging all drivers");

		for( int i=0; true; i++ )
		{
			VideoDriverInfo info;
			if( !GetVideoDriverInfo(i, info) )
				break;

			LogVideoDriverInfo( info );
		}
	}
}

static RString wo_ssprintf( MMRESULT err, const char *fmt, ...)
{
	char buf[MAXERRORLENGTH];
	waveOutGetErrorText(err, buf, MAXERRORLENGTH);

	va_list	va;
	va_start(va, fmt);
	RString s = vssprintf( fmt, va );
	va_end(va);

	return s += ssprintf( "(%s)", buf );
}

static void GetDriveDebugInfo()
{
	/*
	 * HKEY_LOCAL_MACHINE\HARDWARE\DEVICEMAP\Scsi\
	 *    Scsi Port *\
	 *      DMAEnabled  0 or 1
	 *      Driver      "Ultra", "atapi", etc
	 *      Scsi Bus *\
	 *	     Target Id *\
	 *	 	   Logical Unit Id *\
	 *		     Identifier  "WDC WD1200JB-75CRA0"
	 *			 Type        "DiskPeripheral"
	 */
	std::vector<RString> Ports;
	if( !RegistryAccess::GetRegSubKeys( "HKEY_LOCAL_MACHINE\\HARDWARE\\DEVICEMAP\\Scsi", Ports ) )
		return;

	for( unsigned i = 0; i < Ports.size(); ++i )
	{
		int DMAEnabled = -1;
		RegistryAccess::GetRegValue( Ports[i], "DMAEnabled", DMAEnabled );

		RString Driver;
		RegistryAccess::GetRegValue( Ports[i], "Driver", Driver );

		std::vector<RString> Busses;
		if( !RegistryAccess::GetRegSubKeys( Ports[i], Busses, "Scsi Bus .*" ) )
			continue;

		for( unsigned bus = 0; bus < Busses.size(); ++bus )
		{
			std::vector<RString> TargetIDs;
			if( !RegistryAccess::GetRegSubKeys( Busses[bus], TargetIDs, "Target Id .*" ) )
				continue;

			for( unsigned tid = 0; tid < TargetIDs.size(); ++tid )
			{
				std::vector<RString> LUIDs;
				if( !RegistryAccess::GetRegSubKeys( TargetIDs[tid], LUIDs, "Logical Unit Id .*" ) )
					continue;

				for( unsigned luid = 0; luid < LUIDs.size(); ++luid )
				{
					RString Identifier;
					RegistryAccess::GetRegValue( LUIDs[luid], "Identifier", Identifier );
					TrimRight( Identifier );
					LOG->Info( "Drive: \"%s\" Driver: %s DMA: %s",
						Identifier.c_str(), Driver.c_str(), DMAEnabled == 1? "yes":DMAEnabled == -1? "N/A":"NO" );
				}
			}
		}
	}
}

static void GetWindowsVersionDebugInfo()
{
	typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
	HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
	if (hMod)
	{
		RtlGetVersionPtr pRtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
		if (pRtlGetVersion)
		{
			OSVERSIONINFOEXW osvi = { 0 };
			osvi.dwOSVersionInfoSize = sizeof(osvi);
			if (pRtlGetVersion((PRTL_OSVERSIONINFOW)&osvi) == 0)
			{
				RString Ver = ssprintf("Windows %lu.%lu (", osvi.dwMajorVersion, osvi.dwMinorVersion);
				if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
				{
					Ver += "Win7";
				}
				else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2)
				{
					Ver += "Win8";
				}
				else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3)
				{
					Ver += "Win8.1";
				}
				else if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0 && osvi.dwBuildNumber < 22000)
				{
					Ver += "Win10";
				}
				else if (osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0 && osvi.dwBuildNumber >= 22000)
				{
					Ver += "Win11";
				}
				else
				{
					Ver += "unknown";
				}

				Ver += ssprintf(") build %lu [%s]", osvi.dwBuildNumber, osvi.szCSDVersion);
				LOG->Info("%s", Ver.c_str());
				return;
			}
		}
	}
	LOG->Info("RtlGetVersion failed!");
}

static void GetSoundDriverDebugInfo()
{
	int cnt = waveOutGetNumDevs();

	for(int i = 0; i < cnt; ++i)
	{
		WAVEOUTCAPS caps;

		MMRESULT ret = waveOutGetDevCaps(i, &caps, sizeof(caps));
		if(ret != MMSYSERR_NOERROR)
		{
			LOG->Info(wo_ssprintf(ret, "waveOutGetDevCaps(%i) failed", i));
			continue;
		}
		LOG->Info("Sound device %i: %s, %i.%i, MID %i, PID %i %s", i, caps.szPname,
			HIBYTE(caps.vDriverVersion),
			LOBYTE(caps.vDriverVersion),
			caps.wMid, caps.wPid,
			caps.dwSupport & WAVECAPS_SAMPLEACCURATE? "":"(INACCURATE)");
	}
}

void SearchForDebugInfo()
{
	GetWindowsVersionDebugInfo();
	GetMemoryDebugInfo();
	GetDisplayDriverDebugInfo();
	GetDriveDebugInfo();
	GetSoundDriverDebugInfo();
}

/*
 * (c) 2003-2004 Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
