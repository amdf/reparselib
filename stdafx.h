/*! \mainpage ReparseLib
 *
 * A library for working with NTFS Reparse Points
 *
 * \section usage_sec Using
 *
 * ReparseLib is a dynamic link library for Windows applications.
 * File name is reparselib.dll.
 */
#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#define WIN32_NO_STATUS

#include <windows.h>
#include <WinIoCtl.h>


HANDLE OpenFileForWrite(IN LPCWSTR sFileName, IN BOOL bBackup);
HANDLE OpenFileForRead(IN LPCWSTR sFileName, IN BOOL bBackup);
