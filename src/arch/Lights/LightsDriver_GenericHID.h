#ifndef LIGHTSDRIVER_LINUXPACDRIVE_H
#define LIGHTSDRIVER_LINUXPACDRIVE_H

#include "LightsDriver.h"

#include <cstdint>
#include <libusb.h>

#define BIT(i) (1<<(i))

class USBContext
{
public:
	static USBContext& getInstance()
	{
		static USBContext instance;
		return instance;
	}

	libusb_context* getContext() { return context; }

private:
	USBContext()
	{
		int result = libusb_init_context(&context, NULL, 0);
		if (result < 0)
		{
			// initialization error
			context = nullptr;
		}
	}

	~USBContext()
	{
		if (context)
		{
			libusb_exit(context);
		}
	}

	libusb_context* context;

	// prevent copying
	USBContext(const USBContext&) = delete;
	USBContext& operator=(const USBContext&) = delete;
};

class LightsDriver_GenericHID: public LightsDriver
{
public:
	LightsDriver_GenericHID();
	~LightsDriver_GenericHID();

	void Set( const LightsState *ls );
private:

	void OpenDevice();
	void WriteDevice(uint16_t out);
	void CloseDevice();

	libusb_device_handle *DeviceHandle;
	int iLightsOrder;
};

#endif // LIGHTSDRIVER_LINUXPACDRIVE_H

/*
 * Copyright (c) 2008 BoXoRRoXoRs
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
