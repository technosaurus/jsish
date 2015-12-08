#ifndef JSI_LITE_ONLY
#ifdef HAVE_WEBSOCKET
#ifdef VALUE_DEBUG
#include "jsiInt.h"
#else

#ifndef JSI_AMALGAMATION
#include "jsi.h"
#endif
JSI_EXTENSION_INI

#define jsi_Sig int

#ifndef __WIN32
#include <time.h>
#include <sys/time.h>
#endif /* !__WIN32 */
#endif /* VALUE_DEBUG */

#include <ctype.h>

#ifdef CMAKE_BUILD
#include "lws_config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#ifdef WIN32

#ifdef EXTERNAL_POLL
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <stddef.h>

    #include "websock-w32.h"
#endif

#else /* WIN32 */
#include <syslog.h>
#include <unistd.h>
#endif /* WIN32 */

#include <signal.h>

#include <libwebsockets.h>

static Jsi_CmdSpec websockCmds[];

#ifdef EXTERNAL_POLL
static int max_poll_elements;
static struct pollfd *pollfds;
static int *fd_lookup;
static int count_pollfds;
static int force_exit = 0;
#endif /* EXTERNAL_POLL */

typedef enum {  PSS_DEAD, PSS_HTTP, PSS_CONNECTED, PSS_RECV, PSS_SENT, PSS_SENDERR } pss_state;
enum { JWS_SIG_SYS=0xdeadbeea, JWS_SIG_OBJ, JWS_SIG_PSS };

#ifndef NDEBUG
#ifndef MEMCLEAR
#define MEMCLEAR(s) memset(s, 0, sizeof(*s));
#endif
#else
#define MEMCLEAR(s)
#endif
#define WSSIGASSERT(s,n) assert(s->sig == JWS_SIG_##n)

enum demo_protocols {
    /* always first */
    PROTOCOL_HTTP = 0,
    PROTOCOL_JSI,
    /* always last */
    DEMO_PROTOCOL_COUNT
};

typedef struct { /* Interp wide data. */
    int sig;
    Jsi_Interp *interp;
    Jsi_Hash *wsTable;
    int wIdx;
} WebsocketObjInterpData;

typedef struct { /* Per server (or client) data. */
    int sig;
    WebsocketObjInterpData *interpData;
    Jsi_Interp *interp;
    Jsi_Hash *pssTable;
    Jsi_Value *callback;
    Jsi_Value *onCloseLast;
    Jsi_Value *onClose;
    Jsi_Value *onOpen;
    Jsi_Value *defaultUrl;
    Jsi_Value *rootdir;
    Jsi_Value *interface;
    Jsi_Value *address;
    Jsi_Value *mimeTypes;
    char client_name[128];
    char client_ip[128];
    int idx;
    int port;
    char *iface;
    unsigned int oldus;
    char client;
    char noWebsock;
    char use_ssl;
    char nowarn;
    int opts;
    int hasOpts;
    int debug;
    int maxConnects;
    int daemonize;
    int deleted;
    int close_test;
    int connectCnt;
    int createCnt;
    int redirCnt;
    time_t createLast;
    char *cmdName;
    struct libwebsocket *wsi_choked[20];
    int num_wsi_choked;
    struct libwebsocket *wsi;

    struct libwebsocket_context *context;
    struct lws_context_creation_info info;
    Jsi_Event *event;
    Jsi_Obj *fobj;
    Jsi_Hash *handlers;
    int objId;
    struct libwebsocket_protocols protocols[DEMO_PROTOCOL_COUNT+1];
    int ietf_version;
    int rx_buffer_size;
    char *ssl_cert_filepath;
    char *ssl_private_key_filepath;
    int ws_uid;
    int ws_gid;
    char *cl_host;
    char *cl_origin;
} WebsocketObj;

typedef struct { /* Per session connection (to each server) */
    int sig;
    WebsocketObj *cmdPtr;
    pss_state state;
    int sentCnt, recvCnt, sentErrCnt, httpCnt;
    time_t sentLast, recvLast, sentErrLast, httpLast;
    struct libwebsocket *wsi;
    Jsi_HashEntry *hPtr;
    void *user;
    Jsi_Stack stack;
    int id;
} WebsocketPss;

#define IIOF .flags=JSI_OPT_INIT_ONLY
static Jsi_OptionSpec WSOptions[] =
{
    JSI_OPT(INT,    WebsocketObj, connectCnt, .help="Number of active connections", .flags=JSI_OPT_READ_ONLY),
    JSI_OPT(VALUE,  WebsocketObj, address,    .help="Address for client to connect to", .init="127.0.0.0" ),
    JSI_OPT(FUNC,   WebsocketObj, callback,   .help="Function to callback when event occurs"),
    JSI_OPT(BOOL,   WebsocketObj, client,     .help="Run in client mode", IIOF),
    JSI_OPT(INT,    WebsocketObj, debug,      .help="Set debug level"),
    JSI_OPT(VALUE,  WebsocketObj, defaultUrl, .help="Default url to serve out", .init="."),
    JSI_OPT(VALUE,  WebsocketObj, interface,  .help="Interface for server to listen on, eg. 'eth0' or 'lo'", IIOF),
    JSI_OPT(INT,    WebsocketObj, maxConnects,.help="In server mode, max number of client connections accepted"),
    JSI_OPT(VALUE,  WebsocketObj, mimeTypes,  .help="Map of file extensions to mime types (eg. {txt:'text/plain', bb:'text/bb'})", IIOF),
    JSI_OPT(BOOL,   WebsocketObj, noWebsock,  .help="Serve html, but disallow websockets", IIOF),
    JSI_OPT(BOOL,   WebsocketObj, nowarn,     .help="Quietly ignore file not found"),
    JSI_OPT(FUNC,   WebsocketObj, onClose,    .help="Function to call when connection closes"),
    JSI_OPT(FUNC,   WebsocketObj, onCloseLast,.help="Function to call when last connection closes"),
    JSI_OPT(FUNC,   WebsocketObj, onOpen,     .help="Function to call when connection starts"),
    JSI_OPT(INT,    WebsocketObj, port,       .help="Port for server to listen on", IIOF, .init="8080" ),
    JSI_OPT(VALUE,  WebsocketObj, rootdir,    .help="Directory to serve html from", .init="."),
    JSI_OPT(BOOL,   WebsocketObj, use_ssl,    .help="Use https (for client)", IIOF),
    JSI_OPT_END(WebsocketObj)
};

