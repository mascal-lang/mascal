@echo off
setlocal enableDelayedExpansion
for /f %%i in ('where which') do set WhichPath=%%i

set BasePath=%WhichPath:\usr\bin\which.exe=\mingw64\bin%
set ClangPath=%BasePath%\clang++
set LLVMConfigPath=%BasePath%\llvm-config

set LLVMConfigResult=

%LLVMConfigPath% --cxxflags --link-static --ldflags --system-libs --libs all > llvmconfig.txt
for /f "delims=" %%x in (llvmconfig.txt) do set LLVMConfigResult=!LLVMConfigResult! %%x

echo Compiling Mascal on Windows...
%ClangPath% -g -O3 language/*.cpp *.cpp %LLVMConfigResult% -fstack-protector -lssp -frtti -std=c++20 -static -o mascal

IF "%ERRORLEVEL%"=="0" (
    echo Mascal Compiled Successfully!
) ELSE (
    echo Mascal Compilation Failed...
)

PAUSE