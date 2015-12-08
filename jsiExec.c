#ifndef JSI_LITE_ONLY
/*
 * (c) 2008 Steve Bennett <steveb@workware.net.au>
 *
 * Implements the exec command for Jsi
 *
 * Based on code originally from Tcl 6.7 by John Ousterhout.
 * From that code:
 *
 * The Tcl_Fork and Tcl_WaitPids procedures are based on code
 * contributed by Karl Lehenbauer, Mark Diekhans and Peter
 * da Silva.
 *
 * Copyright 1987-1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#include <string.h>
#include <ctype.h>

#ifndef JSI_AMALGAMATION
#include "jsiInt.h"
#endif

static void *jsi_ExecCmdData(Jsi_Interp *interp);

#define HAVE_VFORK
#define HAVE_WAITPID

/* Simple implementation of exec with system().  Returns return code.
 * The system() call *may* do command line redirection, etc.
 * The standard output is not available.
 * Can't redirect filehandles.
 */
int Jsi_SysExecCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_DString *rStr)
{
    int i, j, len, rc = 0;
    Jsi_DString dStr, eStr;
    len = Jsi_ValueGetLength(interp, args);
    Jsi_DSInit(rStr);
    if (len<=0)
        return 0;
    Jsi_DSInit(&dStr);
#ifdef __WIN32
    Jsi_DSAppendLen(&dStr, "\"", 1);
#endif
    for (i=0; i<len; i++) {
        Jsi_Value *v = Jsi_ValueArrayIndex(interp, args, i);
        const char *arg = Jsi_ValueGetDString(interp, v, &eStr, 0);
        if (i>1)
        Jsi_DSAppendLen(&dStr, " ", 1);
        if (strpbrk(arg, "\\\" ") == NULL) {
            Jsi_DSAppend(&dStr, arg, NULL);
        } else {
            Jsi_DSAppendLen(&dStr, "\"", 1);
            for (j = 0; j < len; j++) {
                if (arg[j] == '\\' || arg[j] == '"') {
                    Jsi_DSAppendLen(&dStr, "\\", 1);
                }
                Jsi_DSAppendLen(&dStr, &arg[j], 1);
            }
            Jsi_DSAppendLen(&dStr, "\"", 1);
        }
        Jsi_DSFree(&eStr);

    }
#ifdef __WIN32
    Jsi_DSAppendLen(&dStr, "\"", 1);
#endif
    rc = system(Jsi_DSValue(&dStr));
    Jsi_DSFree(&dStr);
    return rc;
}

#if (!defined(HAVE_VFORK) || !defined(HAVE_WAITPID)) && !defined(__MINGW32__)
/* Poor man's implementation of exec with system()
 * The system() call *may* do command line redirection, etc.
 * The standard output is not available.
 * Can't redirect filehandles.
 */
static int Jsi_ExecCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_DString *rStr)
{
    Jsi_SysExecCmd(interp, args, rStr);
    return JSI_OK;
}

#else
/* Full exec implementation for unix and mingw */

#include <errno.h>
#include <signal.h>

#if defined(__MINGW32__)
    /* XXX: Should we use this implementation for cygwin too? msvc? */
    #ifndef STRICT
    #define STRICT
    #endif
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <fcntl.h>

    typedef HANDLE fdtype;
    typedef HANDLE pidtype;
    #define JSI_BAD_FD INVALID_HANDLE_VALUE
    #define JSI_BAD_PID INVALID_HANDLE_VALUE
    #define JsiCloseFd CloseHandle

    #define WIFEXITED(STATUS) 1
    #define WEXITSTATUS(STATUS) (STATUS)
    #define WIFSIGNALED(STATUS) 0
    #define WTERMSIG(STATUS) 0
    #define WNOHANG 1

    static fdtype JsiFileno(FILE *fh);
    static pidtype JsiWaitPid(pidtype pid, int *status, int nohang);
    static fdtype JsiDupFd(fdtype infd);
    static fdtype JsiOpenForRead(Jsi_Interp *interp, const char *filename);
    static FILE *JsiFdOpenForRead(fdtype fd);
    static int JsiPipe(fdtype pipefd[2]);
    static pidtype JsiStartWinProcess(Jsi_Interp *interp, char **argv, char *env,
        fdtype inputId, fdtype outputId, fdtype errorId);
    static int JsiErrno(void);
#else
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/wait.h>

    typedef int fdtype;
    typedef int pidtype;
    #define JsiPipe pipe
    #define JsiErrno() errno
    #define JSI_BAD_FD -1
    #define JSI_BAD_PID -1
    #define JsiFileno fileno
    #define JsiReadFd read
    #define JsiCloseFd close
    #define JsiWaitPid waitpid
    #define JsiDupFd dup
    #define JsiFdOpenForRead(FD) fdopen((FD), "r")
    /*#define JsiOpenForRead(NAME) open((NAME), O_RDONLY, 0)*/

    #ifndef HAVE_EXECVPE
        #define execvpe(ARG0, ARGV, ENV) execvp(ARG0, ARGV)
    #endif
#endif

static const char *JsiStrError(void);
static char **JsiSaveEnv(char **env);
static void JsiRestoreEnv(char **env);
static int JsiCreatePipeline(Jsi_Interp *interp, Jsi_Value *args, int argc,
    pidtype **pidArrayPtr, fdtype *inPipePtr, fdtype *outPipePtr, fdtype *errFilePtr);
static void JsiDetachPids(Jsi_Interp *interp, int numPids, const pidtype *pidPtr);
static int JsiCleanupChildren(Jsi_Interp *interp, int numPids, pidtype *pidPtr, fdtype errorId, Jsi_DString *dStr, Jsi_DString *cStr, int *code);
static fdtype JsiCreateTemp(Jsi_Interp *interp, const char *contents);
static fdtype JsiOpenForWrite(Jsi_Interp *interp, const char *filename, int append);
static fdtype JsiOpenForRead(Jsi_Interp *interp, const char *filename);
static int JsiRewindFd(fdtype fd);

static void Jsi_SetResultErrno(Jsi_Interp *interp, const char *msg)
{
    Jsi_LogError("%s: %s", msg, JsiStrError());
}

static const char *JsiStrError(void)
{
    return strerror(JsiErrno());
}

static void Jsi_RemoveTrailingNewline(Jsi_DString *dStr)
{
    char *s = Jsi_DSValue(dStr);
    int len = strlen(s);

    if (len > 0 && s[len - 1] == '\n') {
        s[len-1] = 0;
        Jsi_DSSetLength(dStr, len-1);
    }
}

