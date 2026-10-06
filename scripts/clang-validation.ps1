$ErrorActionPreference='Stop'
$compilerRoot='C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231'
$sdkRoot='C:\Program Files (x86)\Windows Kits\10'
$env:PATH="$compilerRoot\bin\Hostx64\x64;C:\Program Files\LLVM\bin;"+$env:PATH
$env:INCLUDE="$compilerRoot\include;$sdkRoot\Include\10.0.26100.0\ucrt;$sdkRoot\Include\10.0.26100.0\um;$sdkRoot\Include\10.0.26100.0\shared"
$env:LIB="$compilerRoot\lib\x64;$sdkRoot\Lib\10.0.26100.0\ucrt\x64;$sdkRoot\Lib\10.0.26100.0\um\x64"
$argsList=@('--checks=clang-analyzer-*,bugprone-*,performance-*,portability-*','--warnings-as-errors=*','src/core.cpp','src/main.cpp','src/windows.cpp','tests/tests.cpp','tests/fixture.cpp','--','-std=c++17','-fexceptions','-fcxx-exceptions','-target','x86_64-pc-windows-msvc','-DUNICODE','-D_UNICODE','-DNOMINMAX','-DWIN32_LEAN_AND_MEAN','-Isrc')
foreach($includeDir in $env:INCLUDE.Split(';')) {$argsList+=@('-isystem',$includeDir)}
& clang-tidy @argsList 2>&1 | Tee-Object artifacts/clang-tidy.txt
if($LASTEXITCODE -ne 0) {throw 'clang-tidy failed'}
cmake -S . -B build-clang -G Ninja -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_BUILD_TYPE=Release
if($LASTEXITCODE -ne 0) {throw 'clang configure failed'}
cmake --build build-clang
if($LASTEXITCODE -ne 0) {throw 'clang build failed'}
ctest --test-dir build-clang --output-on-failure
if($LASTEXITCODE -ne 0) {throw 'clang tests failed'}


