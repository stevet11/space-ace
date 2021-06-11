#!/usr/bin/env python3

import sys

##
## Convert ANSI art files to C++ header file.
##

filenames = sys.argv
filenames.pop(0)

if not filenames:
    print("I need filename(s) to convert.")
    sys.exit(0)



def my_repr(line):
    """ Given a latin1 line, output valid C++ string.

    if the character is < 0x20 or > 0x7e, output in hex format.
    if the character is ', " or ?, output \', \", or \?.

    """
    r = ""
    for c in line:
        o = ord(c)
        if ((o< 0x20) or (o > 0x7e)):
            r += "\\x{0:02x}".format(o)
        else:
            if c == "'":
                r += "\\'"
            elif c == '"':
                r += '\\"'
            elif c == '?':
                r += '\\?'
            else:
                r += c
    return r    

def readfile(filename):
    """ Reads the given ANSI file and outputs valid C++ code. """

    # basename will be the name of the array defined in the headerfile.
    basename = filename
    pos = basename.find('.')
    if pos != -1:
        basename = basename[:pos]
    pos = basename.rfind('/')
    if pos != -1:
        basename = basename[pos+1:]
    
    # Read the file into lines
    lines = []
    with open(filename, "r", encoding="latin1") as fp:
        for line in fp:
            line = line.rstrip("\r\n")
            lines.append(line)

    print("std::array<const char *,{1}> {0} = {{".format(basename, len(lines)))
    first = True
    for line in lines:
        if not first:
            print(",")
        else:
            first = False
        print("    \"{0}\"".format(my_repr(line)), end='')
    print(" };\n")

# Begin the output process
print("#include <array>\n")

# Process each file given
for filename in filenames:
    readfile(filename)


