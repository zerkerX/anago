/*
	Win32 GIVEIO(DirectI/O Access) Service Controll Module
*/

#include "giveio.h"
#include <windows.h>
#include <stdio.h>

/**-----------------------------------------------------**/

/*
+--------------------+---------+----+----+----+----+--------------------+
|Products            |Version  |PFID|MJRV|MNRV|BNUM| note
+--------------------+---------+----+----+----+----+--------------------+
|Windows 95          |4.00.950 |  1 |  4 |  0 | 950|(Windows 95 + SP1)  |
|Windows 95 OSR 2    |4.00.950B|  1 |  4 |  0 |1111|(Windows 95 OSR 2.1)|
|Windows 95 OSR 2.5  |4.00.950C|  1 |  4 |  0 |1212|                    |
|Windows 98          |4.10.1998|  1 |  4 | 10 |1998|                    |
|Windows 98 SE       |4.10.2222|  1 |  4 | 10 |2222|                    |
|Windows Me          |4.90.3000|  1 |  4 | 90 |3000|                    |
|Windows NT 4.0      |4.00.1381|  2 |  4 |  0 |1381|                    |
|Windows 2000        |5.00.2195|  2 |  5 |  0 |2195|                    |
|Windows XP          |5.01.2600|  2 |  5 |  1 |2600|                    |
+--------------------+---------+----+----+----+----+--------------------+

PFID = dwPlatformId
MJRV = dwMajorVersion
MNRV = dwMinorVersion
BNUM = dwBuildNumber
*/

/* check the OS which GIVEIO should be necessary */
static int is_winnt(void)
{
	OSVERSIONINFO osvi;

	osvi.dwOSVersionInfoSize = sizeof(osvi);
	if( !GetVersionEx(&osvi) )
		return 0;

	switch (osvi.dwPlatformId)
	{
	case VER_PLATFORM_WIN32_NT:
		return 1;
	case VER_PLATFORM_WIN32_WINDOWS:
		return 0;
	case VER_PLATFORM_WIN32s:
		return 0;
	}

	/* unknown */
	return 0;
}

/**-----------------------------------------------------**/

/*  connect to local service control manager */
static SC_HANDLE hSCMan = NULL;
static int OpenServiceControlManager(void)
{
	if(hSCMan==NULL)
	{
		/*  connect to local service control manager */
		if ((hSCMan = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)) == NULL) 
			return GetLastError();
	}
	return 0;
}

static void ClseServiceControlManager(void)
{
	if(hSCMan==NULL)
		CloseServiceHandle(hSCMan);
	hSCMan = NULL;
}

/**-----------------------------------------------------**/
/* #define MAX_PATH 256 */
static DWORD DriverInstall(LPCTSTR lpFname,LPCTSTR lpDriver)
{
	BOOL dwStatus = 0;
	SC_HANDLE hService = NULL;
	char *address;
	char fullpath[MAX_PATH];

	/*  connect to local service control manager */
	if(OpenServiceControlManager())
		return 1;

	/* get full path of driver file in current directry */
	GetFullPathName(lpFname,MAX_PATH,fullpath,&address);

	/* add to service control manager's database */
	if ((hService = CreateService(hSCMan, lpDriver, 
		lpDriver, SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, fullpath, 
		NULL, NULL, NULL, NULL, NULL)) == NULL)
		dwStatus = GetLastError();
	else CloseServiceHandle(hService);

	return dwStatus;
}

static DWORD DriverStart(LPCTSTR lpDriver)
{
	BOOL dwStatus = 0;
	SC_HANDLE hService = NULL;

	/*  connect to local service control manager */
	if(OpenServiceControlManager())
		return 1;

	// get a handle to the service
	if ((hService = OpenService(hSCMan, lpDriver, SERVICE_ALL_ACCESS)) != NULL) 
	{
		// start the driver
		if (!StartService(hService, 0, NULL))
			dwStatus = GetLastError();
	} else dwStatus = GetLastError();

	if (hService != NULL) CloseServiceHandle(hService);

	return dwStatus;
} // DriverStart

/**-----------------------------------------------------**/
static DWORD DriverStop(LPCTSTR lpDriver)
{
   BOOL dwStatus = 0;
   SC_HANDLE hService = NULL;
   SERVICE_STATUS serviceStatus;

	/*  connect to local service control manager */
	if(OpenServiceControlManager())
		return 1;


   // get a handle to the service
   if ((hService = OpenService(hSCMan, lpDriver, 
      SERVICE_ALL_ACCESS)) != NULL) 
   {
      // stop the driver
      if (!ControlService(hService, SERVICE_CONTROL_STOP,
         &serviceStatus))
            dwStatus = GetLastError();
   } else dwStatus = GetLastError();

   if (hService != NULL) CloseServiceHandle(hService);
   return dwStatus;
} // DriverStop

/**-----------------------------------------------------**/
static DWORD DriverRemove(LPCTSTR lpDriver)
{
   BOOL dwStatus = 0;
   SC_HANDLE hService = NULL;

	/*  connect to local service control manager */
	if(OpenServiceControlManager())
		return 1;

   // get a handle to the service
   if ((hService = OpenService(hSCMan, lpDriver, 
      SERVICE_ALL_ACCESS)) != NULL) 
   {
      // remove the driver
      if (!DeleteService(hService))
         dwStatus = GetLastError();
   } else dwStatus = GetLastError();

   if (hService != NULL) CloseServiceHandle(hService);
   return dwStatus;
} // DriverRemove

/*****************************************************************************/

const char giveio_dname[] = "\\\\.\\giveio"; /* device name      */
const char giveio_fname[] = "giveio.sys";    /* driver-file name */
const char giveio_sname[] = "giveio";        /* service name     */

/*
open & close "giveio" device for Direct I/O Access

1st try : open giveio 
2nd try : start service & open
3rd try : install current "giveio.sys" file & start & open
*/

int giveio_start(void)
{
	HANDLE h;
	int res;

	/* escape when Win95/98/Me */
	if(!is_winnt())
		return GIVEIO_WIN95;


	/* WinNT , try GIVEIO */
	res = GIVEIO_OPEN;
	while(1)
	{
		if(res>=GIVEIO_INSTALL)
		{
/* printf("Install\n"); */
			/* Install "GIVEIO.SYS" file in currect directry */
			if( DriverInstall(giveio_fname,giveio_sname) )
			{
				/* if install error , no retry */
				break;
			}
		}
		if(res>=GIVEIO_START)
		{
/* printf("Start\n"); */
			/* Start GIVEIO Service */
			if( DriverStart(giveio_sname) )
			{
				/* error , retry with install */
				res ++;
				continue;
			}
		}
/* printf("Open\n"); */
		/* open driver */
		h = CreateFile(giveio_dname, GENERIC_READ, 0, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(h != INVALID_HANDLE_VALUE)
		{
			/* OK */
			CloseHandle(h);
			return res;
		}
		/* error , retry with Start,Install */
		res++;
	}

	/* error */
	return GIVEIO_ERROR;
}

int giveio_stop(int param)
{
	/* escape when Win95/98/Me */
	if(!is_winnt())
		return 0;

	if(param>=GIVEIO_STOP)
	{
/* printf("Stop\n"); */
		if( DriverStop(giveio_sname) )
			return param;
		if(param>=GIVEIO_REMOVE)
		{
/* printf("Remove\n"); */
			if(DriverRemove(giveio_sname) )
				return param;
		}
	}

	ClseServiceControlManager();

	/* Okay */
	return 0;
}

