@REM $Id: simplest-build.bat,v 1.1 2006/11/08 01:42:12 dds Exp $
@MKDIR bin
@cl /DIBPP_WINDOWS /O2 /Oi /W4 /EHsc /I..\..\core /Fobin\ ..\tests.cpp ..\..\core\all_in_one.cpp advapi32.lib
