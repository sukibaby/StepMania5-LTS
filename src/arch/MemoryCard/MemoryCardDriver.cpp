#include "global.h"
#include "MemoryCardDriver.h"
#include "RageFileManager.h"
#include "RageLog.h"
#include "ProfileManager.h"

#include <vector>


static const RString TEMP_MOUNT_POINT = "/@mctemptimeout/";

enum MemoryCardDriverType {
	MemoryCardDriverType_Usb,
	MemoryCardDriverType_Directory,
	NUM_MemoryCardDriverType,
	MemoryCardDriverType_Invalid
};

static const char *MemoryCardDriverTypeNames[] = {
	"USB",
	"Directory"
};
XToString(MemoryCardDriverType);
StringToX(MemoryCardDriverType);
LuaXType(MemoryCardDriverType);

Preference<MemoryCardDriverType> g_MemoryCardDriver("MemoryCardDriver", MemoryCardDriverType_Usb, nullptr, PreferenceType::Immutable);

bool UsbStorageDevice::operator==(const UsbStorageDevice& other) const {
	if (iBus != other.iBus) return false;
	if (iPort != other.iPort) return false;
	if (iLevel != other.iLevel) return false;
	if (sOsMountDir != other.sOsMountDir) return false;
	return true;
}

void UsbStorageDevice::SetOsMountDir( const RString &s )
{
	sOsMountDir = s;
}

bool MemoryCardDriver::NeedUpdate( bool bMount )
{
	if( bMount )
	{
		/* Check if any devices need a write test. */
		for( unsigned i=0; i<m_vDevicesLastSeen.size(); i++ )
		{
			const UsbStorageDevice &d = m_vDevicesLastSeen[i];
			if( d.m_State == UsbStorageDevice::STATE_CHECKING )
				return true;
		}
	}

	return USBStorageDevicesChanged();
}

bool MemoryCardDriver::DoOneUpdate( bool bMount, std::vector<UsbStorageDevice>& vStorageDevicesOut )
{
	if( !NeedUpdate(bMount) )
		return false;

	std::vector<UsbStorageDevice> vOld = m_vDevicesLastSeen; // copy
	GetUSBStorageDevices( vStorageDevicesOut );

	// log connects
	for (UsbStorageDevice &newd : vStorageDevicesOut)
	{
		std::vector<UsbStorageDevice>::iterator iter = find( vOld.begin(), vOld.end(), newd );
		if( iter == vOld.end() )    // didn't find
			LOG->Trace( "New device connected: %s", newd.sDevice.c_str() );
	}

	/* When we first see a device, regardless of bMount, just return it as CHECKING,
	 * so the main thread knows about the device.  On the next call where bMount is
	 * true, check it. */
	for( unsigned i=0; i<vStorageDevicesOut.size(); i++ )
	{
		UsbStorageDevice &d = vStorageDevicesOut[i];

		/* If this device was just connected (it wasn't here last time), set it to
		 * CHECKING and return it, to let the main thread know about the device before
		 * we start checking. */
		std::vector<UsbStorageDevice>::iterator iter = find( vOld.begin(), vOld.end(), d );
		if( iter == vOld.end() )    // didn't find
		{
			LOG->Trace( "New device entering CHECKING: %s", d.sDevice.c_str() );
			d.m_State = UsbStorageDevice::STATE_CHECKING;
			continue;
		}

		/* Preserve the state of the device, and any data loaded from previous checks. */
		d.m_State = iter->m_State;
		d.bIsNameAvailable = iter->bIsNameAvailable;
		d.sName = iter->sName;

		/* The device was here last time.  If CHECKING, check the device now, if
		 * we're allowed to. */
		if( d.m_State == UsbStorageDevice::STATE_CHECKING )
		{
			if( !bMount )
			{
				/* We can't check it now.  Keep STATE_CHECKING, and check it when we can. */
				d.m_State = UsbStorageDevice::STATE_CHECKING;
				continue;
			}

			if( !this->Mount(&d) )
			{
				d.SetError( "MountFailed" );
				continue;
			}

			if( TestWrite(&d) )
			{
				/* We've successfully mounted and tested the device.  Read the
				 * profile name (by mounting a temporary, private mountpoint),
				 * and then unmount it until Mount() is called. */
				d.m_State = UsbStorageDevice::STATE_READY;

				FILEMAN->Mount( "dirro", d.sOsMountDir, TEMP_MOUNT_POINT );
				d.bIsNameAvailable = PROFILEMAN->FastLoadProfileNameFromMemoryCard( TEMP_MOUNT_POINT, d.sName );
				FILEMAN->Unmount( "dirro", d.sOsMountDir, TEMP_MOUNT_POINT );
			}

			this->Unmount( &d );

			LOG->Trace( "WriteTest: %s, Name: %s", d.m_State == UsbStorageDevice::STATE_ERROR? "failed":"succeeded", d.sName.c_str() );
		}
	}

	m_vDevicesLastSeen = vStorageDevicesOut;

	return true;
}

#include "arch/arch_default.h"
MemoryCardDriver *MemoryCardDriver::Create()
{
	MemoryCardDriver *ret = nullptr;

	switch( g_MemoryCardDriver )
	{
		case MemoryCardDriverType_Directory:
			ret = new MemoryCardDriverThreaded_Folder;
			break;
		case MemoryCardDriverType_Usb:
#ifdef ARCH_MEMORY_CARD_DRIVER
			ret = new ARCH_MEMORY_CARD_DRIVER;
#endif
			break;
		default:
			break;
	}

	if( !ret )
		ret = new MemoryCardDriver_Null;

	return ret;
}


/*
 * (c) 2002-2004 Glenn Maynard
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
