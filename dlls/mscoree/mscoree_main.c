/*
 * Implementation of mscoree.dll
 * Microsoft Component Object Runtime Execution Engine
 *
 * Copyright 2006 Paul Chitescu
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#define COBJMACROS
#include "wine/unicode.h"
#include "wine/library.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "ole2.h"
#include "ocidl.h"
#include "shellapi.h"

#include "initguid.h"
#include "cor.h"
#include "corerror.h"
#include "mscoree.h"
#include "metahost.h"
#include "mscoree_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL( mscoree );

LONG dll_refs = 0;

static BOOL get_install_root(LPWSTR install_dir)
{
    const WCHAR dotnet_key[] = {'S','O','F','T','W','A','R','E','\\','M','i','c','r','o','s','o','f','t','\\','.','N','E','T','F','r','a','m','e','w','o','r','k','\\',0};
    const WCHAR install_root[] = {'I','n','s','t','a','l','l','R','o','o','t',0};

    DWORD len;
    HKEY key;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, dotnet_key, 0, KEY_READ, &key))
        return FALSE;

    len = MAX_PATH;
    if (RegQueryValueExW(key, install_root, 0, NULL, (LPBYTE)install_dir, &len))
    {
        RegCloseKey(key);
        return FALSE;
    }
    RegCloseKey(key);

    return TRUE;
}

HRESULT WINAPI CorBindToRuntimeHost(LPCWSTR pwszVersion, LPCWSTR pwszBuildFlavor,
                                    LPCWSTR pwszHostConfigFile, VOID *pReserved,
                                    DWORD startupFlags, REFCLSID rclsid,
                                    REFIID riid, LPVOID *ppv)
{
    HRESULT ret;
    ICLRRuntimeInfo *info;

    TRACE("(%s, %s, %s, %p, %d, %s, %s, %p)\n", debugstr_w(pwszVersion),
          debugstr_w(pwszBuildFlavor), debugstr_w(pwszHostConfigFile), pReserved,
          startupFlags, debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    *ppv = NULL;

    ret = get_runtime_info(NULL, pwszVersion, pwszHostConfigFile, startupFlags, 0, TRUE, &info);

    if (SUCCEEDED(ret))
    {
        ret = ICLRRuntimeInfo_GetInterface(info, rclsid, riid, ppv);

        ICLRRuntimeInfo_Release(info);
    }

    return ret;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        unload_all_runtimes();
        break;
    }
    return TRUE;
}

BOOL WINAPI _CorDllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    FIXME("(%p, %d, %p): stub\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

__int32 WINAPI _CorExeMain2(PBYTE ptrMemory, DWORD cntMemory, LPWSTR imageName, LPWSTR loaderName, LPWSTR cmdLine)
{
    TRACE("(%p, %u, %s, %s, %s)\n", ptrMemory, cntMemory, debugstr_w(imageName), debugstr_w(loaderName), debugstr_w(cmdLine));
    FIXME("Directly running .NET applications not supported.\n");
    return -1;
}

void WINAPI CorExitProcess(int exitCode)
{
    FIXME("(%x) stub\n", exitCode);
    ExitProcess(exitCode);
}

VOID WINAPI _CorImageUnloading(PVOID imageBase)
{
    TRACE("(%p): stub\n", imageBase);
}

HRESULT WINAPI _CorValidateImage(PVOID* imageBase, LPCWSTR imageName)
{
    TRACE("(%p, %s): stub\n", imageBase, debugstr_w(imageName));
    return E_FAIL;
}

HRESULT WINAPI GetCORSystemDirectory(LPWSTR pbuffer, DWORD cchBuffer, DWORD *dwLength)
{
    ICLRRuntimeInfo *info;
    HRESULT ret;

    TRACE("(%p, %d, %p)!\n", pbuffer, cchBuffer, dwLength);

    if (!dwLength || !pbuffer)
        return E_POINTER;

    ret = get_runtime_info(NULL, NULL, NULL, 0, RUNTIME_INFO_UPGRADE_VERSION, TRUE, &info);

    if (SUCCEEDED(ret))
    {
        *dwLength = cchBuffer;
        ret = ICLRRuntimeInfo_GetRuntimeDirectory(info, pbuffer, dwLength);

        ICLRRuntimeInfo_Release(info);
    }

    return ret;
}

HRESULT WINAPI GetCORVersion(LPWSTR pbuffer, DWORD cchBuffer, DWORD *dwLength)
{
    ICLRRuntimeInfo *info;
    HRESULT ret;

    TRACE("(%p, %d, %p)!\n", pbuffer, cchBuffer, dwLength);

    if (!dwLength || !pbuffer)
        return E_POINTER;

    ret = get_runtime_info(NULL, NULL, NULL, 0, RUNTIME_INFO_UPGRADE_VERSION, TRUE, &info);

    if (SUCCEEDED(ret))
    {
        *dwLength = cchBuffer;
        ret = ICLRRuntimeInfo_GetVersionString(info, pbuffer, dwLength);

        ICLRRuntimeInfo_Release(info);
    }

    return ret;
}

HRESULT WINAPI GetRequestedRuntimeInfo(LPCWSTR pExe, LPCWSTR pwszVersion, LPCWSTR pConfigurationFile,
    DWORD startupFlags, DWORD runtimeInfoFlags, LPWSTR pDirectory, DWORD dwDirectory, DWORD *dwDirectoryLength,
    LPWSTR pVersion, DWORD cchBuffer, DWORD *dwlength)
{
    HRESULT ret;
    ICLRRuntimeInfo *info;
    DWORD length_dummy;

    TRACE("(%s, %s, %s, 0x%08x, 0x%08x, %p, 0x%08x, %p, %p, 0x%08x, %p)\n", debugstr_w(pExe),
          debugstr_w(pwszVersion), debugstr_w(pConfigurationFile), startupFlags, runtimeInfoFlags, pDirectory,
          dwDirectory, dwDirectoryLength, pVersion, cchBuffer, dwlength);

    if (!dwDirectoryLength) dwDirectoryLength = &length_dummy;

    if (!dwlength) dwlength = &length_dummy;

    ret = get_runtime_info(pExe, pwszVersion, pConfigurationFile, startupFlags, runtimeInfoFlags, TRUE, &info);

    if (SUCCEEDED(ret))
    {
        *dwlength = cchBuffer;
        ret = ICLRRuntimeInfo_GetVersionString(info, pVersion, dwlength);

        if (SUCCEEDED(ret))
        {
            *dwDirectoryLength = dwDirectory;
            ret = ICLRRuntimeInfo_GetRuntimeDirectory(info, pDirectory, dwDirectoryLength);
        }

        ICLRRuntimeInfo_Release(info);
    }

    return ret;
}

HRESULT WINAPI LoadLibraryShim( LPCWSTR szDllName, LPCWSTR szVersion, LPVOID pvReserved, HMODULE * phModDll)
{
    HRESULT ret=S_OK;
    WCHAR dll_filename[MAX_PATH];
    WCHAR version[MAX_PATH];
    static const WCHAR default_version[] = {'v','1','.','1','.','4','3','2','2',0};
    static const WCHAR slash[] = {'\\',0};
    DWORD dummy;

    TRACE("(%p %s, %p, %p, %p)\n", szDllName, debugstr_w(szDllName), szVersion, pvReserved, phModDll);

    if (!szDllName || !phModDll)
        return E_POINTER;

    if (!get_install_root(dll_filename))
    {
        ERR("error reading registry key for installroot\n");
        dll_filename[0] = 0;
    }
    else
    {
        if (!szVersion)
        {
            ret = GetCORVersion(version, MAX_PATH, &dummy);
            if (SUCCEEDED(ret))
                szVersion = version;
            else
                szVersion = default_version;
        }
        strcatW(dll_filename, szVersion);
        strcatW(dll_filename, slash);
    }

    strcatW(dll_filename, szDllName);

    *phModDll = LoadLibraryW(dll_filename);

    return *phModDll ? S_OK : E_HANDLE;
}

HRESULT WINAPI LockClrVersion(FLockClrVersionCallback hostCallback, FLockClrVersionCallback *pBeginHostSetup, FLockClrVersionCallback *pEndHostSetup)
{
    FIXME("(%p %p %p): stub\n", hostCallback, pBeginHostSetup, pEndHostSetup);
    return S_OK;
}

HRESULT WINAPI CoInitializeCor(DWORD fFlags)
{
    FIXME("(0x%08x): stub\n", fFlags);
    return S_OK;
}

HRESULT WINAPI GetAssemblyMDImport(LPCWSTR szFileName, REFIID riid, IUnknown **ppIUnk)
{
    FIXME("(%p %s, %s, %p): stub\n", szFileName, debugstr_w(szFileName), debugstr_guid(riid), *ppIUnk);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

HRESULT WINAPI GetVersionFromProcess(HANDLE hProcess, LPWSTR pVersion, DWORD cchBuffer, DWORD *dwLength)
{
    FIXME("(%p, %p, %d, %p): stub\n", hProcess, pVersion, cchBuffer, dwLength);
    return E_NOTIMPL;
}

HRESULT WINAPI LoadStringRCEx(LCID culture, UINT resId, LPWSTR pBuffer, int iBufLen, int bQuiet, int* pBufLen)
{
    HRESULT res = S_OK;
    if ((iBufLen <= 0) || !pBuffer)
        return E_INVALIDARG;
    pBuffer[0] = 0;
    if (resId) {
        FIXME("(%d, %x, %p, %d, %d, %p): semi-stub\n", culture, resId, pBuffer, iBufLen, bQuiet, pBufLen);
        res = E_NOTIMPL;
    }
    else
        res = E_FAIL;
    if (pBufLen)
        *pBufLen = lstrlenW(pBuffer);
    return res;
}

HRESULT WINAPI LoadStringRC(UINT resId, LPWSTR pBuffer, int iBufLen, int bQuiet)
{
    return LoadStringRCEx(-1, resId, pBuffer, iBufLen, bQuiet, NULL);
}

HRESULT WINAPI CorBindToRuntimeEx(LPWSTR szVersion, LPWSTR szBuildFlavor, DWORD nflags, REFCLSID rslsid,
                                  REFIID riid, LPVOID *ppv)
{
    HRESULT ret;
    ICLRRuntimeInfo *info;

    TRACE("%s %s %d %s %s %p\n", debugstr_w(szVersion), debugstr_w(szBuildFlavor), nflags, debugstr_guid( rslsid ),
          debugstr_guid( riid ), ppv);

    *ppv = NULL;

    ret = get_runtime_info(NULL, szVersion, NULL, nflags, 0, TRUE, &info);

    if (SUCCEEDED(ret))
    {
        ret = ICLRRuntimeInfo_GetInterface(info, rslsid, riid, ppv);

        ICLRRuntimeInfo_Release(info);
    }

    return ret;
}

HRESULT WINAPI CorBindToCurrentRuntime(LPCWSTR filename, REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    FIXME("(%s, %s, %s, %p): stub\n", debugstr_w(filename), debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

STDAPI ClrCreateManagedInstance(LPCWSTR pTypeName, REFIID riid, void **ppObject)
{
    FIXME("(%s,%s,%p)\n", debugstr_w(pTypeName), debugstr_guid(riid), ppObject);
    return E_NOTIMPL;
}

BOOL WINAPI StrongNameSignatureVerification(LPCWSTR filename, DWORD inFlags, DWORD* pOutFlags)
{
    FIXME("(%s, 0x%X, %p): stub\n", debugstr_w(filename), inFlags, pOutFlags);
    return FALSE;
}

BOOL WINAPI StrongNameSignatureVerificationEx(LPCWSTR filename, BOOL forceVerification, BOOL* pVerified)
{
    FIXME("(%s, %u, %p): stub\n", debugstr_w(filename), forceVerification, pVerified);
    return FALSE;
}

HRESULT WINAPI CLRCreateInstance(REFCLSID clsid, REFIID riid, LPVOID *ppInterface)
{
    TRACE("(%s,%s,%p)\n", debugstr_guid(clsid), debugstr_guid(riid), ppInterface);

    if (IsEqualGUID(clsid, &CLSID_CLRMetaHost))
        return CLRMetaHost_CreateInstance(riid, ppInterface);

    FIXME("not implemented for class %s\n", debugstr_guid(clsid));

    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    FIXME("(%s, %s, %p): stub\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    if(!ppv)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT WINAPI DllRegisterServer(void)
{
    FIXME("\n");
    return S_OK;
}

HRESULT WINAPI DllUnregisterServer(void)
{
    FIXME("\n");
    return S_OK;
}

HRESULT WINAPI DllCanUnloadNow(VOID)
{
    if (dll_refs)
        return S_FALSE;
    else
        return S_OK;
}

INT WINAPI ND_RU1( const void *ptr, INT offset )
{
    return *((const BYTE *)ptr + offset);
}

INT WINAPI ND_RI2( const void *ptr, INT offset )
{
    return *(const SHORT *)((const BYTE *)ptr + offset);
}

INT WINAPI ND_RI4( const void *ptr, INT offset )
{
    return *(const INT *)((const BYTE *)ptr + offset);
}

INT64 WINAPI ND_RI8( const void *ptr, INT offset )
{
    return *(const INT64 *)((const BYTE *)ptr + offset);
}

void WINAPI ND_WU1( void *ptr, INT offset, BYTE val )
{
    *((BYTE *)ptr + offset) = val;
}

void WINAPI ND_WI2( void *ptr, INT offset, SHORT val )
{
    *(SHORT *)((BYTE *)ptr + offset) = val;
}

void WINAPI ND_WI4( void *ptr, INT offset, INT val )
{
    *(INT *)((BYTE *)ptr + offset) = val;
}

void WINAPI ND_WI8( void *ptr, INT offset, INT64 val )
{
    *(INT64 *)((BYTE *)ptr + offset) = val;
}

void WINAPI ND_CopyObjDst( const void *src, void *dst, INT offset, INT size )
{
    memcpy( (BYTE *)dst + offset, src, size );
}

void WINAPI ND_CopyObjSrc( const void *src, INT offset, void *dst, INT size )
{
    memcpy( dst, (const BYTE *)src + offset, size );
}