/**
 * Read from 'fd', append the data to strObj and close 'fd'.
 * Returns JSI_OK if OK, or JSI_ERROR on error.
 */
static int JsiAppendStreamToString(Jsi_Interp *interp, fdtype fd, Jsi_DString *dStr)
{
    char buf[256];
    FILE *fh = JsiFdOpenForRead(fd);
    if (fh == NULL) {
        return JSI_ERROR;
    }

    while (1) {
        int retval = fread(buf, 1, sizeof(buf), fh);
        if (retval > 0) {
            Jsi_DSAppendLen(dStr, buf, retval);
        }
        if (retval != sizeof(buf)) {
            break;
        }
    }
    Jsi_RemoveTrailingNewline(dStr);
    fclose(fh);
    return JSI_OK;
}

static char **JsiBuildEnv(Jsi_Interp *interp)
{
    /* TODO: causing crash. */
    /*return Jsi_GetEnviron();*/
    return NULL;
}

struct WaitInfo
{
    pidtype pid;                    /* Process id of child. */
    int status;                 /* Status returned when child exited or suspended. */
    int flags;                  /* Various flag bits;  see below for definitions. */
};

struct WaitInfoTable {
    struct WaitInfo *info;
    int size;
    int used;
};

/*
 * Flag bits in WaitInfo structures:
 *
 * WI_DETACHED -        Non-zero means no-one cares about the
 *                      process anymore.  Ignore it until it
 *                      exits, then forget about it.
 */

#define WI_DETACHED 2

#define WAIT_TABLE_GROW_BY 4

static int JsiFreeWaitInfoTable(Jsi_Interp *interp, void *privData)
{
    struct WaitInfoTable *table = privData;

    Jsi_Free(table->info);
    Jsi_Free(table);
    return JSI_OK;
}

static struct WaitInfoTable *JsiAllocWaitInfoTable(void)
{
    struct WaitInfoTable *table = Jsi_Calloc(1, sizeof(*table));
    table->info = NULL;
    table->size = table->used = 0;

    return table;
}

/*
 * The main exec command
 */
int jsi_execCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_DString *dStr, Jsi_DString *cStr, int *code)
{
    fdtype outputId;               /* File id for output pipe.  -1
                                 * means command overrode. */
    fdtype errorId;                /* File id for temporary file
                                 * containing error output. */
    pidtype *pidPtr;
    int numPids, result, argc;
    
    argc = Jsi_ValueGetLength(interp, args);
    /*
     * See if the command is to be run in background;  if so, create
     * the command, detach it, and return.
     */

    if (argc > 1 && Jsi_Strcmp(Jsi_ValueArrayIndexToStr(interp, args, argc-1, NULL), "&") == 0) {
        int i;

        argc--;
        numPids = JsiCreatePipeline(interp, args, argc, &pidPtr, NULL, NULL, NULL);
        if (numPids < 0) {
            return JSI_ERROR;
        }
        if (cStr) {
            /* The return value is a list of the pids */
            Jsi_DSAppend(cStr,(Jsi_DSLength(cStr)>1?", ":""), "pids : [", NULL);
            for (i = 0; i < numPids; i++) {
                char ibuf[100];
                sprintf(ibuf, "%s%ld", (i?",":""), (long)pidPtr[i]);
                Jsi_DSAppend(cStr, ibuf, NULL);
            }
            Jsi_DSAppend(cStr, "]", NULL);
        }
        JsiDetachPids(interp, numPids, pidPtr);
        Jsi_Free(pidPtr);
        return JSI_OK;
    }
    /*
     * Create the command's pipeline.
     */
    numPids =
        JsiCreatePipeline(interp, args, argc, &pidPtr, NULL, &outputId, &errorId);

    if (numPids < 0) {
        return JSI_ERROR;
    }

    /*
     * Read the child's output (if any) and put it into the result.
     */

    result = JSI_OK;
    if (outputId != JSI_BAD_FD) {
        result = JsiAppendStreamToString(interp, outputId, dStr);
        if (result != JSI_OK && cStr)
            Jsi_DSAppend(cStr, (Jsi_DSLength(cStr)>1?", ":""), "errstr: \"error reading from output pipe\"", NULL);
    }

    if (JsiCleanupChildren(interp, numPids, pidPtr, errorId, dStr, cStr, code) != JSI_OK) {
        result = JSI_ERROR;
    }
    return result;
}

static void JsiReapDetachedPids(struct WaitInfoTable *table)
{
    struct WaitInfo *waitPtr;
    int count;

    if (!table) {
        return;
    }

    for (waitPtr = table->info, count = table->used; count > 0; waitPtr++, count--) {
        if (waitPtr->flags & WI_DETACHED) {
            int status;
            pidtype pid = JsiWaitPid(waitPtr->pid, &status, WNOHANG);
            if (pid != JSI_BAD_PID) {
                if (waitPtr != &table->info[table->used - 1]) {
                    *waitPtr = table->info[table->used - 1];
                }
                table->used--;
            }
        }
    }
}

/**
 * Does waitpid() on the given pid, and then removes the
 * entry from the wait table.
 *
 * Returns the pid if OK and updates *statusPtr with the status,
 * or JSI_BAD_PID if the pid was not in the table.
 */
static pidtype JsiWaitForProcess(struct WaitInfoTable *table, pidtype pid, int *statusPtr)
{
    int i;

    /* Find it in the table */
    for (i = 0; i < table->used; i++) {
        if (pid == table->info[i].pid) {
            /* wait for it */
            JsiWaitPid(pid, statusPtr, 0);

            /* Remove it from the table */
            if (i != table->used - 1) {
                table->info[i] = table->info[table->used - 1];
            }
            table->used--;
            return pid;
        }
    }

    /* Not found */
    return JSI_BAD_PID;
}

/*
 *----------------------------------------------------------------------
 *
 * JsiDetachPids --
 *
 *  This procedure is called to indicate that one or more child
 *  processes have been placed in background and are no longer
 *  cared about.  These children can be cleaned up with JsiReapDetachedPids().
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */

static void JsiDetachPids(Jsi_Interp *interp, int numPids, const pidtype *pidPtr)
{
    int j;
    struct WaitInfoTable *table = jsi_ExecCmdData(interp);

    for (j = 0; j < numPids; j++) {
        /* Find it in the table */
        int i;
        for (i = 0; i < table->used; i++) {
            if (pidPtr[j] == table->info[i].pid) {
                table->info[i].flags |= WI_DETACHED;
                break;
            }
        }
    }
}