static int websocketObjFree(Jsi_Interp *interp, void *data);
static int websocketObjIsTrue(void *data);
static int websocketObjEqual(void *data1, void *data2);

static Jsi_UserObjReg websockobject = {
    "Websocket",
    websockCmds,
    websocketObjFree,
    websocketObjIsTrue,
    websocketObjEqual
};

/* this protocol server (always the first one) just knows how to do HTTP */

static int callback_http(struct libwebsocket_context *context,
        struct libwebsocket *wsi,
        enum libwebsocket_callback_reasons reason, void *user,
        void *in, size_t len);

static int
callback_jsi_protocol(struct libwebsocket_context *context,
      struct libwebsocket *wsi,
      enum libwebsocket_callback_reasons reason,
      void *user, void *in, size_t len);

#if 0
/* list of supported protocols and callbacks */
static struct libwebsocket_protocols protocols[] = {
    /* first protocol must always be HTTP handler */

    {
        .name="http-only",
        .callback=callback_http,
        .per_session_data_size=0,
        .rx_buffer_size=0,          /* max frame size / rx buffer */
    },
    {
        .name="jsi-protocol",
        .callback=callback_jsi_protocol,
        .per_session_data_size=sizeof(WebsocketPss),
        .rx_buffer_size=50000,
    },

    { NULL, NULL, 0, 0 } /* terminator */
};
#endif

static WebsocketPss*
getPss(WebsocketObj *cmdPtr, struct libwebsocket *wsi, void *user, int create)
{
    Jsi_HashEntry *hPtr;
    int isNew;
    WebsocketPss *pss = user;
    if (user==NULL)
        return NULL;
    if (create)
        hPtr = Jsi_HashEntryCreate(cmdPtr->pssTable, user, &isNew);
    else
        hPtr = Jsi_HashEntryFind(cmdPtr->pssTable, user);
    if (!hPtr)
        return NULL;
    if (create == 0 || isNew == 0) {
        WSSIGASSERT(pss, PSS);
        return pss;
    }
    memset(pss, 0, sizeof(*pss));
    pss->sig = JWS_SIG_PSS;
    pss->hPtr = hPtr;
    cmdPtr->connectCnt++;
    cmdPtr->createCnt++;
    cmdPtr->createLast = time(NULL);
    Jsi_HashValueSet(hPtr, pss);
    pss->cmdPtr = cmdPtr;
    pss->wsi = wsi;
    pss->user = user; /* Same as pss. */
    pss->state = PSS_CONNECTED;
    pss->id = cmdPtr->idx++;
    Jsi_StackInit(&pss->stack);
    return pss;
}

static int DelPss(Jsi_Interp *interp, void *data) { Jsi_Free(data); return JSI_OK; }

static void
deletePss(WebsocketPss *pss)
{
    if (pss->hPtr) {
        Jsi_HashEntryDelete(pss->hPtr);
    }
    Jsi_StackFreeElements(&pss->stack, DelPss);
    Jsi_StackFree(&pss->stack);
    pss->cmdPtr->connectCnt--;
    /*Jsi_ObjDecrRefCount(pss->msgs);*/
    pss->hPtr = NULL;
    pss->state = PSS_DEAD;
}

static int ServeString(WebsocketObj *cmdPtr, struct libwebsocket *wsi,
    const char *buf, int code, const char *reason, const char *mime)
{
    int rc, strLen = Jsi_Strlen(buf);
    Jsi_DString jStr = {};
    Jsi_DSPrintf(&jStr,
        "HTTP/1.0 %d %s\x0d\x0a"
        "Server: libwebsockets\x0d\x0a"
        "Content-Type: %s\x0d\x0a"
        "Content-Length: %u\x0d\x0a\x0d\x0a",
        (code<=0?200:code), (reason?reason:"OK"),
        (mime?mime:"text/html"),
        strLen);
    Jsi_DSAppend(&jStr, buf, NULL);
    char *vStr = Jsi_DSValue(&jStr);
    rc = libwebsocket_write(wsi, (unsigned char*)vStr, Jsi_Strlen(vStr), LWS_WRITE_HTTP);
    Jsi_DSFree(&jStr);
    return rc;
}

