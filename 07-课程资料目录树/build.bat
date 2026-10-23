@echo off
chcp 65001 >nul
gcc -std=c11 -Wall -Wextra -Wpedantic -finput-charset=UTF-8 -fexec-charset=UTF-8 -Iinclude -o catalog.exe src\mystr.c src\seqlist.c src\linklist.c src\stack.c src\hashtable.c src\sortsearch.c src\catalog.c src\demo.c src\tests.c src\main.c
if errorlevel 1 exit /b 1
echo build ok