static FILE *JsiGetAioFilehandle(Jsi_Interp *interp, const char *name)
{
    Jsi_Channel chan = Jsi_FSNameToChannel(interp, name);
    if (!chan)
        return NULL;
    if (!chan->isNative)
        return NULL;
    return chan->fp;
}
static void FileNormalize(Jsi_Interp *interp, Jsi_Value *path) {
    char *np, *cp = Jsi_ValueString(interp, path, 0);
    if (!cp)
        return;
    np = Jsi_FileRealpath(interp, path, NULL);
    if (np && Jsi_Strcmp(cp, np))
        Jsi_ValueMakeString(interp, path, np);
    else if (np)
        Jsi_Free(np);
}
/*
 *----------------------------------------------------------------------
 *
 * JsiCreatePipeline --
 *
 *  Given an argc/argv array, instantiate a pipeline of processes
 *  as described by the argv.
 *
 * Results:
 *  The return value is a count of the number of new processes
 *  created, or -1 if an error occurred while creating the pipeline.
 *  *pidArrayPtr is filled in with the address of a dynamically
 *  allocated array giving the ids of all of the processes.  It
 *  is up to the caller to free this array when it isn't needed
 *  anymore.  If inPipePtr is non-NULL, *inPipePtr is filled in
 *  with the file id for the input pipe for the pipeline (if any):
 *  the caller must eventually close this file.  If outPipePtr
 *  isn't NULL, then *outPipePtr is filled in with the file id
 *  for the output pipe from the pipeline:  the caller must close
 *  this file.  If errFilePtr isn't NULL, then *errFilePtr is filled
 *  with a file id that may be used to read error output after the
 *  pipeline completes.
 *
 * Side effects:
 *  Processes and pipes are created.
 *
 *----------------------------------------------------------------------
 */
