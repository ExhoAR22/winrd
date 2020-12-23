#pragma once
#include <wdm.h>

#define CHECK_EXPECTATION(Result, Expectation) \
	if ((Status = Result) != Expectation) { \
		goto Cleanup; \
	}

#define CHECK(Result) CHECK_EXPECTATION(Result, STATUS_SUCCESS)
#define TRACE(Format, ...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0xffffffff, "[WinRD][" __FUNCTION__ "] " Format "\n", __VA_ARGS__)