static int callback_http(struct libwebsocket_context *context,
                         struct libwebsocket *wsi,
                         enum libwebsocket_callback_reasons reason, void *user,
                         void *in, size_t len)
{
    char buf[BUFSIZ];
    char *ext = NULL, *inPtr = (char*)in;
    // int n;
#ifdef EXTERNAL_POLL
    int m;
    int fd = (int)(long)user;
#endif
    WebsocketObj *cmdPtr = libwebsocket_context_user(context);
    Jsi_Interp *interp = cmdPtr->interp;
    int rc = 0;

    buf[0] = 0;
    WSSIGASSERT(cmdPtr, OBJ);
    switch (reason) {
    case LWS_CALLBACK_PROTOCOL_INIT:
        break;
    case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
        if (cmdPtr->maxConnects && cmdPtr->connectCnt>=cmdPtr->maxConnects) {
            if (cmdPtr->debug)
                fprintf(stderr, "maxConnects exceeded: rejecting connection <%p>\n", user);
            rc = -1;
        }
        /* if we returned non-zero from here, we kill the connection */
        break;

    case LWS_CALLBACK_HTTP:
    {
        const char *mime = "text/html";
        if (cmdPtr->defaultUrl && (*inPtr == 0 || !strcmp(inPtr, "/")) && cmdPtr->redirCnt++ < 1000) {
            /* Redirect to defaultUrl. */
            inPtr = Jsi_ValueString(cmdPtr->interp, cmdPtr->defaultUrl, NULL);
            if (inPtr) {
                snprintf(buf, sizeof(buf), "<head><meta http-equiv=\"refresh\" content=\"1; url=%s\" /></head><body>Redirecting...</body>", inPtr);
                rc = ServeString(cmdPtr, wsi, buf, 0, NULL, NULL);
                break;
            }
        }
        if (inPtr) {
            ext = strrchr(inPtr, '.');
        }

        snprintf(buf, sizeof(buf), "%s/%s",
             cmdPtr->rootdir?Jsi_ValueString(cmdPtr->interp, cmdPtr->rootdir, NULL):"./", inPtr);
        if (ext) {
            Jsi_HashEntry *hPtr;

            if (Jsi_Strncasecmp(ext,".png", -1) == 0) mime = "image/png";
            else if (Jsi_Strncasecmp(ext,".ico", -1) == 0) mime = "image/icon";
            else if (Jsi_Strncasecmp(ext,".gif", -1) == 0) mime = "image/gif";
            else if (Jsi_Strncasecmp(ext,".jpeg", -1) == 0) mime = "image/jpeg";
            else if (Jsi_Strncasecmp(ext,".jpg", -1) == 0) mime = "image/jpeg";
            else if (Jsi_Strncasecmp(ext,".js", -1) == 0) mime = "application/x-javascript";
            else if (Jsi_Strncasecmp(ext,".jsi", -1) == 0) mime = "application/x-javascript";
            else if (Jsi_Strncasecmp(ext,".svg", -1) == 0) mime = "image/svg+xml";
            else if (Jsi_Strncasecmp(ext,".css", -1) == 0) mime = "text/css";
            else if (Jsi_Strncasecmp(ext,".json", -1) == 0) mime = "application/json";
            else if (Jsi_Strncasecmp(ext,".txt", -1) == 0) mime = "text/plain";
            else if ((hPtr = Jsi_HashEntryFind(cmdPtr->handlers, ext))) {
                /* Use interprete html eg. using jsi_wpp preprocessor */
                Jsi_DString jStr = {};
                Jsi_Value *vrc = NULL;
                int hrc = 0, strLen, evrc, isalloc=0;
                char *vStr, *hstr;
                Jsi_Value *hv = Jsi_HashValueGet(hPtr);
                
                if (strchr(buf, '\'') || strchr(buf, '\"')) {
                    ServeString(cmdPtr, wsi, "Can not handle quotes in url", 404, NULL, NULL);                    
                }
                if (Jsi_ValueIsFunction(interp, hv)) {
                    Jsi_DSAppend(&jStr, "[\"", buf, "\"];", NULL);
                    vrc = Jsi_ValueNew1(interp);
                    evrc = Jsi_FunctionInvokeJSON(interp, hv, Jsi_DSValue(&jStr), &vrc);
                    isalloc = 1;
                } else {
                    hstr = Jsi_ValueString(interp, hv, NULL);
                    Jsi_DSAppend(&jStr, hstr, "('", buf, "');", NULL);
                    evrc = Jsi_EvalString(interp, Jsi_DSValue(&jStr), JSI_EVAL_RETURN);
                    if (evrc == JSI_OK)
                        vrc = Jsi_InterpResult(interp);
                }
                if (evrc != JSI_OK) {
                    Jsi_LogError("failure in websocket handler");
                } else if ((!vrc) ||
                    (!(vStr = Jsi_ValueString(interp, vrc, &strLen)))) {
                    Jsi_LogError("failed to get result");
                } else {
                    Jsi_DSSetLength(&jStr, 0);
                    Jsi_DSPrintf(&jStr,
                        "HTTP/1.0 200 OK\x0d\x0a"
                        "Server: libwebsockets\x0d\x0a"
                        "Content-Type: %s\x0d\x0a"
                        "Content-Length: %u\x0d\x0a\x0d\x0a",
                        mime,
                        strLen);
                    Jsi_DSAppend(&jStr, vStr, NULL);
                    vStr = Jsi_DSValue(&jStr);
                    hrc = libwebsocket_write(wsi, (unsigned char*)vStr, Jsi_Strlen(vStr), LWS_WRITE_HTTP);
                }
                Jsi_DSFree(&jStr);
                if (isalloc)
                    Jsi_DecrRefCount(interp, vrc);
                if (hrc)
                    return 1;
                return 0;
            } else if (Jsi_Strncasecmp(ext,".html", -1) == 0) {
            } else if (cmdPtr->mimeTypes) {
                /* Lookup mime type in mimeTypes object. */
                const char *nmime;
                Jsi_Value *mVal = Jsi_ValueObjLookup(interp, cmdPtr->mimeTypes, ext+1, 1);
                if (mVal && ((nmime = Jsi_ValueString(interp, mVal, NULL))))
                    mime = nmime;
            }
        }
        if (!buf[0]) {
            if (cmdPtr->debug)
                fprintf(stderr, "Unknown file: %s\n", inPtr);
            break;
        }
        Jsi_Value* fname = Jsi_ValueNewStringDup(interp, buf);
        Jsi_IncrRefCount(interp, fname);

        Jsi_StatBuf jsb;
        if (Jsi_Stat(interp, fname, &jsb)) {
nofile:
            if (cmdPtr->debug && !strstr(buf, "favicon.ico"))
                fprintf(stderr, "failed open for read: %s\n", buf);
            Jsi_DecrRefCount(interp, fname);
            rc = ServeString(cmdPtr, wsi, "<b style='color:red'>ERROR: can not serve file!</b>", 404, NULL, NULL);
            break;
        }
        if (S_ISDIR(jsb.st_mode)) {
            if (cmdPtr->debug)
                fprintf(stderr, "can not serve directory: %s\n", buf);
            rc = ServeString(cmdPtr, wsi, "<b style='color:red'>ERROR: can not serve directory!</b>", 404, NULL, NULL);
            Jsi_DecrRefCount(interp, fname);
            break;
        }
        if (Jsi_IsNative(interp, fname)) {
            if (libwebsockets_serve_http_file(context, wsi, buf, mime, NULL)) {
                Jsi_DecrRefCount(interp, fname);
                return 0;
            }
        } else {
            Jsi_Channel chan = Jsi_Open(interp, fname, "rb");
            int n, sum = 0;

            if (!chan) {
                goto nofile;
            }
            Jsi_DString dStr = {};
            char sbuf[BUFSIZ];
            Jsi_DSPrintf(&dStr,
                "HTTP/1.0 200 OK\x0d\x0a"
                "Server: libwebsockets\x0d\x0a"
                "Content-Type: %s\x0d\x0a"
                "Content-Length: %u\x0d\x0a\x0d\x0a",
                mime,
                (unsigned int)jsb.st_size);
            while (sum < 10000000 && (n = Jsi_Read(chan, sbuf, sizeof(sbuf))) > 0) {
                Jsi_DSAppendLen(&dStr, sbuf, n);
                sum += n;
            }
            Jsi_Close(chan);
            char *str = Jsi_DSValue(&dStr);
            int hrc, strLen = Jsi_DSLength(&dStr);
            Jsi_DecrRefCount(interp, fname);
            hrc = libwebsocket_write(wsi, (unsigned char*)str, strLen, LWS_WRITE_HTTP);
            Jsi_DSFree(&dStr);
            if (hrc)
                return 1;
        }
        /*
         * notice that the sending of the file completes asynchronously,
         * we'll get a LWS_CALLBACK_HTTP_FILE_COMPLETION callback when
         * it's done
         */

        break;
    }

    case LWS_CALLBACK_HTTP_FILE_COMPLETION:
//      lwsl_info("LWS_CALLBACK_HTTP_FILE_COMPLETION seen\n");
        /* kill the connection after we sent one file */
        return 1;

        /*
         * callback for confirming to continue with client IP appear in
         * protocol 0 callback since no websocket protocol has been agreed
         * yet.  You can just ignore this if you won't filter on client IP
         * since the default uhandled callback return is 0 meaning let the
         * connection continue.
         */

#ifdef EXTERNAL_POLL
        /*
         * callbacks for managing the external poll() array appear in
         * protocol 0 callback
         */

    case LWS_CALLBACK_ADD_POLL_FD:

        if (count_pollfds >= max_poll_elements) {
            lwsl_err("LWS_CALLBACK_ADD_POLL_FD: too many sockets to track\n");
            return 1;
        }

        fd_lookup[fd] = count_pollfds;
        pollfds[count_pollfds].fd = fd;
        pollfds[count_pollfds].events = (int)(long)len;
        pollfds[count_pollfds++].revents = 0;
        break;

    case LWS_CALLBACK_DEL_POLL_FD:
        if (!--count_pollfds)
            break;
        m = fd_lookup[fd];
        /* have the last guy take up the vacant slot */
        pollfds[m] = pollfds[count_pollfds];
        fd_lookup[pollfds[count_pollfds].fd] = m;
        break;

    case LWS_CALLBACK_SET_MODE_POLL_FD:
        pollfds[fd_lookup[fd]].events |= (int)(long)len;
        break;

    case LWS_CALLBACK_CLEAR_MODE_POLL_FD:
        pollfds[fd_lookup[fd]].events &= ~(int)(long)len;
        break;
#endif

    default:
        break;
    }

    return rc;
}