static int
JsiCreatePipeline(Jsi_Interp *interp, Jsi_Value* args, int argc, pidtype **pidArrayPtr,
    fdtype *inPipePtr, fdtype *outPipePtr, fdtype *errFilePtr)
{
    pidtype *pidPtr = NULL;         /* Points to malloc-ed array holding all
                                 * the pids of child processes. */
    int numPids = 0;            /* Actual number of processes that exist
                                 * at *pidPtr right now. */
    int cmdCount;               /* Count of number of distinct commands
                                 * found in argc/argv. */
    const char *input = NULL;   /* Describes input for pipeline, depending
                                 * on "inputFile".  NULL means take input
                                 * from stdin/pipe. */

#define FILE_NAME   0           /* input/output: filename */
#define FILE_APPEND 1           /* output only:  filename, append */
#define FILE_HANDLE 2           /* input/output: filehandle */
#define FILE_TEXT   3           /* input only:   input is actual text */

    int inputFile = FILE_NAME;  /* 1 means input is name of input file.
                                 * 2 means input is filehandle name.
                                 * 0 means input holds actual
                                 * text to be input to command. */

    int outputFile = FILE_NAME; /* 0 means output is the name of output file.
                                 * 1 means output is the name of output file, and append.
                                 * 2 means output is filehandle name.
                                 * All this is ignored if output is NULL
                                 */
    int errorFile = FILE_NAME;  /* 0 means error is the name of error file.
                                 * 1 means error is the name of error file, and append.
                                 * 2 means error is filehandle name.
                                 * All this is ignored if error is NULL
                                 */
    const char *output = NULL;  /* Holds name of output file to pipe to,
                                 * or NULL if output goes to stdout/pipe. */
    const char *error = NULL;   /* Holds name of stderr file to pipe to,
                                 * or NULL if stderr goes to stderr/pipe. */
    fdtype inputId = JSI_BAD_FD;
                                 /* Readable file id input to current command in
                                 * pipeline (could be file or pipe).  JSI_BAD_FD
                                 * means use stdin. */
    fdtype outputId = JSI_BAD_FD;
                                 /* Writable file id for output from current
                                 * command in pipeline (could be file or pipe).
                                 * JSI_BAD_FD means use stdout. */
    fdtype errorId = JSI_BAD_FD;
                                 /* Writable file id for all standard error
                                 * output from all commands in pipeline.  JSI_BAD_FD
                                 * means use stderr. */
    fdtype lastOutputId = JSI_BAD_FD;
                                 /* Write file id for output from last command
                                 * in pipeline (could be file or pipe).
                                 * -1 means use stdout. */
    fdtype pipeIds[2];           /* File ids for pipe that's being created. */
    int firstArg, lastArg;      /* Indexes of first and last arguments in
                                 * current command. */
    int lastBar;
    int i;
    pidtype pid;
    char **save_environ;
    struct WaitInfoTable *table = jsi_ExecCmdData(interp);  /* TODO: mutex??? */
    /*int argc = Jsi_ValueGetLength(interp, args);*/

    /* Holds the args which will be used to exec */
    char **arg_array = Jsi_Calloc((argc + 1), sizeof(*arg_array));
    int arg_count = 0;

    JsiReapDetachedPids(table);

    if (inPipePtr != NULL) {
        *inPipePtr = JSI_BAD_FD;
    }
    if (outPipePtr != NULL) {
        *outPipePtr = JSI_BAD_FD;
    }
    if (errFilePtr != NULL) {
        *errFilePtr = JSI_BAD_FD;
    }
    pipeIds[0] = pipeIds[1] = JSI_BAD_FD;

    /*
     * First, scan through all the arguments to figure out the structure
     * of the pipeline.  Count the number of distinct processes (it's the
     * number of "|" arguments).  If there are "<", "<<", or ">" arguments
     * then make note of input and output redirection and remove these
     * arguments and the arguments that follow them.
     */
    cmdCount = 1;
    lastBar = -1;
    for (i = 0; i < argc; i++) {
        if (i == 0) {
            FileNormalize(interp,  Jsi_ValueArrayIndex(interp, args, 0));
        }
        const char *arg = Jsi_ValueArrayIndexToStr(interp, args, i, NULL);
        
        if (arg[0] == '<') {
            inputFile = FILE_NAME;
            input = arg + 1;
            if (*input == '<') {
                inputFile = FILE_TEXT; // TODO: make this or @ from a var
                input++;
            }
            else if (*input == '@') {
                inputFile = FILE_HANDLE;
                input++;
            }

            if (!*input && ++i < argc) {
                input = Jsi_ValueArrayIndexToStr(interp, args, i, NULL);
            }
        }
        else if (arg[0] == '>') {
            int dup_error = 0;

            outputFile = FILE_NAME;

            output = arg + 1;
            if (*output == '>') {
                outputFile = FILE_APPEND;
                output++;
            }
            if (*output == '&') {
                /* Redirect stderr too */
                output++;
                dup_error = 1;
            }
            if (*output == '@') {
                outputFile = FILE_HANDLE;
                output++;
            }
            if (!*output && ++i < argc) {
                output = Jsi_ValueArrayIndexToStr(interp, args, i, NULL);
            }
            if (dup_error) {
                errorFile = outputFile;
                error = output;
            }
        }
        else if (arg[0] == '2' && arg[1] == '>') {
            error = arg + 2;
            errorFile = FILE_NAME;

            if (*error == '@' || (*error == '&' && error[1] == '1')) {
                errorFile = FILE_HANDLE;
                error++;
            }
            else if (*error == '>') {
                errorFile = FILE_APPEND;
                error++;
            }
            if (!*error && ++i < argc) {
                error = Jsi_ValueArrayIndexToStr(interp, args, i, NULL);
            }
        }
        else {
            if (Jsi_Strcmp(arg, "|") == 0 || Jsi_Strcmp(arg, "|&") == 0) {
                if (i == lastBar + 1 || i == argc - 1) {
                    Jsi_LogError("illegal use of | or |& in command");
                    goto badargs;
                }
                lastBar = i;
                cmdCount++;
            }
            /* Either |, |& or a "normal" arg, so store it in the arg array */
            arg_array[arg_count++] = (char *)arg;
            continue;
        }

        if (i >= argc) {
            Jsi_LogError("can't specify \"%s\" as last word in command", arg);
            goto badargs;
        }
    }

    if (arg_count == 0) {
        Jsi_LogError("didn't specify command to execute");
badargs:
        Jsi_Free(arg_array);
        return -1;
    }

    /* Must do this before vfork(), so do it now */
    save_environ = JsiSaveEnv(JsiBuildEnv(interp));

    /*
     * Set up the redirected input source for the pipeline, if
     * so requested.
     */
    if (input != NULL) {
        if (inputFile == FILE_TEXT) {
            /*
             * Immediate data in command.  Create temporary file and
             * put data into file.
             */
            inputId = JsiCreateTemp(interp, input);
            if (inputId == JSI_BAD_FD) {
                goto error;
            }
        }
        else if (inputFile == FILE_HANDLE) {
            /* Should be a file descriptor */
            FILE *fh = JsiGetAioFilehandle(interp, input);

            if (fh == NULL) {
                goto error;
            }
            inputId = JsiDupFd(JsiFileno(fh));
        }
        else {
            /*
             * File redirection.  Just open the file.
             */
            inputId = JsiOpenForRead(interp, input);
            if (inputId == JSI_BAD_FD) {
                Jsi_LogError("couldn't read file \"%s\": %s", input, JsiStrError());
                goto error;
            }
        }
    }
    else if (inPipePtr != NULL) {
        if (JsiPipe(pipeIds) != 0) {
            Jsi_LogError("couldn't create input pipe for command");
            goto error;
        }
        inputId = pipeIds[0];
        *inPipePtr = pipeIds[1];
        pipeIds[0] = pipeIds[1] = JSI_BAD_FD;
    }

    /*
     * Set up the redirected output sink for the pipeline from one
     * of two places, if requested.
     */
    if (output != NULL) {
        if (outputFile == FILE_HANDLE) {
            FILE *fh = JsiGetAioFilehandle(interp, output);
            if (fh == NULL) {
                goto error;
            }
            fflush(fh);
            lastOutputId = JsiDupFd(JsiFileno(fh));
        }
        else {
            /*
             * Output is to go to a file.
             */
            lastOutputId = JsiOpenForWrite(interp, output, outputFile == FILE_APPEND);
            if (lastOutputId == JSI_BAD_FD) {
                Jsi_LogError("couldn't write file \"%s\": %s", output, JsiStrError());
                goto error;
            }
        }
    }
    else if (outPipePtr != NULL) {
        /*
         * Output is to go to a pipe.
         */
        if (JsiPipe(pipeIds) != 0) {
            Jsi_LogError("couldn't create output pipe");
            goto error;
        }
        lastOutputId = pipeIds[1];
        *outPipePtr = pipeIds[0];
        pipeIds[0] = pipeIds[1] = JSI_BAD_FD;
    }
    /* If we are redirecting stderr with 2>filename or 2>@fileId, then we ignore errFilePtr */
    if (error != NULL) {
        if (errorFile == FILE_HANDLE) {
            if (Jsi_Strcmp(error, "1") == 0) {
                /* Special 2>@1 */
                if (lastOutputId != JSI_BAD_FD) {
                    errorId = JsiDupFd(lastOutputId);
                }
                else {
                    /* No redirection of stdout, so just use 2>@stdout */
                    error = "stdout";
                }
            }
            if (errorId == JSI_BAD_FD) {
                FILE *fh = JsiGetAioFilehandle(interp, error);
                if (fh == NULL) {
                    goto error;
                }
                fflush(fh);
                errorId = JsiDupFd(JsiFileno(fh));
            }
        }
        else {
            /*
             * Output is to go to a file.
             */
            errorId = JsiOpenForWrite(interp, error, errorFile == FILE_APPEND);
            if (errorId == JSI_BAD_FD) {
                Jsi_LogError("couldn't write file \"%s\": %s", error, JsiStrError());
                goto error;
            }
        }
    }
    else if (errFilePtr != NULL) {
        /*
         * Set up the standard error output sink for the pipeline, if
         * requested.  Use a temporary file which is opened, then deleted.
         * Could potentially just use pipe, but if it filled up it could
         * cause the pipeline to deadlock:  we'd be waiting for processes
         * to complete before reading stderr, and processes couldn't complete
         * because stderr was backed up.
         */
        errorId = JsiCreateTemp(interp, NULL);
        if (errorId == JSI_BAD_FD) {
            goto error;
        }
        *errFilePtr = JsiDupFd(errorId);
    }

    /*
     * Scan through the argc array, forking off a process for each
     * group of arguments between "|" arguments.
     */

    pidPtr = Jsi_Calloc(cmdCount, sizeof(*pidPtr));
    for (i = 0; i < numPids; i++) {
        pidPtr[i] = JSI_BAD_PID;
    }
    for (firstArg = 0; firstArg < arg_count; numPids++, firstArg = lastArg + 1) {
        int pipe_dup_err = 0;
        fdtype origErrorId = errorId;

        for (lastArg = firstArg; lastArg < arg_count; lastArg++) {
            if (arg_array[lastArg][0] == '|') {
                if (arg_array[lastArg][1] == '&') {
                    pipe_dup_err = 1;
                }
                break;
            }
        }
        /* Replace | with NULL for execv() */
        arg_array[lastArg] = NULL;
        if (lastArg == arg_count) {
            outputId = lastOutputId;
        }
        else {
            if (JsiPipe(pipeIds) != 0) {
                Jsi_LogError("couldn't create pipe");
                goto error;
            }
            outputId = pipeIds[1];
        }

        /* Now fork the child */

#ifdef __MINGW32__
        pid = JsiStartWinProcess(interp, &arg_array[firstArg], save_environ ? save_environ[0] : NULL, inputId, outputId, errorId);
        if (pid == JSI_BAD_PID) {
            Jsi_LogError("couldn't exec \"%s\"", arg_array[firstArg]);
            goto error;
        }
#else
        /*
         * Disable SIGPIPE signals:  if they were allowed, this process
         * might go away unexpectedly if children misbehave.  This code
         * can potentially interfere with other application code that
         * expects to handle SIGPIPEs;  what's really needed is an
         * arbiter for signals to allow them to be "shared".
         */
        if (table->info == NULL) {
            (void)signal(SIGPIPE, SIG_IGN);
        }

        /* Need to do this befor vfork() */
        if (pipe_dup_err) {
            errorId = outputId;
        }

        /*
         * Make a new process and enter it into the table if the fork
         * is successful.
         */
        pid = vfork();
        if (pid < 0) {
            Jsi_LogError("couldn't fork child process");
            goto error;
        }
        if (pid == 0) {
            /* Child */

            if (inputId != -1) dup2(inputId, 0);
            if (outputId != -1) dup2(outputId, 1);
            if (errorId != -1) dup2(errorId, 2);

            for (i = 3; (i <= outputId) || (i <= inputId) || (i <= errorId); i++) {
                close(i);
            }

            /* TODO: execvpe not using last arg! */
            execvpe(arg_array[firstArg], &arg_array[firstArg], Jsi_GetEnviron());

            /* Need to prep an error message before vfork(), just in case */
            fprintf(stderr, "couldn't exec \"%s\"", arg_array[firstArg]);
            _exit(127);
        }
#endif

        /* parent */

        /*
         * Enlarge the wait table if there isn't enough space for a new
         * entry.
         */
        if (table->used == table->size) {
            table->size += WAIT_TABLE_GROW_BY;
            table->info = Jsi_Realloc(table->info, table->size * sizeof(*table->info));
        }

        table->info[table->used].pid = pid;
        table->info[table->used].flags = 0;
        table->used++;

        pidPtr[numPids] = pid;

        /* Restore in case of pipe_dup_err */
        errorId = origErrorId;

        /*
         * Close off our copies of file descriptors that were set up for
         * this child, then set up the input for the next child.
         */

        if (inputId != JSI_BAD_FD) {
            JsiCloseFd(inputId);
        }
        if (outputId != JSI_BAD_FD) {
            JsiCloseFd(outputId);
        }
        inputId = pipeIds[0];
        pipeIds[0] = pipeIds[1] = JSI_BAD_FD;
    }
    *pidArrayPtr = pidPtr;

    /*
     * All done.  Cleanup open files lying around and then return.
     */

  cleanup:
    if (inputId != JSI_BAD_FD) {
        JsiCloseFd(inputId);
    }
    if (lastOutputId != JSI_BAD_FD) {
        JsiCloseFd(lastOutputId);
    }
    if (errorId != JSI_BAD_FD) {
        JsiCloseFd(errorId);
    }
    Jsi_Free(arg_array);

    if (save_environ)
        JsiRestoreEnv(save_environ);

    return numPids;

    /*
     * An error occurred.  There could have been extra files open, such
     * as pipes between children.  Clean them all up.  Detach any child
     * processes that have been created.
     */

  error:
    if ((inPipePtr != NULL) && (*inPipePtr != JSI_BAD_FD)) {
        JsiCloseFd(*inPipePtr);
        *inPipePtr = JSI_BAD_FD;
    }
    if ((outPipePtr != NULL) && (*outPipePtr != JSI_BAD_FD)) {
        JsiCloseFd(*outPipePtr);
        *outPipePtr = JSI_BAD_FD;
    }
    if ((errFilePtr != NULL) && (*errFilePtr != JSI_BAD_FD)) {
        JsiCloseFd(*errFilePtr);
        *errFilePtr = JSI_BAD_FD;
    }
    if (pipeIds[0] != JSI_BAD_FD) {
        JsiCloseFd(pipeIds[0]);
    }
    if (pipeIds[1] != JSI_BAD_FD) {
        JsiCloseFd(pipeIds[1]);
    }
    if (pidPtr != NULL) {
        for (i = 0; i < numPids; i++) {
            if (pidPtr[i] != JSI_BAD_PID) {
                JsiDetachPids(interp, 1, &pidPtr[i]);
            }
        }
        Jsi_Free(pidPtr);
    }
    numPids = -1;
    goto cleanup;
}

