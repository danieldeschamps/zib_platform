@REM $Id: simplest-build.bat,v 1.1 2006/11/08 01:42:11 dds Exp $
@MKDIR bin
@bcc32 -DIBPP_WINDOWS -O2 -w -I..\..\core -nbin ..\tests.cpp ..\..\core\all_in_one.cpp