static Jsi_Value*
dump_handshake_info(WebsocketObj *cmdPtr, struct libwebsocket *wsi)
{
    int n;
    static const char *token_names[WSI_TOKEN_COUNT] = {
        /*[WSI_TOKEN_GET_URI]       =*/ "Uri",
        /*[WSI_TOKEN_POST_URI]      =*/ "POST URI",
        /*[WSI_TOKEN_HOST]      =*/ "Host",
        /*[WSI_TOKEN_CONNECTION]    =*/ "Connection",
        /*[WSI_TOKEN_KEY1]      =*/ "Key1",
        /*[WSI_TOKEN_KEY2]      =*/ "Key2",
        /*[WSI_TOKEN_PROTOCOL]      =*/ "Protocol",
        /*[WSI_TOKEN_UPGRADE]       =*/ "Upgrade",
        /*[WSI_TOKEN_ORIGIN]        =*/ "Origin",
        /*[WSI_TOKEN_DRAFT]     =*/ "Draft",
        /*[WSI_TOKEN_CHALLENGE]     =*/ "Challenge",

        /* new for 04 */
        /*[WSI_TOKEN_KEY]       =*/ "Key",
        /*[WSI_TOKEN_VERSION]       =*/ "Version",
        /*[WSI_TOKEN_SWORIGIN]      =*/ "Sworigin",

        /* new for 05 */
        /*[WSI_TOKEN_EXTENSIONS]    =*/ "Extensions",

        /* client receives these */
        /*[WSI_TOKEN_ACCEPT]        =*/ "Accept",
        /*[WSI_TOKEN_NONCE]     =*/ "Nonce",
        /*[WSI_TOKEN_HTTP]      =*/ "Http",

        "Accept:",
        "If-Modified-Since:",
        "Accept-Encoding:",
        "Accept-Language:",
        "Pragma:",
        "Cache-Control:",
        "Authorization:",
        "Cookie:",
        "Content-Length:",
        "Content-Type:",
        "Date:",
        "Range:",
        "Referer:",
        "Uri-Args:",
        
        /*[WSI_TOKEN_MUXURL]    =*/ "MuxURL",
    };
    char buf[BUFSIZ];
    int ntoks = sizeof(token_names)/sizeof(char*);
    Jsi_Interp *interp = cmdPtr->interp;
    Jsi_Obj *nobj = Jsi_ObjNewType(interp, JSI_OT_OBJECT);
    Jsi_Value *nv, *ret = Jsi_ValueMakeObject(interp, NULL, nobj);
    Jsi_ValueMakeObject(interp, ret, nobj);
   
    for (n = 0; n < ntoks; n++) {
        if (lws_hdr_total_length(wsi, n)<=0)
            continue;

        lws_hdr_copy(wsi, buf, sizeof(buf), n);
        buf[sizeof(buf)-1] = 0;
        nv = Jsi_ValueNewStringDup(interp, buf);
        Jsi_ObjInsert(interp, nobj, token_names[n], nv, 0);
    }
    return ret;
}