/*
 *----------------------------------------------------------------------
 *
 * JsiCleanupChildren --
 *
 *  This is a utility procedure used to wait for child processes
 *  to exit, record information about abnormal exits, and then
 *  collect any stderr output generated by them.
 *
 * Results:
 *  The return value is a standard Tcl result.  If anything at
 *  weird happened with the child processes, JSI_ERROR is returned
 *  and a message is left in interp->result.
 *
 * Side effects:
 *  If the last character of interp->result is a newline, then it
 *  is removed.  File errorId gets closed, and pidPtr is freed
 *  back to the storage allocator.
 *
 *----------------------------------------------------------------------
 */

static int JsiCleanupChildren(Jsi_Interp *interp, int numPids, pidtype *pidPtr, fdtype errorId, Jsi_DString *dStr, Jsi_DString *cStr, int *code)
{
    struct WaitInfoTable *table = jsi_ExecCmdData(interp);
    int result = JSI_OK;
    int i, exitCode = 256;
    char buf[1000];
    Jsi_DString sStr;
    Jsi_DSInit(&sStr);
    for (i = 0; i < numPids; i++) {
        int waitStatus = 0;
        if (cStr) {
            if (i==0)
                Jsi_DSAppend(cStr, (Jsi_DSLength(cStr)>1?", ":""), "children: [", NULL);
            else
                Jsi_DSAppend(&sStr, ", ", NULL);
        }
        if (JsiWaitForProcess(table, pidPtr[i], &waitStatus) != JSI_BAD_PID) {
            //  if (JsiCheckWaitStatus(interp, pidPtr[i], waitStatus, dStr?&sStr:0) != JSI_OK) 
            //result = JSI_ERROR;  // TODO: we don't error out on non-zero return code. Find way to return it.
            int es = WEXITSTATUS(waitStatus);
            if (WIFEXITED(waitStatus)) {
                if (i==0)
                    exitCode = es;
                else if (es>exitCode)
                    exitCode = es;
            }
            if (cStr) {
                pidtype pid = pidPtr[i];
                if (WIFEXITED(waitStatus))
                    sprintf(buf, "{type:\"exit\", pid:%ld, exitCode:%d}", (long)pid, es);
                else 
                    sprintf(buf, "{type:\"%s\", pid:%ld, signal: %d}",
                        (WIFSIGNALED(waitStatus) ? "killed" : "suspended"), (long)pid, WTERMSIG(waitStatus));
                Jsi_DSAppend(&sStr, buf, NULL);
            }
        }
    }
    if (i>0 && cStr) {
        if (exitCode != 256)
            sprintf(buf, ", exitCode: %d", exitCode);
        Jsi_DSAppend(cStr, Jsi_DSValue(&sStr), "]", buf, NULL);
    }
    Jsi_DSFree(&sStr);
    Jsi_Free(pidPtr);

    /*
     * Read the standard error file.  If there's anything there,
     * then add the file's contents to the result
     * string.
     */
    if (errorId != JSI_BAD_FD) {
        JsiRewindFd(errorId);
        if (JsiAppendStreamToString(interp, errorId, dStr) != JSI_OK) {
            result = JSI_ERROR;
        }
    }
    *code = exitCode;
    return result;
}
void *jsi_ExecCmdData(Jsi_Interp *interp)
{
    struct WaitInfoTable *table = Jsi_InterpGetData(interp, "jsiExecData", NULL);
    return table;
}

