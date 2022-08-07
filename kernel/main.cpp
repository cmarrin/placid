/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include <circle/startup.h>
#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/serial.h>
#include <circle/logger.h>
#include <circle/debug.h>

class MyKernel
{
public:
	MyKernel()
		: 	_timer (&_interrupt)
		,	_logger(_options.GetLogLevel())
	{
		_actLED.Off();
	}

	~MyKernel() { }

	bool initialize()
	{
		bool result = true;

		if (result) {
			result = _serial.Initialize(115200);
		}
	
		if (result) {
			result = _logger.Initialize(&_serial);
		}
	
		if (result) {
			result = _interrupt.Initialize();
		}

		if (result) {
			result = _timer.Initialize();
		}

		_timer.StartKernelTimer(HZ / 2, timerHandler, this);

		return result;
	}

	void run()
	{
		_serial.Write("Hello World!\n", 13);
		_serial.Write("enter text> ", 12);
		
		int result = 0xff;
		while (true) {
			char c;
			result = _serial.Read(&c, 1);
			if (result == 0) {
				//_timer.usDelay(1000);
				continue;
			}
			if (result != 1) {
				break;
			}
			_serial.Write(&c, 1);
		}
		
		_logger.Write("kernel", LogNotice, "************** Serial Read Result: %d\n", result);
		
		_logger.Write("kernel", LogNotice, "Compile time: " __DATE__ " " __TIME__);
		
		_timer.MsDelay(10 * 1000);

#ifndef NDEBUG
		// some debugging features
		_logger.Write("kernel", LogDebug, "THIS IS FROM PLACID 12!!!");
		_logger.Write("kernel", LogDebug, "Dumping the start of the ATAGS");
		debug_hexdump((void *) 0x100, 128, "kernel");

		_logger.Write("kernel", LogNotice, "The following assertion will fail");
		assert (1 == 2);
#endif
	}
	
private:
	static void timerHandler(TKernelTimerHandle hTimer, void *param, void *context)
	{
		MyKernel* kernel = reinterpret_cast<MyKernel*>(param);
		kernel->_blinkOn = !kernel->_blinkOn;
		if (kernel->_blinkOn) {
			kernel->_actLED.On();
		} else {
			kernel->_actLED.Off();
		}

		kernel->_timer.StartKernelTimer(HZ / 2, timerHandler, kernel);
	}
	
	// do not change this order
	CActLED				_actLED;
	CKernelOptions		_options;
	CDeviceNameService	_deviceNameService; // MUST BE HERE EVEN THOUGH NOT USED!
	CSerialDevice		_serial;
	CInterruptSystem	_interrupt;
	CTimer				_timer;
	CLogger				_logger;
	
	bool _blinkOn = false;
};

int main()
{
	// cannot return here because some destructors used in CKernel are not implemented

	MyKernel kernel;
	if (!kernel.initialize())
	{
		halt();
		return EXIT_HALT;
	}
	
	kernel.run();
	halt();
	return EXIT_HALT;
}
