#!/bin/bash
echo "
A static library is basically a set of object files that were copied into a single file. This single file is the static library. The static file is created with the archiver (ar). "
echo "1--> .c file for the library"
echo "2--> create object .o"
echo "3--> with .o create a static library"
echo ""
echo "We always forget filenames my friend.There you go!"
ls *.c
echo "Type .c (**without .c suffix!) file, followed by [ENTER]:"
echo ""
read source_code
gcc -Wall -c "$source_code.c" -o "$source_code.o"
echo "Created $source_code.o"
echo "Time for static library :)"
echo "Type static library's name(without suffix), followed by [ENTER]:"
echo ""
read library_name
ar rcs "lib$library_name.a" "$source_code.o"
echo "Done with library"
echo "Linking against static library"
echo "Type your main program's .c name (without suffix), followed by [ENTER]:"
echo ""
read main
gcc -Wall -static "$main.c" -L. -l"$library_name" -o $main

echo "Type your main program's .c name (without suffix), followed by [ENTER]:"
echo ""
read main
gcc -Wall -static "$main.c" -L. -l"$library_name" -o $main