int Jsi_execInit(Jsi_Interp *interp)
{
    Jsi_InterpSetData(interp, "jsiExecData", JsiFreeWaitInfoTable, JsiAllocWaitInfoTable());
    return JSI_OK;
}

#if defined(__MINGW32__)
/* Windows-specific (mingw) implementation */

static SECURITY_ATTRIBUTES *JsiStdSecAttrs(void)
{
    static SECURITY_ATTRIBUTES secAtts;

    secAtts.nLength = sizeof(SECURITY_ATTRIBUTES);
    secAtts.lpSecurityDescriptor = NULL;
    secAtts.bInheritHandle = TRUE;
    return &secAtts;
}

static int JsiErrno(void)
{
    switch (GetLastError()) {
    case ERROR_FILE_NOT_FOUND: return ENOENT;
    case ERROR_PATH_NOT_FOUND: return ENOENT;
    case ERROR_TOO_MANY_OPEN_FILES: return EMFILE;
    case ERROR_ACCESS_DENIED: return EACCES;
    case ERROR_INVALID_HANDLE: return EBADF;
    case ERROR_BAD_ENVIRONMENT: return E2BIG;
    case ERROR_BAD_FORMAT: return ENOEXEC;
    case ERROR_INVALID_ACCESS: return EACCES;
    case ERROR_INVALID_DRIVE: return ENOENT;
    case ERROR_CURRENT_DIRECTORY: return EACCES;
    case ERROR_NOT_SAME_DEVICE: return EXDEV;
    case ERROR_NO_MORE_FILES: return ENOENT;
    case ERROR_WRITE_PROTECT: return EROFS;
    case ERROR_BAD_UNIT: return ENXIO;
    case ERROR_NOT_READY: return EBUSY;
    case ERROR_BAD_COMMAND: return EIO;
    case ERROR_CRC: return EIO;
    case ERROR_BAD_LENGTH: return EIO;
    case ERROR_SEEK: return EIO;
    case ERROR_WRITE_FAULT: return EIO;
    case ERROR_READ_FAULT: return EIO;
    case ERROR_GEN_FAILURE: return EIO;
    case ERROR_SHARING_VIOLATION: return EACCES;
    case ERROR_LOCK_VIOLATION: return EACCES;
    case ERROR_SHARING_BUFFER_EXCEEDED: return ENFILE;
    case ERROR_HANDLE_DISK_FULL: return ENOSPC;
    case ERROR_NOT_SUPPORTED: return ENODEV;
    case ERROR_REM_NOT_LIST: return EBUSY;
    case ERROR_DUP_NAME: return EEXIST;
    case ERROR_BAD_NETPATH: return ENOENT;
    case ERROR_NETWORK_BUSY: return EBUSY;
    case ERROR_DEV_NOT_EXIST: return ENODEV;
    case ERROR_TOO_MANY_CMDS: return EAGAIN;
    case ERROR_ADAP_HDW_ERR: return EIO;
    case ERROR_BAD_NET_RESP: return EIO;
    case ERROR_UNEXP_NET_ERR: return EIO;
    case ERROR_NETNAME_DELETED: return ENOENT;
    case ERROR_NETWORK_ACCESS_DENIED: return EACCES;
    case ERROR_BAD_DEV_TYPE: return ENODEV;
    case ERROR_BAD_NET_NAME: return ENOENT;
    case ERROR_TOO_MANY_NAMES: return ENFILE;
    case ERROR_TOO_MANY_SESS: return EIO;
    case ERROR_SHARING_PAUSED: return EAGAIN;
    case ERROR_REDIR_PAUSED: return EAGAIN;
    case ERROR_FILE_EXISTS: return EEXIST;
    case ERROR_CANNOT_MAKE: return ENOSPC;
    case ERROR_OUT_OF_STRUCTURES: return ENFILE;
    case ERROR_ALREADY_ASSIGNED: return EEXIST;
    case ERROR_INVALID_PASSWORD: return EPERM;
    case ERROR_NET_WRITE_FAULT: return EIO;
    case ERROR_NO_PROC_SLOTS: return EAGAIN;
    case ERROR_DISK_CHANGE: return EXDEV;
    case ERROR_BROKEN_PIPE: return EPIPE;
    case ERROR_OPEN_FAILED: return ENOENT;
    case ERROR_DISK_FULL: return ENOSPC;
    case ERROR_NO_MORE_SEARCH_HANDLES: return EMFILE;
    case ERROR_INVALID_TARGET_HANDLE: return EBADF;
    case ERROR_INVALID_NAME: return ENOENT;
    case ERROR_PROC_NOT_FOUND: return ESRCH;
    case ERROR_WAIT_NO_CHILDREN: return ECHILD;
    case ERROR_CHILD_NOT_COMPLETE: return ECHILD;
    case ERROR_DIRECT_ACCESS_HANDLE: return EBADF;
    case ERROR_SEEK_ON_DEVICE: return ESPIPE;
    case ERROR_BUSY_DRIVE: return EAGAIN;
    case ERROR_DIR_NOT_EMPTY: return EEXIST;
    case ERROR_NOT_LOCKED: return EACCES;
    case ERROR_BAD_PATHNAME: return ENOENT;
    case ERROR_LOCK_FAILED: return EACCES;
    case ERROR_ALREADY_EXISTS: return EEXIST;
    case ERROR_FILENAME_EXCED_RANGE: return ENAMETOOLONG;
    case ERROR_BAD_PIPE: return EPIPE;
    case ERROR_PIPE_BUSY: return EAGAIN;
    case ERROR_PIPE_NOT_CONNECTED: return EPIPE;
    case ERROR_DIRECTORY: return ENOTDIR;
    }
    return EINVAL;
}