static int
callback_jsi_protocol(struct libwebsocket_context *context,
      struct libwebsocket *wsi,
      enum libwebsocket_callback_reasons reason,
      void *user, void *in, size_t len)
{
    WebsocketPss *pss = (WebsocketPss *)user;
    WebsocketObj *cmdPtr = libwebsocket_context_user(context);
    Jsi_Interp *interp = cmdPtr->interp;
    char *inPtr = (char*)in;
    int sLen, bSiz, n, rc =0; /*, result = JSI_OK;*/
#define LWSPAD (LWS_SEND_BUFFER_PRE_PADDING + LWS_SEND_BUFFER_POST_PADDING)
#define LBUFMAX (BUFSIZ+LWSPAD)
    char buf[LBUFMAX], *bufPtr = buf;
    static char *statBuf = NULL;
    static int statSize = 0;

    WSSIGASSERT(cmdPtr, OBJ);
    switch (reason) {
    case LWS_CALLBACK_PROTOCOL_INIT:
        if (cmdPtr->debug)
            fprintf(stderr, "WS:CALLBACK_INIT: %p\n", user);
        if (cmdPtr->noWebsock)
            return 1;
        //pss = getPss(cmdPtr, wsi, user, 1);
        break;
        
    case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
        //pss = getPss(cmdPtr, wsi, user, 1);
        if (cmdPtr->debug)
            fprintf(stderr, "WS:CALLBACK_FILTER: %p\n", pss);
        if (cmdPtr->onOpen) {
            int killcon = 0;
            Jsi_Obj *oarg1;
            Jsi_Value *vpargs, *vargs[2], *ret = Jsi_ValueNew1(interp);
            
            vargs[0] = dump_handshake_info(cmdPtr, wsi);
            vpargs = Jsi_ValueMakeObject(interp, NULL, oarg1 = Jsi_ObjNewArray(interp, vargs, 1));
            Jsi_IncrRefCount(interp, vpargs);
            Jsi_ValueMakeUndef(interp, ret);
            rc = Jsi_FunctionInvoke(interp, cmdPtr->onOpen, vpargs, &ret, NULL);
            if (Jsi_InterpGone(interp))
                return 1;
            if (rc == JSI_OK && Jsi_ValueIsFalse(interp, ret)) {
                if (cmdPtr->debug)
            fprintf(stderr, "WS:KILLING CONNECTION: %p\n", pss);
                killcon = 1;
            }

            Jsi_DecrRefCount(interp, vpargs);
            Jsi_DecrRefCount(interp, ret);
            if (rc != JSI_OK) {
                Jsi_LogError("websock bad rcv eval");
                return 1;
            }
            if (killcon)
                return 1;
        }

        /* you could return false here and kill the connection */
        break;

    case LWS_CALLBACK_CLIENT_ESTABLISHED:
    case LWS_CALLBACK_ESTABLISHED:
        pss = getPss(cmdPtr, wsi, user, 1);
        if (cmdPtr->debug)
            fprintf(stderr, "WS:CALLBACK_ESTABLISHED: %d,%p\n", pss->id, pss);
        break;

    case LWS_CALLBACK_CLOSED:
    case LWS_CALLBACK_PROTOCOL_DESTROY:
        pss = user;
        if (cmdPtr->debug)
            fprintf(stderr, "WS:CLOSE: %p\n", pss);
        if (!pss) break;
        if (cmdPtr->onClose) {
            /* Pass 2 args: data and id. */
            Jsi_Obj *oarg1;
            Jsi_Value *vpargs, *vargs[2];
            vargs[0] = Jsi_ValueNewNumber(interp, (Jsi_Number)(pss->id));
            vpargs = Jsi_ValueMakeObject(interp, NULL, oarg1 = Jsi_ObjNewArray(interp, vargs, 1));
            Jsi_IncrRefCount(interp, vpargs);
            
            Jsi_Value *ret = Jsi_ValueNew1(interp);
            Jsi_ValueMakeUndef(interp, ret);
            rc = Jsi_FunctionInvoke(interp, cmdPtr->onClose, vpargs, &ret, NULL);
            if (Jsi_InterpGone(interp))
                return JSI_ERROR;

            Jsi_DecrRefCount(interp, vpargs);
            Jsi_DecrRefCount(interp, ret);
            if (rc != JSI_OK) {
                Jsi_LogError("websock bad rcv eval");
                return JSI_ERROR;
            }
        }        
        deletePss(pss);
        if (cmdPtr->connectCnt<=0 && cmdPtr->onCloseLast) {
            Jsi_FunctionInvokeBool(interp, cmdPtr->onCloseLast, NULL);
            if (Jsi_InterpGone(interp))
                return JSI_ERROR;
        }
        break;
    case LWS_CALLBACK_CLIENT_WRITEABLE:
    case LWS_CALLBACK_SERVER_WRITEABLE:
        pss = getPss(cmdPtr, wsi, user, 0);
        if (!pss) break;
        n=0;
        while (1) {
            char *data = Jsi_StackPop(&pss->stack);
            unsigned char *p;
            if (data == NULL)
                break;
            pss->state = PSS_SENT;
            sLen = strlen(data);
            bSiz = sLen + LWSPAD;
            if (bSiz >= LBUFMAX) {
                if (statBuf == NULL) {
                    statSize = bSiz+1+LBUFMAX;
                    statBuf = Jsi_Malloc(statSize);
                } else if (statSize <= bSiz) {
                    statSize = bSiz+1+LBUFMAX;
                    statBuf = Jsi_Realloc(statBuf, statSize);
                }
                bufPtr = statBuf;
            }
            // TODO: check output size
            p = (unsigned char *)bufPtr+LWS_SEND_BUFFER_PRE_PADDING;
            memcpy(p, data, sLen);
            Jsi_Free(data);
            n = libwebsocket_write(wsi, p, sLen,LWS_WRITE_TEXT);
            if (cmdPtr->debug>=10)
                fprintf(stderr, "WS:CLIENT WRITE(%p): %d=>%d\n", pss, sLen, n);
                                   
            if (n >= 0) {
                pss->sentCnt++;
                pss->sentLast = time(NULL);
            } else {
                lwsl_err("ERROR %d writing to socket\n", n);
                pss->state = PSS_SENDERR;
                pss->sentErrCnt++;
                pss->sentErrLast = time(NULL);
                rc = 1;
                return rc;
            }

            // lwsl_debug("tx fifo %d\n", (ringbuffer_head - pss->ringbuffer_tail) & (MAX_MESSAGE_QUEUE - 1));

            /*if (lws_send_pipe_choked(wsi)) {
                    libwebsocket_callback_on_writable(context, wsi);
                    return 0;
            }*/
        }
        break;
        
    case LWS_CALLBACK_CLIENT_RECEIVE:
    case LWS_CALLBACK_RECEIVE:
    {
        int rc;
        pss = getPss(cmdPtr, wsi, user, 0);
        if (!pss) break;
        if (cmdPtr->debug>=10)
            fprintf(stderr, "WS:RECV: %p\n", pss);

        //fprintf(stderr, "rx %d\n", (int)len);
        pss->recvCnt++;
        pss->recvLast = time(NULL);

        if (cmdPtr->callback && !Jsi_ValueIsNull(interp, cmdPtr->callback)) {
            /* Pass 2 args: data and id. */
            Jsi_Obj *oarg1;
            Jsi_Value *vpargs, *vargs[2];
            vargs[0]  = Jsi_ValueNewStringDup(interp, inPtr);
            vargs[1] = Jsi_ValueNewNumber(interp, (Jsi_Number)(pss->id));
            vpargs = Jsi_ValueMakeObject(interp, NULL, oarg1 = Jsi_ObjNewArray(interp, vargs, 2));
            Jsi_IncrRefCount(interp, vpargs);
            
            Jsi_Value *ret = Jsi_ValueNew1(interp);
            Jsi_ValueMakeUndef(interp, ret);
            rc = Jsi_FunctionInvoke(interp, cmdPtr->callback, vpargs, &ret, NULL);
            if (Jsi_InterpGone(interp))
                return JSI_ERROR;
            if (rc == JSI_OK && Jsi_ValueIsUndef(interp, ret)==0) {
                /* TODO: handle callback return data??? */
            }

            Jsi_DecrRefCount(interp, vpargs);
            Jsi_DecrRefCount(interp, ret);
            if (rc != JSI_OK) {
                Jsi_LogError("websock bad rcv eval");
                return 1;
            }
        }
        //if (!strlen(Jsi_GetStringResult(interp))) { }
        libwebsocket_callback_on_writable_all_protocol(libwebsockets_get_protocol(wsi));

        //if (len < 6)
            //break;
        //if (Jsi_Strcmp((const char *)in, "reset\n") == 0)
        //pss->number = 0;
        break;
 
    }
    default:
        break;
    }
    return rc;
}


