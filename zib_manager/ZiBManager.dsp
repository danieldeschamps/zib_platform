# Microsoft Developer Studio Project File - Name="ZiBManager" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ZiBManager - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ZiBManager.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ZiBManager.mak" CFG="ZiBManager - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ZiBManager - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ZiBManager - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ZiBManager - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".\\" /I "..\\" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "IBPP_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x416 /d "NDEBUG"
# ADD RSC /l 0x416 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib advapi32.lib ws2_32.lib /nologo /machine:I386 /out:"C:\ZiBManager\ZiBManager.exe"

!ELSEIF  "$(CFG)" == "ZiBManager - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /GR /GX /ZI /Od /I ".\\" /I "..\\" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "IBPP_WINDOWS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x416 /d "_DEBUG"
# ADD RSC /l 0x416 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib advapi32.lib ws2_32.lib /nologo /debug /machine:I386 /out:"C:\ZiBManager\ZiBManager.exe" /pdbtype:sept
# SUBTRACT LINK32 /incremental:no

!ENDIF 

# Begin Target

# Name "ZiBManager - Win32 Release"
# Name "ZiBManager - Win32 Debug"
# Begin Group "src"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Alarms.cpp
# End Source File
# Begin Source File

SOURCE=.\Main.cpp
# End Source File
# Begin Source File

SOURCE=.\ModemEricsson.cpp
# End Source File
# Begin Source File

SOURCE=.\Stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\SystemService.cpp
# End Source File
# Begin Source File

SOURCE=.\ZiBDb.cpp
# End Source File
# Begin Source File

SOURCE=.\ZiBGatewayComm.cpp
# End Source File
# Begin Source File

SOURCE=.\ZiBManager.cpp
# End Source File
# Begin Source File

SOURCE=.\ZiBManagerRegistry.cpp
# End Source File
# End Group
# Begin Group "h"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Alarms.h
# End Source File
# Begin Source File

SOURCE=.\ModemEricsson.h
# End Source File
# Begin Source File

SOURCE=.\Stdafx.h
# End Source File
# Begin Source File

SOURCE=.\SystemService.h
# End Source File
# Begin Source File

SOURCE=.\ZiBDb.h
# End Source File
# Begin Source File

SOURCE=.\ZiBDiscovery.h
# End Source File
# Begin Source File

SOURCE=.\ZiBGatewayComm.h
# End Source File
# Begin Source File

SOURCE=.\ZiBManager.h
# End Source File
# Begin Source File

SOURCE=..\common\ZiBManagerGatewayProtocol.h
# End Source File
# Begin Source File

SOURCE=.\ZiBManagerRegistry.h
# End Source File
# Begin Source File

SOURCE=..\zib_network\ZigBeeStack\zZiBPlatform.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "ibpp"

# PROP Default_Filter ""
# Begin Group "core"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\ibpp\core\_dpb.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\_ibpp.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\_ibs.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\_rb.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\_spb.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\_tpb.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\array.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\blob.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\database.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\date.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\dbkey.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\events.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\exception.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\row.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\service.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\statement.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\time.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\transaction.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\user.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Group "header"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\ibpp\core\_ibpp.h
# End Source File
# Begin Source File

SOURCE=.\ibpp\core\ibpp.h
# End Source File
# End Group
# End Group
# Begin Group "serial"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\serial\ATComm.cpp
# End Source File
# Begin Source File

SOURCE=.\serial\ATComm.h
# End Source File
# Begin Source File

SOURCE=.\serial\Serial.cpp
# End Source File
# Begin Source File

SOURCE=.\serial\Serial.h
# End Source File
# Begin Source File

SOURCE=.\serial\SerialEx.cpp
# End Source File
# Begin Source File

SOURCE=.\serial\SerialEx.h
# End Source File
# End Group
# Begin Group "misc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\misc\CriticalSection.cpp
# End Source File
# Begin Source File

SOURCE=.\misc\CriticalSection.h
# End Source File
# Begin Source File

SOURCE=.\misc\Debug.cpp
# End Source File
# Begin Source File

SOURCE=.\misc\Debug.h
# End Source File
# Begin Source File

SOURCE=.\misc\IbppTest.cpp
# End Source File
# Begin Source File

SOURCE=.\misc\IbppTest.h
# End Source File
# End Group
# Begin Group "modem"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\modem\Modem.cpp
# End Source File
# Begin Source File

SOURCE=.\modem\Modem.h
# End Source File
# Begin Source File

SOURCE=.\modem\Pdu.cpp
# End Source File
# Begin Source File

SOURCE=.\modem\Pdu.h
# End Source File
# Begin Source File

SOURCE=.\modem\PduConversion.cpp
# End Source File
# Begin Source File

SOURCE=.\modem\PduConversion.h
# End Source File
# End Group
# Begin Group "registry"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\registry\RegEntry.h
# End Source File
# Begin Source File

SOURCE=.\registry\Registry.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\registry\Registry.h
# End Source File
# End Group
# Begin Group "email"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\email\FastSmtp.cpp
# End Source File
# Begin Source File

SOURCE=.\email\FastSmtp.h
# End Source File
# End Group
# End Target
# End Project
