@echo off
cmake --preset debug || exit /b 1
cmake --build --preset debug --parallel || exit /b 1
echo Build OK: Intermediate\debug\bin\MyOpenWorld.exe
