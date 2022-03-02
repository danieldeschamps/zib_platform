// Main.cpp : Defines the entry point for the console application.
//

// original
#include "Stdafx.h"
#include "SystemService.h"
#include "ModemEricsson.h"
#include <modem/Modem.h>

int main(int argc, char* argv[])
{
	_Module.Init("ZiBManager","ZiB Manager");
	
	// Argument handler
	if (argc > 1)
	{
		char seps[] = "-/";
		char *pToken;
		
		pToken = strtok(argv[1],seps);
		while (pToken)
		{
			if (!stricmp(pToken,"?"))
			{
				printf("ZiB Manager WIN32 System Service - Command line help\n\n");
				printf("Syntax: ZiBManager.exe [option]\n\n");
				printf("Options:    ?           Help\n");
				printf("            install     Installs the service at SCM, creates the registry key\n");
				printf("                        and creates the database\n");
				printf("            uninstall   Removes the service from SCM, deletes the registry key\n");
				printf("                        and deletes the database\n");
			}
			if (!stricmp(pToken,"install"))
			{
				_Module.Install();
			}
			else if (!stricmp(pToken,"uninstall"))
			{
				_Module.Uninstall();
			}
			else if (!stricmp(pToken,"rebuilddb"))
			{
				CZiBManager manager;
				if (manager.Uninstall(CZiBManager::EMangerInstallComponentsDatabase)
					 && manager.Install(CZiBManager::EMangerInstallComponentsDatabase))
				{
					printf("Successfully rebuilt the database\n");
				}
				else
				{
					printf("Failed to rebuild the database\n");
				}
			}
			else if (!stricmp(pToken,"rebuildreg"))
			{
				CZiBManager manager;
				if (manager.Uninstall(CZiBManager::EMangerInstallComponentsRegistry)
					&& manager.Install(CZiBManager::EMangerInstallComponentsRegistry))
				{
					printf("Successfully rebuilt the registry\n");
				}
				else
				{
					printf("Failed to rebuild the registry\n");
				}
			}

			else if (!stricmp(pToken,"testgateway"))
			{
				bool bExit = false;
				while (!bExit)
				{
					// The service is running.
					 CZiBManager manager;
					// todo:configure the serial port properly.
					manager.StartSerialPort ("COM1", 
						CSerial::EBaud115200,
						CSerial::EData8,
						CSerial::EParNone,
						CSerial::EStop1,
						CSerial::EHandshakeOff);
					if (manager.Run() == CZiBManager::EMangerReturnActionExit)
					{
						bExit = true;
					}
				}
			}
			else if (!stricmp(pToken,"testmodem"))
			{
				CModem* modem;

				pToken = strtok(argv[2],seps);
				string strModemChoice (pToken);
				if (strModemChoice.find("ericsson") != string::npos)
				{
					modem = new CModemEricsson;

				}
				else if (strModemChoice.find("generic") != string::npos)
				{
					modem = new CModem;
				}
				else
				{
					printf("ERROR: Unknown modem type\n");
					return 0;
				}

				pToken = strtok(argv[3],seps);
				if( modem->SetupSerialPort(	strupr(pToken), 
											CSerial::EBaud19200,
											CSerial::EData8,
											CSerial::EParNone,
											CSerial::EStop1,
											CSerial::EHandshakeOff ) )
				{
					if ( modem->TestCommunication() )
					{
						TRACE(DBG_LVL_INFO, "Modem is ONLINE\n");
						if( modem->InitializationCommands() )
						{
							modem->SendSms("554199131143", "Mensagem de teste");
						}
						else
						{
							TRACE(DBG_LVL_WARNING, "Error while sending initialization commands to the modem.\n");
						}
					}
					else
					{
						TRACE(DBG_LVL_ERROR, "Modem is OFFLINE\n");
					}
				}

				delete modem;
				
			}

			pToken = strtok( NULL, seps );
			return 0;
		}
	}

	_Module.m_bService = TRUE;
	_Module.Start();

	return 0;
}
