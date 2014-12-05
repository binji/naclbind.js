#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <nacl_io/nacl_io.h>

/* naclbind-gen:
    -B cfree
    -B dtoa
    -B fstatat
    -B .*lockfile
    -B .*mktemp.*
    -B _mstats_r
    -B renameat
    -B _rename_r
    -B strtold
    -B .*wc.*
 */
