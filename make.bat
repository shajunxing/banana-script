@echo off
cl /nologo /O2 /W3 /MD /Femake-msvc.exe make.c && make-msvc.exe %*
set ret=%errorlevel%
del make.obj make-msvc.exe make-msvc.exp make-msvc.lib
:: exit /b %ret% not works with &&, seems && only test last process's exit code, not %errorlevel%, batch is not process, so create a new cmd process and let it exit that code
cmd /c exit %ret%
