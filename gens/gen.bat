call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Enterprise\MSBuild\15.0\Bin\amd64\MSBuild.exe"
call "pkggen\bin\x64\Debug\pkggen.exe"
xcopy output\PKG_TypeIdMappings.cs pkggen_template_PKG\ /y
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Enterprise\MSBuild\15.0\Bin\amd64\MSBuild.exe"
call "pkggen\bin\x64\Debug\pkggen.exe"