/** File I/O
 * @file
 *
 * This source file is part of the Cone Programming Language C compiler
 * See Copyright Notice in conec.h
*/

#include "fileio.h"
#include "memory.h"

#include <stdio.h>
#include <string.h>
#include <stddef.h>

char **fileSearchPaths = NULL;

/** Load a file into an allocated string, return pointer or NULL if not found */
char *fileLoad(char *fn) {
    FILE *file;
    size_t filesize;
    char *filestr;

    // Open the file - return null on failure
    if (!(file = fopen(fn, "rb")))
        return NULL;

    // Determine the file length (so we can accurately allocate memory)
    fseek(file, 0, SEEK_END);
    filesize=ftell(file);
    fseek(file, 0, SEEK_SET);

    // Load the data into an allocated string buffer and close file
    filestr = memAllocStr(NULL, filesize);
    fread(filestr, 1, filesize, file);
    filestr[filesize]='\0';
    fclose(file);
    return filestr;
}

/** Extract a filename only (no extension) from a path */
char *fileName(char *fn) {
    char *dotp;
    char *fnp = &fn[strlen(fn)-1];

    // Look backwards for '.' If not found, we are done
    while (fnp != fn && *fnp != '.' && *fnp != '/' && *fnp != '\\')
        --fnp;
    if (fnp == fn)
        return fn;
    if (*fnp == '/' || *fnp == '\\')
        return fnp + 1;

    // Look backwards for slash
    dotp = fnp;
    while (fnp != fn && *fnp != '/' && *fnp != '\\')
        --fnp;
    if (fnp != fn)
        ++fnp;

    // Create string to hold filename and return
    return memAllocStr(fnp, dotp-fnp);
}

/** Concatenate folder, filename and extension into a path */
char *fileMakePath(char *dir, char *srcfn, char *ext) {
    char *outnm;
    if (dir == NULL)
        dir = "";
    outnm = memAllocStr(dir, strlen(dir) + strlen(srcfn) + strlen(ext) + 2);
    if (strlen(dir) && outnm[strlen(outnm) - 1] != '/' && outnm[strlen(outnm) - 1] != '\\')
        strcat(outnm, "/");
    strcat(outnm, srcfn);
    strcat(outnm, ".");
    strcat(outnm, ext);
    return outnm;
}

// Get number of characters in string up to file name
size_t fileFolder(char *fn) {
    char *fnp = &fn[strlen(fn) - 1];

    // Look backwards for '/' If not found, we are done
    while (fnp != fn && *fnp != '/' && *fnp != '\\')
        --fnp;
    if (fnp == fn)
        return 0;
    return fnp - fn + 1;
}

// Return position of last period (after last slash)
char *fileExtPos(char *fn) {
    char *dotpos = strrchr(fn, '.');
    return dotpos && dotpos > strrchr(fn, '/') ? dotpos : 0;
}

// Return position of filename (after last slash)
char *fileNamePos(char *fn) {
    char *slashpos = strrchr(fn, '/');
    return slashpos? slashpos + 1 : fn;
}

// Create a new source file url relative to current, substituting new path and .cone extension
char *fileSrcUrl(char *cururl, char *srcfn, int newfolder) {
    if (cururl == NULL)
        cururl = "";
    char *extp = fileExtPos(srcfn);
    char *fnamep = fileNamePos(srcfn);

    // Calculate how large composed string needs to be, then allocate space
    size_t outnmsz = strlen(cururl) + strlen(srcfn) + 1;
    if (newfolder)
        outnmsz += strlen(fnamep) + 1;
    if (!extp)
        outnmsz += strlen(".cone");
    char *outnm = memAllocStr("", outnmsz);

    // Compose full file path
    if (cururl && srcfn[0]!='/')
        strncat(outnm, cururl, fileFolder(cururl));
    strcat(outnm, srcfn);
    if (newfolder) {
        // Look for file inside name as folder
        if (extp)
            *fileExtPos(outnm) = 0;
        strcat(outnm, "/");
        strcat(outnm, fnamep);
    }
    if (!extp)
        strcat(outnm, ".cone");
    return outnm;
}

// Load source file, where srcfn is relative to cururl
// - Look at fn+.cone or fn+/fn.cone
// - return full pathname for source file
char *fileLoadSrcWithFolder(char *cururl, char *srcfn, char **fn) {
    char *src;
    *fn = fileSrcUrl(cururl, srcfn, 0);
    if ((src = fileLoad(*fn)))
        return src;
    *fn = fileSrcUrl(cururl, srcfn, 1);
    return fileLoad(*fn);
}

// Search for and load source file, where srcfn is relative to cururl
// - Use search paths
// - Look at fn+.cone or fn+/mod.cone
// - return full pathname for source file
char *fileLoadSrc(char *cururl, char *srcfn, char **fn) {
    char *src;
    if ((src = fileLoadSrcWithFolder(cururl, srcfn, fn)))
        return src;
    char **searchPaths = fileSearchPaths;
    if (searchPaths == NULL)
        return NULL;
    while (*searchPaths) {
        if ((src = fileLoadSrcWithFolder(*searchPaths++, srcfn, fn)))
            return src;
    }
    return NULL;
}
