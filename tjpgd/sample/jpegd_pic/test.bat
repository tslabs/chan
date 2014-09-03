pic30-gcc -mcpu=24FJ64GA002 -x c -c %1.c -o%1.o -g -Wall -pedantic -Os
@if ERRORLEVEL 1 goto exit
pic30-objdump -s %1.o
:exit