static int WebsocketConfCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    WebsocketObj *cmdPtr = Jsi_UserObjGetData(interp, _this, funcPtr);
  
    if (!cmdPtr) {
        Jsi_LogError("Apply in a non-websock object");
        return JSI_ERROR;
    }
    return Jsi_OptionsConf(interp, WSOptions, Jsi_ValueArrayIndex(interp, args, 0), cmdPtr, *ret, 0);

}


#define FN_wshandler JSI_INFO("\
With no args, returns handlers object.  With one, return value for a single handler."\
"Otherwise, sets the handler.")

static int WebsocketHandlerCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    WebsocketObj *cmdPtr = Jsi_UserObjGetData(interp, _this, funcPtr);
    Jsi_HashEntry *hPtr;
    if (!cmdPtr) {
        Jsi_LogError("Apply in a non-websock object");
        return JSI_ERROR;
    }
    WSSIGASSERT(cmdPtr, OBJ);
    int argc = Jsi_ValueGetLength(interp, args);
    if (argc == 0) {
        Jsi_HashSearch search;
        Jsi_Obj* obj = Jsi_ObjNew(interp);
        for (hPtr = Jsi_HashEntryFirst(cmdPtr->handlers, &search); hPtr; hPtr = Jsi_HashEntryNext(&search)) {
            const char *key = Jsi_HashKeyGet(hPtr);
            Jsi_Value *val = Jsi_HashValueGet(hPtr);
            Jsi_ObjInsert(interp, obj, key, val, 0);
        }
        Jsi_ValueMakeObject(interp, *ret, obj);
        return JSI_OK;
    }
    if (argc == 1) {
        hPtr = Jsi_HashEntryFind(cmdPtr->handlers, Jsi_ValueArrayIndexToStr(interp, args, 0, NULL));
        if (!hPtr)
            return JSI_OK;
        Jsi_Value *val = Jsi_HashValueGet(hPtr);
        Jsi_ValueCopy(interp, *ret, val);
        return JSI_OK;
    }
    const char *key = Jsi_ValueArrayIndexToStr(interp, args, 0, NULL);
    Jsi_Value *valPtr = Jsi_ValueArrayIndex(interp, args, 1);
    if (Jsi_ValueIsNull(interp, valPtr)) {
        hPtr = Jsi_HashEntryFind(cmdPtr->handlers, key);
        if (!hPtr)
            return JSI_OK;
        valPtr = Jsi_HashValueGet(hPtr);
        Jsi_DecrRefCount(interp, valPtr);
        Jsi_HashEntryDelete(hPtr);
        return JSI_OK;
    }
    if (Jsi_ValueIsFunction(interp, valPtr)==0 && Jsi_ValueIsString(interp, valPtr)==0) {
        Jsi_LogError("expected string, function or null");
        return JSI_ERROR;
    }
    hPtr = Jsi_HashEntryCreate(cmdPtr->handlers, key, NULL);
    if (!hPtr)
        return JSI_ERROR;
    Jsi_HashValueSet(hPtr, valPtr);
    Jsi_IncrRefCount(interp, valPtr);
    return JSI_OK;
}

#define FN_wssend JSI_INFO("\
Send a message to 1 (or all connections if -1). If not already a string, msg is format as JSON prior to the send.")

static int WebsocketSendCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    WebsocketObj *cmdPtr = Jsi_UserObjGetData(interp, _this, funcPtr);
    if (!cmdPtr) {
        Jsi_LogError("Apply in a non-websock object");
        return JSI_ERROR;
    }
    WSSIGASSERT(cmdPtr, OBJ);
    //int len;
    //char *in;
    //Jsi_HashEntry *hPtr;
    //Jsi_HashSearch cursor;
    //Jsi_Obj *objPtr;
    WebsocketPss *pss;
    Jsi_HashEntry *hPtr;
    Jsi_HashSearch cursor;
    Jsi_Value *arg = Jsi_ValueArrayIndex(interp, args, 0);
    char *str = Jsi_ValueString(interp, arg, NULL);
    int id = -1, argc = Jsi_ValueGetLength(interp, args);
    Jsi_DString eStr = {};
    if (argc>1) {
        Jsi_Number dnum;
        Jsi_Value *darg = Jsi_ValueArrayIndex(interp, args, 1);
        if (Jsi_ValueGetNumber(interp, darg, &dnum) != JSI_OK) {
            Jsi_LogError("invalid id");
            return JSI_ERROR;
        }
        id = (int)dnum;
    }
    if (!str) {
        str = (char*)Jsi_ValueGetDString(interp, arg, &eStr, JSI_OUTPUT_JSON);
    }
    for (hPtr = Jsi_HashEntryFirst(cmdPtr->pssTable, &cursor);
        hPtr != NULL; hPtr = Jsi_HashEntryNext(&cursor)) {
        pss = Jsi_HashValueGet(hPtr);
        WSSIGASSERT(pss, PSS);
        if (id<0 || pss->id == id)
            Jsi_StackPush(&pss->stack, Jsi_Strdup(str));
    }
   
    Jsi_DSFree(&eStr);
    return JSI_OK;
}