static int JsiPipe(fdtype pipefd[2])
{
    if (CreatePipe(&pipefd[0], &pipefd[1], NULL, 0)) {
        return 0;
    }
    return -1;
}

static fdtype JsiDupFd(fdtype infd)
{
    fdtype dupfd;
    pidtype pid = GetCurrentProcess();

    if (DuplicateHandle(pid, infd, pid, &dupfd, 0, TRUE, DUPLICATE_SAME_ACCESS)) {
        return dupfd;
    }
    return JSI_BAD_FD;
}

static int JsiRewindFd(fdtype fd)
{
    return SetFilePointer(fd, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER ? -1 : 0;
}

static FILE *JsiFdOpenForRead(fdtype fd)
{
    return _fdopen(_open_osfhandle((int)fd, _O_RDONLY | _O_TEXT), "r");
}

static fdtype JsiFileno(FILE *fh)
{
    return (fdtype)_get_osfhandle(_fileno(fh));
}

static fdtype JsiOpenForRead(Jsi_Interp *interp, const char *filename)
{
    char *fn = Jsi_FileRealpathStr(interp, filename, NULL);
    int rc = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        JsiStdSecAttrs(), OPEN_EXISTING, 0, NULL);
    Jsi_Free(fn);
    return rc;
}

static fdtype JsiOpenForWrite(Jsi_Interp *interp, const char *filename, int append)
{
    char *fn = Jsi_FileRealpathStr(interp, filename, NULL);
    int rc = CreateFile(fn, append ? FILE_APPEND_DATA : GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        JsiStdSecAttrs(), append ? OPEN_ALWAYS : CREATE_ALWAYS, 0, (HANDLE) NULL);
    Jsi_Free(fn);
    return rc;
}

static FILE *JsiFdOpenForWrite(fdtype fd)
{
    return _fdopen(_open_osfhandle((int)fd, _O_TEXT), "w");
}

static pidtype JsiWaitPid(pidtype pid, int *status, int nohang)
{
    DWORD ret = WaitForSingleObject(pid, nohang ? 0 : INFINITE);
    if (ret == WAIT_TIMEOUT || ret == WAIT_FAILED) {
        /* WAIT_TIMEOUT can only happend with WNOHANG */
        return JSI_BAD_PID;
    }
    GetExitCodeProcess(pid, &ret);
    *status = ret;
    CloseHandle(pid);
    return pid;
}

HANDLE JsiCreateTemp(Jsi_Interp *interp, const char *contents)
{
    char name[MAX_PATH];
    HANDLE handle;

    if (!GetTempPath(MAX_PATH, name) || !GetTempFileName(name, "JSI", 0, name)) {
        return JSI_BAD_FD;
    }

    handle = CreateFile(name, GENERIC_READ | GENERIC_WRITE, 0, JsiStdSecAttrs(),
            CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
            NULL);

    if (handle == INVALID_HANDLE_VALUE) {
        goto error;
    }

    if (contents != NULL) {
        /* Use fdopen() to get automatic text-mode translation */
        FILE *fh = JsiFdOpenForWrite(JsiDupFd(handle));
        if (fh == NULL) {
            goto error;
        }

        if (fwrite(contents, strlen(contents), 1, fh) != 1) {
            fclose(fh);
            goto error;
        }
        fseek(fh, 0, SEEK_SET);
        fclose(fh);
    }
    return handle;

  error:
    Jsi_SetResultErrno(interp, "failed to create temp file");
    CloseHandle(handle);
    DeleteFile(name);
    return JSI_BAD_FD;
}

