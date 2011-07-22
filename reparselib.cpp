/** @file     reparselib.cpp
 *  @brief    A library for working with NTFS Reparse Points
 *  @author   amdf
 *  @version  0.1
 *  @date     May 2011
 */

#include "stdafx.h"
#include <ks.h> // because of GUID_NULL
#include "reparselib.h"

static HANDLE OpenFileForWrite(IN LPCWSTR sFileName, IN BOOL bBackup)
{
  return CreateFile(
    sFileName, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
    (bBackup)
    ? FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS
    : FILE_FLAG_OPEN_REPARSE_POINT, 0);
}

static HANDLE OpenFileForRead(IN LPCWSTR sFileName, IN BOOL bBackup)
{
  return CreateFile(
    sFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
    (bBackup)
    ? FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS
    : FILE_FLAG_OPEN_REPARSE_POINT, 0);
}


/**
 *  @brief      Determine whether the symbolic link is relative or not
 *  @param[in]  sFileName File name
 *  @return     TRUE if the link is relative, FALSE otherwise
 *              FALSE if not a symbolic link
 */
REPARSELIB_API BOOL IsSymbolicLinkRelative(IN LPCWSTR sFileName)
{
  DWORD dwTag;
  PREPARSE_DATA_BUFFER pReparse;
  BOOL bResult = FALSE;

  if (ReparsePointExists(sFileName))
  {
    if (GetReparseTag(sFileName, &dwTag))
    {
      if (IO_REPARSE_TAG_SYMLINK == dwTag)
      {
        pReparse = (PREPARSE_DATA_BUFFER)
          GlobalAlloc(GPTR, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

        GetReparseBuffer(sFileName, (PREPARSE_GUID_DATA_BUFFER)pReparse);

        bResult = (pReparse->SymbolicLinkReparseBuffer.Flags | SYMLINK_FLAG_RELATIVE);

        GlobalFree(pReparse);
      }
    }
  }
  return bResult;
}

/**
 *  @brief      Determine whether the file is a junction point
 *  @param[in]  sFileName File name
 *  @return     TRUE if the file is a junction point, FALSE otherwise
 */
REPARSELIB_API BOOL IsJunctionPoint(IN LPCWSTR sFileName)
{
  DWORD dwTag;
  PREPARSE_DATA_BUFFER pReparse;
  BOOL bResult = FALSE;
  PWCHAR pChar;

  if (ReparsePointExists(sFileName))
  {
    if (GetReparseTag(sFileName, &dwTag))
    {
      // IO_REPARSE_TAG_MOUNT_POINT is a common type for both
      // mount points and junction points
      if (IO_REPARSE_TAG_MOUNT_POINT == dwTag)
      {
        pChar = (PWCHAR) GlobalAlloc(GPTR, 16 * 1024);
        if (GetPrintName(sFileName, pChar, 16 * 1024))
        {
          if (0 != wcsncmp(L"\\??\\Volume", pChar, 10))
          {
            bResult = TRUE;
          }
        }
        GlobalFree(pChar);
      }
    }
  }
  return bResult;
}

/**
 *  @brief      Determine whether the file is a volume mount point
 *  @param[in]  sFileName File name
 *  @return     TRUE if the file is a volume mount point, FALSE otherwise
 */
REPARSELIB_API BOOL IsMountPoint(IN LPCWSTR sFileName)
{
  DWORD dwTag;
  PREPARSE_DATA_BUFFER pReparse;
  BOOL bResult = FALSE;
  PWCHAR pChar;

  if (ReparsePointExists(sFileName))
  {
    if (GetReparseTag(sFileName, &dwTag))
    {
      // IO_REPARSE_TAG_MOUNT_POINT is a common type for both
      // mount points and junction points
      if (IO_REPARSE_TAG_MOUNT_POINT == dwTag)
      {
        pChar = (PWCHAR) GlobalAlloc(GPTR, 16 * 1024);
        if (GetPrintName(sFileName, pChar, 16 * 1024))
        {
          if (0 == wcsncmp(L"\\??\\Volume", pChar, 10))
          {
            bResult = TRUE;
          }
        }
        GlobalFree(pChar);
      }
    }
  }
  return bResult;
}

/**
 *  @brief      Determine whether the file is a symbolic link
 *  @param[in]  sFileName File name
 *  @return     TRUE if the file is a symbolic link, FALSE otherwise
 */
REPARSELIB_API BOOL IsSymbolicLink(IN LPCWSTR sFileName)
{
  DWORD dwTag;
  if (!ReparsePointExists(sFileName))
  {
    return FALSE;
  } else
  {
    if (!GetReparseTag(sFileName, &dwTag))
    {
      return FALSE;
    } else
    {
      return (IO_REPARSE_TAG_SYMLINK == dwTag);
    }
  }
}

/**
 *  @brief      Checks an existence of a Reparse Point of a specified file
 *  @param[in]  sFileName File name
 *  @return     TRUE if exists, FALSE otherwise
 */
REPARSELIB_API BOOL ReparsePointExists(IN LPCWSTR sFileName)
{
  return (GetFileAttributes(sFileName) & FILE_ATTRIBUTE_REPARSE_POINT);
}

/**
 *  @brief      Get a reparse point buffer
 *  @param[in]  sFileName File name
 *  @param[out] pBuf Caller allocated buffer (16 Kb minimum)
 *  @return     TRUE if success
 */
REPARSELIB_API BOOL GetReparseBuffer(IN LPCWSTR sFileName, OUT PREPARSE_GUID_DATA_BUFFER pBuf)
{  
  DWORD dwRet;
  HANDLE hSrc;
  BOOL bResult = FALSE;

  if (NULL == pBuf)
  {
    return bResult;
  }

  if (!ReparsePointExists(sFileName))
  {
    return bResult;
  }

  hSrc = OpenFileForRead(sFileName,
    (GetFileAttributes(sFileName) & FILE_ATTRIBUTE_DIRECTORY));

  if (hSrc == INVALID_HANDLE_VALUE)
  {
    return bResult;
  }  

  if (DeviceIoControl(hSrc, FSCTL_GET_REPARSE_POINT,
    NULL, 0, pBuf, MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &dwRet, NULL))
  {    
    bResult = TRUE;
  }

  CloseHandle(hSrc);
  
  return bResult;
}

/**
 *  @brief      Get a GUID field of a reparse point
 *  @param[in]  sFileName File name
 *  @param[out] pGuid Pointer to GUID
 *  @return     TRUE if success
 */
REPARSELIB_API BOOL GetReparseGUID(IN LPCWSTR sFileName, OUT GUID* pGuid)
{  
  BOOL bResult = FALSE;

  if (NULL == pGuid)
  {
    return FALSE;
  }

  if (!ReparsePointExists(sFileName))
  {
    return FALSE;
  }
  
  PREPARSE_GUID_DATA_BUFFER rd
    = (PREPARSE_GUID_DATA_BUFFER) GlobalAlloc(GPTR, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

  if (GetReparseBuffer(sFileName, rd))
  {
    *pGuid = rd->ReparseGuid;
    bResult = TRUE;
  }

  GlobalFree(rd);

  return bResult;
}

/**
 *  @brief      Get a reparse tag of a reparse point
 *  @param[in]  sFileName File name
 *  @param[out] pTag Pointer to reparse tag
 *  @return     Reparse tag or 0 if fails
 */
REPARSELIB_API BOOL GetReparseTag(IN LPCWSTR sFileName, OUT DWORD* pTag)
{  
  BOOL bResult = FALSE;

  if (NULL == pTag)
  {
    return FALSE;
  }

  if (!ReparsePointExists(sFileName))
  {
    return FALSE;
  }
  
  PREPARSE_GUID_DATA_BUFFER rd
    = (PREPARSE_GUID_DATA_BUFFER) GlobalAlloc(GPTR, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

  if (GetReparseBuffer(sFileName, rd))
  {
    *pTag = rd->ReparseTag;
    bResult = TRUE;
  }

  GlobalFree(rd);

  return bResult;
}

/**
 *  @brief      Delete a reparse point
 *  @param[in]  sFileName File name
 *  @return     TRUE if success, FALSE otherwise
 */
REPARSELIB_API BOOL DeleteReparsePoint(IN LPCWSTR sFileName)
{
  PREPARSE_GUID_DATA_BUFFER rd;
  BOOL bResult;
  GUID gu;
  DWORD dwRet, dwReparseTag;

  if (!ReparsePointExists(sFileName) || !GetReparseGUID(sFileName, &gu))
  {
    return FALSE;
  }

  rd = (PREPARSE_GUID_DATA_BUFFER)
    GlobalAlloc(GPTR, REPARSE_GUID_DATA_BUFFER_HEADER_SIZE);  

  if (GetReparseTag(sFileName, &dwReparseTag))
  {
    rd->ReparseTag = dwReparseTag;
  } else
  {
    //! The routine cannot delete a reparse point without determining it's reparse tag
    GlobalFree(rd);
    return FALSE;
  }

  HANDLE hDel = OpenFileForWrite(sFileName,
    (GetFileAttributes(sFileName) & FILE_ATTRIBUTE_DIRECTORY));

  if (hDel == INVALID_HANDLE_VALUE)
  {
    return FALSE;
  }  

  // Try to delete a system type of the reparse point (without GUID)
  bResult = DeviceIoControl(hDel, FSCTL_DELETE_REPARSE_POINT,
      rd, REPARSE_GUID_DATA_BUFFER_HEADER_SIZE, NULL, 0,
      &dwRet, NULL);

  if (!bResult)
  {
    // Set up the GUID
    rd->ReparseGuid = gu;

    // Try to delete with GUID
    bResult = DeviceIoControl(hDel, FSCTL_DELETE_REPARSE_POINT,
        rd, REPARSE_GUID_DATA_BUFFER_HEADER_SIZE, NULL, 0,
        &dwRet, NULL);    
  }

  GlobalFree(rd);
  CloseHandle(hDel);
  return bResult;  
}

/**
 *  @brief      Creates a custom reparse point
 *  @param[in]  sFileName   File name
 *  @param[in]  pBuffer     Reparse point content
 *  @param[in]  uBufSize    Size of the content
 *  @param[in]  uGuid       Reparse point GUID
 *  @param[in]  uReparseTag Reparse point tag
 *  @return     TRUE if success, FALSE otherwise
 */
REPARSELIB_API BOOL CreateCustomReparsePoint
(
  IN LPCWSTR  sFileName,
  IN PVOID    pBuffer,
  IN UINT     uBufSize,
  IN GUID     uGuid,
  IN ULONG    uReparseTag
)
{ 
  DWORD dwLen = 0;
  BOOL bResult = FALSE;

  if (NULL == pBuffer || 0 == uBufSize || uBufSize > MAXIMUM_REPARSE_DATA_BUFFER_SIZE)
  {
    return bResult;
  }

  HANDLE hHandle = OpenFileForWrite(sFileName,
    (GetFileAttributes(sFileName) & FILE_ATTRIBUTE_DIRECTORY));

  if (INVALID_HANDLE_VALUE == hHandle)
  {
    return bResult;
  }

  PREPARSE_GUID_DATA_BUFFER rd
    = (PREPARSE_GUID_DATA_BUFFER) GlobalAlloc(GPTR, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
  
  rd->ReparseTag = uReparseTag;
  rd->ReparseDataLength = uBufSize;
  rd->Reserved = 0;
  rd->ReparseGuid = uGuid;

  memcpy(rd->GenericReparseBuffer.DataBuffer, pBuffer, uBufSize);

  if (DeviceIoControl(hHandle, FSCTL_SET_REPARSE_POINT, rd,
    rd->ReparseDataLength + REPARSE_GUID_DATA_BUFFER_HEADER_SIZE,
    NULL, 0, &dwLen, NULL))
  {
    bResult = TRUE;
  }

  CloseHandle(hHandle);
  GlobalFree(rd);
  
  return bResult;
}

/**
 *  @brief      Get a print name of a mount point, junction point or symbolic link
 *  @param[in]  sFileName         File name
 *  @param[out] sPrintName        Print name from the reparse buffer
 *  @param[in]  uPrintNameLength  Length of the sPrintName buffer
 *  @return     TRUE if success, FALSE otherwise
 */
REPARSELIB_API BOOL GetPrintName(IN LPCWSTR sFileName, OUT LPWSTR sPrintName, IN USHORT uPrintNameLength)
{
  PREPARSE_DATA_BUFFER pReparse;
  DWORD dwTag;

  if ((NULL == sPrintName) || (0 == uPrintNameLength))
  {
    return FALSE;
  }
  
  if (!ReparsePointExists(sFileName))
  {
    return FALSE;
  }

  if (!GetReparseTag(sFileName, &dwTag))
  {
    return FALSE;
  }
  
  // If not mount point, reparse point or symbolic link
  if ( ! ( (dwTag == IO_REPARSE_TAG_MOUNT_POINT) || (dwTag == IO_REPARSE_TAG_SYMLINK) ) )
  {
    return FALSE;
  }

  pReparse = (PREPARSE_DATA_BUFFER)
    GlobalAlloc(GPTR, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

  if (!GetReparseBuffer(sFileName, (PREPARSE_GUID_DATA_BUFFER)pReparse))
  {
    GlobalFree(pReparse);
    return FALSE;
  }
  
  switch (dwTag)
  {
    case IO_REPARSE_TAG_MOUNT_POINT:
      if (uPrintNameLength >= pReparse->MountPointReparseBuffer.PrintNameLength)
      {
        memcpy
        (
          sPrintName,
          &pReparse->MountPointReparseBuffer.PathBuffer
          [
            pReparse->MountPointReparseBuffer.PrintNameOffset / sizeof(wchar_t)
          ],
          pReparse->MountPointReparseBuffer.PrintNameLength
        );
      } else
      {
        GlobalFree(pReparse);
        return FALSE;
      }
    break;
    case IO_REPARSE_TAG_SYMLINK:
      if (uPrintNameLength >= pReparse->SymbolicLinkReparseBuffer.PrintNameLength)
      {
        memcpy
        (
          sPrintName,
          &pReparse->SymbolicLinkReparseBuffer.PathBuffer
          [
            pReparse->SymbolicLinkReparseBuffer.PrintNameOffset / sizeof(wchar_t)
          ],
          pReparse->SymbolicLinkReparseBuffer.PrintNameLength
        );
      } else
      {
        GlobalFree(pReparse);
        return FALSE;
      }
    break;
  }

  GlobalFree(pReparse);

  return TRUE;
}

/**
 *  @brief      Get a substitute name of a mount point, junction point or symbolic link
 *  @param[in]  sFileName         File name
 *  @param[out] sSubstituteName   Substitute name from the reparse buffer
 *  @param[in]  uSubstituteNameLength  Length of the sSubstituteName buffer
 *  @return     TRUE if success, FALSE otherwise
 */
REPARSELIB_API BOOL GetSubstituteName(IN LPCWSTR sFileName, OUT LPWSTR sSubstituteName, IN USHORT uSubstituteNameLength)
{
  PREPARSE_DATA_BUFFER pReparse;
  DWORD dwTag;

  if ((NULL == sSubstituteName) || (0 == uSubstituteNameLength))
  {
    return FALSE;
  }
  
  if (!ReparsePointExists(sFileName))
  {
    return FALSE;
  }

  if (!GetReparseTag(sFileName, &dwTag))
  {
    return FALSE;
  }
  
  // If not mount point, reparse point or symbolic link
  if ( ! ( (dwTag == IO_REPARSE_TAG_MOUNT_POINT) || (dwTag == IO_REPARSE_TAG_SYMLINK) ) )
  {
    return FALSE;
  }

  pReparse = (PREPARSE_DATA_BUFFER)
    GlobalAlloc(GPTR, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

  if (!GetReparseBuffer(sFileName, (PREPARSE_GUID_DATA_BUFFER)pReparse))
  {
    GlobalFree(pReparse);
    return FALSE;
  }
  
  switch (dwTag)
  {
    case IO_REPARSE_TAG_MOUNT_POINT:
      if (uSubstituteNameLength >= pReparse->MountPointReparseBuffer.SubstituteNameLength)
      {
        memcpy
        (
          sSubstituteName,
          &pReparse->MountPointReparseBuffer.PathBuffer
          [
            pReparse->MountPointReparseBuffer.SubstituteNameOffset / sizeof(wchar_t)
          ],
          pReparse->MountPointReparseBuffer.SubstituteNameLength
        );
      } else
      {
        GlobalFree(pReparse);
        return FALSE;
      }
    break;
    case IO_REPARSE_TAG_SYMLINK:
      if (uSubstituteNameLength >= pReparse->SymbolicLinkReparseBuffer.SubstituteNameLength)
      {
        memcpy
        (
          sSubstituteName,
          &pReparse->SymbolicLinkReparseBuffer.PathBuffer
          [
            pReparse->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(wchar_t)
          ],
          pReparse->SymbolicLinkReparseBuffer.SubstituteNameLength
        );
      } else
      {
        GlobalFree(pReparse);
        return FALSE;
      }
    break;
  }

  GlobalFree(pReparse);

  return TRUE;
}