static int wsService(WebsocketObj *cmdPtr)
{
    int n = 0;
    struct timeval tv;

    gettimeofday(&tv, NULL);

    /*
     * This provokes the LWS_CALLBACK_SERVER_WRITEABLE for every
     * live websocket connection using the DUMB_INCREMENT protocol,
     * as soon as it can take more packets (usually immediately)
     */

    if (((unsigned int)tv.tv_usec - cmdPtr->oldus) > 50000) {
        libwebsocket_callback_on_writable_all_protocol(&cmdPtr->protocols[PROTOCOL_JSI]);
        cmdPtr->oldus = tv.tv_usec;
    }

#ifdef EXTERNAL_POLL

    /*
     * this represents an existing server's single poll action
     * which also includes libwebsocket sockets
     */

    n = poll(pollfds, count_pollfds, 50);
    if (n < 0)
        return 0;


    if (n)
        for (n = 0; n < count_pollfds; n++)
            if (pollfds[n].revents)
                /*
                * returns immediately if the fd does not
                * match anything under libwebsockets
                * control
                */
                if (libwebsocket_service_fd(context,
                                            &pollfds[n]) < 0)
                    return -1;
#else
    /*
     * If libwebsockets sockets are all we care about,
     * you can use this api which takes care of the poll()
     * and looping through finding who needed service.
     *
     * If no socket needs service, it'll return anyway after
     * the number of ms in the second argument.
     */

    n = libwebsocket_service(cmdPtr->context, 50);
#endif
    return n;
}

#define FN_wsupdate JSI_INFO("\
Update websocket queue.  This is used only in server mode to broadcast to clients.")

static int WebsocketUpdateCmd(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    WebsocketObj *cmdPtr = Jsi_UserObjGetData(interp, _this, funcPtr);
    Jsi_Value *oldcb = cmdPtr->callback;
    if (!cmdPtr) {
        Jsi_LogError("Apply websock object");
        return JSI_ERROR;
    }
    Jsi_Value *arg = Jsi_ValueArrayIndex(interp, args, 0);
    if (arg) {
        if (Jsi_ValueIsType(interp, arg, JSI_VT_OBJECT)==0 || Jsi_ValueIsObjType(interp, arg, JSI_OT_FUNCTION)==0) {
            Jsi_LogError("arg is not a function");
            return JSI_ERROR;
        }
        cmdPtr->callback = arg;
    }
    wsService(cmdPtr);
    if (arg)
        cmdPtr->callback = oldcb;
    return JSI_OK;
}

static int websockUpdate(Jsi_Interp *interp, void *data)
{
    WebsocketObj *cmdPtr = data;
    WSSIGASSERT(cmdPtr,OBJ);
    wsService(cmdPtr);
    return JSI_OK;
}

static void websocketObjErase(WebsocketObj *cmdPtr)
{
    if (cmdPtr->interp) {
        if (cmdPtr->event)
            Jsi_EventFree(cmdPtr->interp, cmdPtr->event);
        cmdPtr->event = NULL;
        if (cmdPtr->hasOpts)
            Jsi_OptionsFree(cmdPtr->interp, WSOptions, cmdPtr, 0);
        cmdPtr->hasOpts = 0;
        if (cmdPtr->handlers)
            Jsi_HashDelete(cmdPtr->handlers);
        cmdPtr->handlers = NULL;
    }
    cmdPtr->interp = NULL;
}

static int websocketObjFree(Jsi_Interp *interp, void *data)
{
    WebsocketObj *cmdPtr = data;
    WSSIGASSERT(cmdPtr,OBJ);
    if (cmdPtr->context) 
        libwebsocket_context_destroy(cmdPtr->context);
    websocketObjErase(cmdPtr);
    Jsi_Free(cmdPtr);
    return 0;
}

static int websocketObjIsTrue(void *data)
{
    //WebsocketObj *cmdPtr = data;
    return 1;
   /* if (!fo->websockname) return 0;
    else return 1;*/
}

static int websocketObjEqual(void *data1, void *data2)
{
    return (data1 == data2);
}

static int freeHandlers(Jsi_Interp *interp, void *ptr) {
    Jsi_Value *v = ptr;
    if (v)
        Jsi_DecrRefCount(interp, v);
    return JSI_OK;
}