static int
JsiWinFindExecutable(const char *originalName, char fullPath[MAX_PATH])
{
    int i;
    static char extensions[][5] = {".exe", "", ".bat"};

    for (i = 0; i < (int) (sizeof(extensions) / sizeof(extensions[0])); i++) {
        lstrcpyn(fullPath, originalName, MAX_PATH - 5);
        lstrcat(fullPath, extensions[i]);

        if (SearchPath(NULL, fullPath, NULL, MAX_PATH, fullPath, NULL) == 0) {
            continue;
        }
        if (GetFileAttributes(fullPath) & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        return 0;
    }

    return -1;
}

static char **JsiSaveEnv(char **env)
{
    return env;
}

static void JsiRestoreEnv(char **env)
{
    //JsiFreeEnv(env, Jsi_GetEnviron());
}

char *JsiWinBuildCommandLine(Jsi_Interp *interp, char **argv, Jsi_DString *dStr)
{
    char *start, *special;
    int quote, i;
    Jsi_DSInit(dStr);

    for (i = 0; argv[i]; i++) {
        if (i > 0) {
            Jsi_DSAppendLen(dStr, " ", 1);
        }

        if (argv[i][0] == '\0') {
            quote = JSI_OUTPUT_QUOTE;
        }
        else {
            quote = 0;
            for (start = argv[i]; *start != '\0'; start++) {
                if (isspace(UCHAR(*start))) {
                    quote = JSI_OUTPUT_QUOTE;
                    break;
                }
            }
        }
        if (quote) {
            Jsi_DSAppendLen(dStr, "\"" , 1);
        }

        start = argv[i];
        for (special = argv[i]; ; ) {
            if ((*special == '\\') && (special[1] == '\\' ||
                    special[1] == '"' || (quote && special[1] == '\0'))) {
                Jsi_DSAppendLen(dStr, start, special - start);
                start = special;
                while (1) {
                    special++;
                    if (*special == '"' || (quote && *special == '\0')) {
                        /*
                         * N backslashes followed a quote -> insert
                         * N * 2 + 1 backslashes then a quote.
                         */

                        Jsi_DSAppendLen(dStr, start, special - start);
                        break;
                    }
                    if (*special != '\\') {
                        break;
                    }
                }
                Jsi_DSAppendLen(dStr, start, special - start);
                start = special;
            }
            if (*special == '"') {
        if (special == start) {
            Jsi_DSAppendLen(dStr, "\"", 1);
        }
        else {
            Jsi_DSAppendLen(dStr, start, special - start);
        }
                Jsi_DSAppendLen(dStr, "\\\"", 2);
                start = special + 1;
            }
            if (*special == '\0') {
                break;
            }
            special++;
        }
        Jsi_DSAppendLen(dStr, start, special - start);
        if (quote) {
            Jsi_DSAppendLen(dStr, "\"", 1);
        }
    }
    return Jsi_DSValue(dStr);
}

static pidtype
JsiStartWinProcess(Jsi_Interp *interp, char **argv, char *env, fdtype inputId, fdtype outputId, fdtype errorId)
{
    STARTUPINFO startInfo;
    PROCESS_INFORMATION procInfo;
    HANDLE hProcess, h;
    char execPath[MAX_PATH];
    char *originalName;
    pidtype pid = JSI_BAD_PID;
    char *cmdLineStr;

    if (JsiWinFindExecutable(argv[0], execPath) < 0) {
        return JSI_BAD_PID;
    }
    originalName = argv[0];
    argv[0] = execPath;

    hProcess = GetCurrentProcess();
    Jsi_DString dStr;
    cmdLineStr = JsiWinBuildCommandLine(interp, argv, &dStr);

    /*
     * STARTF_USESTDHANDLES must be used to pass handles to child process.
     * Using SetStdHandle() and/or dup2() only works when a console mode
     * parent process is spawning an attached console mode child process.
     */

    ZeroMemory(&startInfo, sizeof(startInfo));
    startInfo.cb = sizeof(startInfo);
    startInfo.dwFlags   = STARTF_USESTDHANDLES;
    startInfo.hStdInput = INVALID_HANDLE_VALUE;
    startInfo.hStdOutput= INVALID_HANDLE_VALUE;
    startInfo.hStdError = INVALID_HANDLE_VALUE;

    /*
     * Duplicate all the handles which will be passed off as stdin, stdout
     * and stderr of the child process. The duplicate handles are set to
     * be inheritable, so the child process can use them.
     */
    if (inputId == JSI_BAD_FD) {
        if (CreatePipe(&startInfo.hStdInput, &h, JsiStdSecAttrs(), 0) != FALSE) {
            CloseHandle(h);
        }
    } else {
        DuplicateHandle(hProcess, inputId, hProcess, &startInfo.hStdInput,
                0, TRUE, DUPLICATE_SAME_ACCESS);
    }
    if (startInfo.hStdInput == JSI_BAD_FD) {
        goto end;
    }

    if (outputId == JSI_BAD_FD) {
        startInfo.hStdOutput = CreateFile("NUL:", GENERIC_WRITE, 0,
                JsiStdSecAttrs(), OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    } else {
        DuplicateHandle(hProcess, outputId, hProcess, &startInfo.hStdOutput,
                0, TRUE, DUPLICATE_SAME_ACCESS);
    }
    if (startInfo.hStdOutput == JSI_BAD_FD) {
        goto end;
    }

    if (errorId == JSI_BAD_FD) {
        /*
         * If handle was not set, errors should be sent to an infinitely
         * deep sink.
         */

        startInfo.hStdError = CreateFile("NUL:", GENERIC_WRITE, 0,
                JsiStdSecAttrs(), OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    } else {
        DuplicateHandle(hProcess, errorId, hProcess, &startInfo.hStdError,
                0, TRUE, DUPLICATE_SAME_ACCESS);
    }
    if (startInfo.hStdError == JSI_BAD_FD) {
        goto end;
    }

    if (!CreateProcess(NULL, cmdLineStr, NULL, NULL, TRUE,
            0, env, NULL, &startInfo, &procInfo)) {
        goto end;
    }

    /*
     * "When an application spawns a process repeatedly, a new thread
     * instance will be created for each process but the previous
     * instances may not be cleaned up.  This results in a significant
     * virtual memory loss each time the process is spawned.  If there
     * is a WaitForInputIdle() call between CreateProcess() and
     * CloseHandle(), the problem does not occur." PSS ID Number: Q124121
     */

    WaitForInputIdle(procInfo.hProcess, 5000);
    CloseHandle(procInfo.hThread);

    pid = procInfo.hProcess;

    end:
    Jsi_DSFree(&dStr);
    if (startInfo.hStdInput != JSI_BAD_FD) {
        CloseHandle(startInfo.hStdInput);
    }
    if (startInfo.hStdOutput != JSI_BAD_FD) {
        CloseHandle(startInfo.hStdOutput);
    }
    if (startInfo.hStdError != JSI_BAD_FD) {
        CloseHandle(startInfo.hStdError);
    }
    return pid;
}
#else
/* Unix-specific implementation */

static int JsiOpenForRead(Jsi_Interp *interp, const char *filename)
{
    char *fn = Jsi_FileRealpathStr(interp, filename, NULL);
    int rc = open(fn, O_RDONLY, 0);
    Jsi_Free(fn);
    return rc;
}

static int JsiOpenForWrite(Jsi_Interp *interp, const char *filename, int append)
{
    char *fn = Jsi_FileRealpathStr(interp, filename, NULL);
    int rc = open(fn, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0666);
    Jsi_Free(fn);
    return rc;
}

static int JsiRewindFd(int fd)
{
    return lseek(fd, 0L, SEEK_SET);
}

int JsiCreateTemp(Jsi_Interp *interp, const char *contents)
{
    char inName[] = "/tmp/jsiXXXXXX";

    int fd = mkstemp(inName);
    if (fd == JSI_BAD_FD) {
        Jsi_SetResultErrno(interp, "couldn't create temp file");
        return -1;
    }
    unlink(inName);
    if (contents) {
        int length = strlen(contents);
        if (write(fd, contents, length) != length) {
            Jsi_SetResultErrno(interp, "couldn't write temp file");
            close(fd);
            return -1;
        }
        lseek(fd, 0L, SEEK_SET);
    }
    return fd;
}

static char **JsiSaveEnv(char **env)
{
#if HAS_SAVE_ENV
    char **saveenv = Jsi_GetEnviron();
    Jsi_SetEnviron(env);
    return saveenv;
#else
    return NULL;
#endif
}

static void JsiRestoreEnv(char **env)
{
#if HAS_SAVE_ENV
    JsiFreeEnv(Jsi_GetEnviron(), env);
    Jsi_SetEnviron(env);
#endif
}
#endif
#endif
#endif