#define FN_Websocket JSI_INFO("\
Create a websocket server or client object.  The server can serve pages out to a \
web browser and then use javascript to upgrade the connection to a bidirectional websocket.")
static int WebsocketConstructor(Jsi_Interp *interp, Jsi_Value *args, Jsi_Value *_this,
    Jsi_Value **ret, Jsi_Func *funcPtr)
{
    WebsocketObj *cmdPtr;
    Jsi_Value *toacc = NULL;
    Jsi_Value *arg = Jsi_ValueArrayIndex(interp, args, 0);
    
    if (Jsi_FunctionIsConstructor(funcPtr)) {
        toacc = _this;
    } else {
        Jsi_Obj *o = Jsi_ObjNew(interp);
        Jsi_PrototypeObjSet(interp, "Websocket", o);
        Jsi_ValueMakeObject(interp, *ret, o);
        toacc = *ret;
    }

    cmdPtr = Jsi_Calloc(1, sizeof(*cmdPtr));
    cmdPtr->sig = JWS_SIG_OBJ;
    cmdPtr->port = 8080;
    //cmdPtr->rootdir = "./";

    cmdPtr->interp = interp;
    cmdPtr->protocols[PROTOCOL_HTTP].name="http-only";
    cmdPtr->protocols[PROTOCOL_HTTP].callback=callback_http;
    cmdPtr->protocols[PROTOCOL_JSI].name="jsi-protocol";
    cmdPtr->protocols[PROTOCOL_JSI].callback=callback_jsi_protocol;
    cmdPtr->protocols[PROTOCOL_JSI].per_session_data_size=sizeof(WebsocketPss);
    cmdPtr->ietf_version = -1;
    cmdPtr->rx_buffer_size = 50000;
    cmdPtr->ws_gid = -1;
    cmdPtr->ws_uid = -1;
    cmdPtr->hasOpts = (arg != NULL && !Jsi_ValueIsNull(interp,arg));
    if (cmdPtr->hasOpts && Jsi_OptionsProcess(interp, WSOptions, arg, cmdPtr, 0) < 0) {
        cmdPtr->deleted = 1;
        //Jsi_EventuallyFree(cmdPtr, destroyWebsocketObj);
        return JSI_ERROR;
    }
    cmdPtr->protocols[PROTOCOL_JSI].rx_buffer_size=cmdPtr->rx_buffer_size;
    cmdPtr->pssTable = Jsi_HashNew(interp, JSI_KEYS_ONEWORD, NULL);
    cmdPtr->info.port = (cmdPtr->client ? CONTEXT_PORT_NO_LISTEN : cmdPtr->port);


#if !defined(LWS_NO_DAEMONIZE) && !defined(WIN32)
    /* 
     * normally lock path would be /var/lock/lwsts or similar, to
     * simplify getting started without having to take care about
     * permissions or running as root, set to /tmp/.lwsts-lock
     */
    if (cmdPtr->daemonize && lws_daemonize("/tmp/.lwsts-lock")) {
        if (cmdPtr->debug)
            fprintf(stderr, "WS:Failed to daemonize\n");
        return JSI_ERROR;
    }
#endif
    cmdPtr->info.user = cmdPtr;
    cmdPtr->info.iface = cmdPtr->interface ? Jsi_ValueString(interp, cmdPtr->interface, NULL) : NULL;
    cmdPtr->info.protocols = cmdPtr->protocols;
#ifndef LWS_NO_EXTENSIONS
    cmdPtr->info.extensions = libwebsocket_get_internal_extensions();
#endif
    cmdPtr->info.ssl_cert_filepath = cmdPtr->ssl_cert_filepath;
    cmdPtr->info.ssl_private_key_filepath = cmdPtr->ssl_private_key_filepath;
    cmdPtr->info.gid = cmdPtr->ws_gid;
    cmdPtr->info.uid = cmdPtr->ws_uid;
    cmdPtr->opts = LWS_SERVER_OPTION_SKIP_SERVER_CANONICAL_NAME;
    cmdPtr->info.options = cmdPtr->opts;

     lws_set_log_level(cmdPtr->debug>1?cmdPtr->debug-1:0, NULL);
     cmdPtr->context = libwebsocket_create_context(&cmdPtr->info);
     if (cmdPtr->context == NULL) {
        Jsi_LogError("libwebsocket init failed on port %d (try another port?)", cmdPtr->port);
        websocketObjFree(interp, cmdPtr);
        return JSI_ERROR;
    }

    if (cmdPtr->client) {
        if (NULL == libwebsocket_client_connect(cmdPtr->context,
            cmdPtr->address ? Jsi_ValueString(cmdPtr->interp, cmdPtr->address, NULL) : "127.0.0.1",
            cmdPtr->port, cmdPtr->use_ssl,
            cmdPtr->rootdir?Jsi_ValueString(cmdPtr->interp, cmdPtr->rootdir, NULL):"./",
            cmdPtr->cl_host?cmdPtr->cl_host:"localhost",
            cmdPtr->cl_origin?cmdPtr->cl_origin:"localhost",
             cmdPtr->protocols[PROTOCOL_JSI].name, cmdPtr->ietf_version)) {
            Jsi_LogError("websock connect failed");
            websocketObjFree(interp, cmdPtr);
            return JSI_ERROR;
        }
    }

    cmdPtr->event = Jsi_EventNew(interp, websockUpdate, cmdPtr);
    Jsi_Obj *fobj = Jsi_ValueGetObj(interp, toacc);
    if ((cmdPtr->objId = Jsi_UserObjNew(interp, &websockobject, fobj, cmdPtr))<0) {
        websocketObjFree(interp, cmdPtr);
        //Jsi_Free(cmdPtr); /* TODO: eliminate memory leak. */
        return JSI_ERROR;
    }
    cmdPtr->handlers = Jsi_HashNew(interp, JSI_KEYS_STRING, freeHandlers);
    cmdPtr->fobj = fobj;
    return JSI_OK;
}

static Jsi_CmdSpec websockCmds[] = {
    { "Websocket",  WebsocketConstructor, 0,  1, "?options?", JSI_CMD_IS_CONSTRUCTOR, .help="Create websocket server/client object", .opts=WSOptions, .info=FN_Websocket },
    { "conf",       WebsocketConfCmd,     0,  1, "?string|options?",.help="Configure options" , .opts=WSOptions},
    { "handler",    WebsocketHandlerCmd,  0,  2, "?ext??,cmd?", .help="Get/Set handler command for an extension", .info=FN_wshandler },
    { "send",       WebsocketSendCmd,     1,  2, "data?,id?", .help="Send a websocket message to id", .info=FN_wssend },
    { "update",     WebsocketUpdateCmd,   0,  1, "?callback?", .help="Service just websocket events", .info=FN_wsupdate },
    { NULL, .help="Commands for managing Websocket server/client connections"  }
};


int Jsi_DoneWebsocket(Jsi_Interp *interp)
{
    Jsi_UserObjUnregister(interp, &websockobject);
    return JSI_OK;
}

int Jsi_InitWebsocket(Jsi_Interp *interp)
{
    Jsi_Hash *wsys;
#ifdef JSI_USE_STUBS
  if (Jsi_StubsInit(interp, 0) != JSI_OK)
    return JSI_ERROR;
#endif
    if (!(wsys = Jsi_UserObjRegister(interp, &websockobject))) {
        Jsi_LogFatal("Can not init websock\n");
        return JSI_ERROR;
    }

    if (!Jsi_CommandCreateSpecs(interp, websockobject.name, websockCmds, wsys, 0))
        return JSI_ERROR;
    return JSI_OK;
}

#endif
#endif
