/************************************************

  error.c -

  $Author$
  $Date$
  created at: Mon Aug  9 16:11:34 JST 1993

  Copyright (C) 1993-1998 Yukihiro Matsumoto

************************************************/

#include "ruby.h"
#include "env.h"
#include <stdio.h>
#include <varargs.h>

extern char *sourcefile;
extern int   sourceline;

int nerrs;

static void
err_sprintf(buf, fmt, args)
    char *buf, *fmt;
    va_list args;
{
    if (!sourcefile) {
	vsprintf(buf, fmt, args);
    }
    else {
	sprintf(buf, "%s:%d: ", sourcefile, sourceline);
	vsprintf((char*)buf+strlen(buf), fmt, args);
    }
}

static void
err_append(s)
    char *s;
{
    extern VALUE errinfo;

    if (rb_in_eval) {
	if (NIL_P(errinfo)) {
	    errinfo = str_new2(s);
	}
	else {
	    str_cat(errinfo, "\n", 1);
	    str_cat(errinfo, s, strlen(s));
	}
    }
    else {
	fputs(s, stderr);
	fputs("\n", stderr);
	fflush(stderr);
    }
}

static void
err_print(fmt, args)
    char *fmt;
    va_list args;
{
    char buf[BUFSIZ];

    err_sprintf(buf, fmt, args);
    err_append(buf);
}

void
Error(fmt, va_alist)
    char *fmt;
    va_dcl
{
    va_list args;

    va_start(args);
    err_print(fmt, args);
    va_end(args);
    nerrs++;
}

void
Error_Append(fmt, va_alist)
    char *fmt;
    va_dcl
{
    va_list args;
    char buf[BUFSIZ];

    va_start(args);
    vsprintf(buf, fmt, args);
    va_end(args);
    err_append(buf);
}

void
Warn(fmt, va_alist)
    char *fmt;
    va_dcl
{
    char buf[BUFSIZ];
    va_list args;

    sprintf(buf, "warning: %s", fmt);

    va_start(args);
    err_print(buf, args);
    va_end(args);
}

/* Warning() reports only in verbose mode */
void
Warning(fmt, va_alist)
    char *fmt;
    va_dcl
{
    char buf[BUFSIZ];
    va_list args;

    if (!RTEST(verbose)) return;

    sprintf(buf, "warning: %s", fmt);

    va_start(args);
    err_print(buf, args);
    va_end(args);
}

void
Bug(fmt, va_alist)
    char *fmt;
    va_dcl
{
    char buf[BUFSIZ];
    va_list args;

    sprintf(buf, "[BUG] %s", fmt);
    rb_in_eval = 0;

    va_start(args);
    err_print(buf, args);
    va_end(args);
    abort();
}

static struct types {
    int type;
    char *name;
} builtin_types[] = {
    T_NIL,	"nil",
    T_OBJECT,	"Object",
    T_CLASS,	"Class",
    T_ICLASS,	"iClass",	/* internal use: mixed-in module holder */
    T_MODULE,	"Module",
    T_FLOAT,	"Float",
    T_STRING,	"String",
    T_REGEXP,	"Regexp",
    T_ARRAY,	"Array",
    T_FIXNUM,	"Fixnum",
    T_HASH,	"Hash",
    T_STRUCT,	"Struct",
    T_BIGNUM,	"Bignum",
    T_FILE,	"File",
    T_TRUE,	"TRUE",
    T_FALSE,	"FALSE",
    T_DATA,	"Data",		/* internal use: wrapped C pointers */
    T_MATCH,	"Match",	/* data of $~ */
    T_VARMAP,	"Varmap",	/* internal use: dynamic variables */
    T_SCOPE,	"Scope",	/* internal use: variable scope */
    T_NODE,	"Node",		/* internal use: syntax tree node */
    -1,		0,
};

extern void TypeError();

void
rb_check_type(x, t)
    VALUE x;
    int t;
{
    struct types *type = builtin_types;
    int tt = TYPE(x);

    if (tt != t) {
	while (type->type >= 0) {
	    if (type->type == t) {
		char *etype;

		if (NIL_P(x)) {
		    etype = "nil";
		}
		else if (FIXNUM_P(x)) {
		    etype = "Fixnum";
		}
		else if (rb_special_const_p(x)) {
		    etype = RSTRING(obj_as_string(x))->ptr;
		}
		else {
		    etype = rb_class2name(CLASS_OF(x));
		}
		TypeError("wrong argument type %s (expected %s)",
			  etype, type->name);
	    }
	    type++;
	}
	Bug("unknown type 0x%x", t);
    }
}

/* exception classes */
#include "errno.h"

extern VALUE cString;
VALUE eException;
VALUE eSystemExit, eInterrupt, eFatal;
VALUE eStandardError;
VALUE eRuntimeError;
VALUE eSyntaxError;
VALUE eTypeError;
VALUE eArgError;
VALUE eNameError;
VALUE eIndexError;
VALUE eLoadError;
VALUE eSecurityError;
VALUE eNotImpError;

VALUE eSystemCallError;
VALUE mErrno;

VALUE
exc_new(etype, ptr, len)
    VALUE etype;
    char *ptr;
    unsigned len;
{
    VALUE exc = obj_alloc(etype);

    rb_iv_set(exc, "mesg", str_new(ptr, len));
    return exc;
}

VALUE
exc_new2(etype, s)
    VALUE etype;
    char *s;
{
    return exc_new(etype, s, strlen(s));
}

VALUE
exc_new3(etype, str)
    VALUE etype, str;
{
    char *s;
    int len;

    s = str2cstr(str, &len);
    return exc_new(etype, s, len);
}

static VALUE
exc_initialize(argc, argv, exc)
    int argc;
    VALUE *argv;
    VALUE exc;
{
    VALUE mesg;

    rb_scan_args(argc, argv, "01", &mesg);
    if (NIL_P(mesg)) {
	mesg = str_new(0, 0);
    }
    else {
	STR2CSTR(mesg);		/* ensure mesg can be converted to String */
    }
    rb_iv_set(exc, "mesg", mesg);

    return exc;
}

static VALUE
exc_new_method(argc, argv, self)
    int argc;
    VALUE *argv;
    VALUE self;
{
    VALUE etype, exc;

    if (argc == 1 && self == argv[0]) return self;
    etype = CLASS_OF(self);
    while (FL_TEST(etype, FL_SINGLETON)) {
	etype = RCLASS(etype)->super;
    }
    exc = obj_alloc(etype);
    obj_call_init(exc);

    return exc;
}

static VALUE
exc_to_s(exc)
    VALUE exc;
{
    return rb_iv_get(exc, "mesg");
}

static VALUE
exc_inspect(exc)
    VALUE exc;
{
    VALUE str, klass;

    klass = CLASS_OF(exc);
    exc = obj_as_string(exc);
    if (RSTRING(exc)->len == 0) {
	return str_dup(rb_class_path(klass));
    }

    str = str_new2("#<");
    klass = rb_class_path(klass);
    str_concat(str, klass);
    str_cat(str, ":", 1);
    str_concat(str, exc);
    str_cat(str, ">", 1);

    return str;
}

static VALUE
exc_backtrace(exc)
    VALUE exc;
{
    return rb_iv_get(exc, "bt");
}

static VALUE
check_backtrace(bt)
    VALUE bt;
{
    int i;
    static char *err = "backtrace must be Array of String";

    if (!NIL_P(bt)) {
	int t = TYPE(bt);

	if (t == T_STRING) return ary_new3(1, bt);
	if (t != T_ARRAY) {
	    TypeError(err);
	}
	for (i=0;i<RARRAY(bt)->len;i++) {
	    if (TYPE(RARRAY(bt)->ptr[i]) != T_STRING) {
		TypeError(err);
	    }
	}
    }
    return bt;
}

static VALUE
exc_set_backtrace(exc, bt)
    VALUE exc;
{
    return rb_iv_set(exc, "bt", check_backtrace(bt));
}

static VALUE
exception(argc, argv)
    int argc;
    VALUE *argv;
{
    void ArgError();
    VALUE v = Qnil;
    VALUE etype = eStandardError;
    int i;
    ID id;

    if (argc == 0) {
	ArgError("wrong # of arguments");
    }
    Warn("Exception() is now obsolete");
    if (TYPE(argv[argc-1]) == T_CLASS) {
	etype = argv[argc-1];
	argc--;
	if (!rb_funcall(etype, '<', 1, eException)) {
	    TypeError("exception should be subclass of Exception");
	}
    }
    for (i=0; i<argc; i++) {	/* argument check */
	id = rb_to_id(argv[i]);
	if (!rb_is_const_id(id)) {
	    ArgError("identifier `%s' needs to be constant", rb_id2name(id));
	}
	if (!rb_id2name(id)) {
	    ArgError("argument needs to be symbol or string");
	}
    }
    for (i=0; i<argc; i++) {
	v = rb_define_class_under(the_class,
				  rb_id2name(rb_to_id(argv[i])),
				  eStandardError);
    }
    return v;
}

static VALUE *syserr_list;

#ifndef NT
extern int sys_nerr;
#endif

static VALUE
set_syserr(i, name)
    int i;
    char *name;
{
    VALUE error = rb_define_class_under(mErrno, name, eSystemCallError);
    rb_define_const(error, "Errno", INT2FIX(i));
    if (i <= sys_nerr) {
	syserr_list[i] = error;
    }
    return error;
}

static VALUE
syserr_errno(self)
    VALUE self;
{
    return rb_iv_get(self, "errno");
}

static void init_syserr();

void
Init_Exception()
{
    eException   = rb_define_class("Exception", cObject);
    rb_define_method(eException, "new", exc_new_method, -1);
    rb_define_method(eException, "initialize", exc_initialize, -1);
    rb_define_method(eException, "to_s", exc_to_s, 0);
    rb_define_method(eException, "to_str", exc_to_s, 0);
    rb_define_method(eException, "message", exc_to_s, 0);
    rb_define_method(eException, "inspect", exc_inspect, 0);
    rb_define_method(eException, "backtrace", exc_backtrace, 0);
    rb_define_method(eException, "set_backtrace", exc_set_backtrace, 1);

    eSystemExit  = rb_define_class("SystemExit", eException);
    eFatal  	 = rb_define_class("fatal", eException);
    eInterrupt   = rb_define_class("Interrupt", eException);

    eStandardError = rb_define_class("StandardError", eException);
    eSyntaxError = rb_define_class("SyntaxError", eStandardError);
    eTypeError   = rb_define_class("TypeError", eStandardError);
    eArgError    = rb_define_class("ArgumentError", eStandardError);
    eNameError   = rb_define_class("NameError", eStandardError);
    eIndexError  = rb_define_class("IndexError", eStandardError);
    eLoadError   = rb_define_class("LoadError", eStandardError);

    eRuntimeError = rb_define_class("RuntimeError", eStandardError);
    eSecurityError = rb_define_class("SecurityError", eStandardError);
    eNotImpError = rb_define_class("NotImplementError", eException);

    init_syserr();

    rb_define_global_function("Exception", exception, -1);
}

#define RAISE_ERROR(klass) {\
    va_list args;\
    char buf[BUFSIZ];\
\
    va_start(args);\
    vsprintf(buf, fmt, args);\
    va_end(args);\
\
    rb_raise(exc_new2(klass, buf));\
}

void
Raise(exc, fmt, va_alist)
    VALUE exc;
    char *fmt;
    va_dcl
{
    RAISE_ERROR(exc);
}

void
TypeError(fmt, va_alist)
    char *fmt;
    va_dcl
{
    RAISE_ERROR(eTypeError);
}

void
ArgError(fmt, va_alist)
    char *fmt;
    va_dcl
{
    RAISE_ERROR(eArgError);
}

void
NameError(fmt, va_alist)
    char *fmt;
    va_dcl
{
    RAISE_ERROR(eNameError);
}

void
IndexError(fmt, va_alist)
    char *fmt;
    va_dcl
{
    RAISE_ERROR(eIndexError);
}

void
Fail(fmt, va_alist)
    char *fmt;
    va_dcl
{
    RAISE_ERROR(eRuntimeError);
}

void
rb_notimplement()
{
    Raise(eNotImpError,
	  "The %s() function is unimplemented on this machine",
	  rb_id2name(the_frame->last_func));
}

void
LoadError(fmt, va_alist)
    char *fmt;
    va_dcl
{
    RAISE_ERROR(eLoadError);
}

void
Fatal(fmt, va_alist)
    char *fmt;
    va_dcl
{
    va_list args;
    char buf[BUFSIZ];

    va_start(args);
    vsprintf(buf, fmt, args);
    va_end(args);

    rb_in_eval = 0;
    rb_fatal(exc_new2(eFatal, buf));
}

void
rb_sys_fail(mesg)
    char *mesg;
{
#ifndef NT
    char *strerror();
#endif
    char *err;
    char *buf;
    extern int errno;
    int n = errno;
    VALUE ee;

    err = strerror(errno);
    if (mesg) {
	buf = ALLOCA_N(char, strlen(err)+strlen(mesg)+1);
	sprintf(buf, "%s - %s", err, mesg);
    }
    else {
	buf = ALLOCA_N(char, strlen(err)+1);
	sprintf(buf, "%s", err);
    }

    errno = 0;
    if (n > sys_nerr || !syserr_list[n]) {
	char name[6];

	sprintf(name, "E%03d", n);
	ee = set_syserr(n, name);
    }
    else {
	ee = syserr_list[n];
    }
    ee = exc_new2(ee, buf);
    rb_iv_set(ee, "errno", INT2FIX(n));
    rb_raise(ee);
}

static void
init_syserr()
{
    eSystemCallError = rb_define_class("SystemCallError", eStandardError);
    rb_define_method(eSystemCallError, "errno", syserr_errno, 0);

    mErrno = rb_define_module("Errno");
    syserr_list = ALLOC_N(VALUE, sys_nerr+1);
    MEMZERO(syserr_list, VALUE, sys_nerr+1);

#ifdef EPERM
    set_syserr(EPERM, "EPERM");
#endif
#ifdef ENOENT
    set_syserr(ENOENT, "ENOENT");
#endif
#ifdef ESRCH
    set_syserr(ESRCH, "ESRCH");
#endif
#ifdef EINTR
    set_syserr(EINTR, "EINTR");
#endif
#ifdef EIO
    set_syserr(EIO, "EIO");
#endif
#ifdef ENXIO
    set_syserr(ENXIO, "ENXIO");
#endif
#ifdef E2BIG
    set_syserr(E2BIG, "E2BIG");
#endif
#ifdef ENOEXEC
    set_syserr(ENOEXEC, "ENOEXEC");
#endif
#ifdef EBADF
    set_syserr(EBADF, "EBADF");
#endif
#ifdef ECHILD
    set_syserr(ECHILD, "ECHILD");
#endif
#ifdef EAGAIN
    set_syserr(EAGAIN, "EAGAIN");
#endif
#ifdef ENOMEM
    set_syserr(ENOMEM, "ENOMEM");
#endif
#ifdef EACCES
    set_syserr(EACCES, "EACCES");
#endif
#ifdef EFAULT
    set_syserr(EFAULT, "EFAULT");
#endif
#ifdef ENOTBLK
    set_syserr(ENOTBLK, "ENOTBLK");
#endif
#ifdef EBUSY
    set_syserr(EBUSY, "EBUSY");
#endif
#ifdef EEXIST
    set_syserr(EEXIST, "EEXIST");
#endif
#ifdef EXDEV
    set_syserr(EXDEV, "EXDEV");
#endif
#ifdef ENODEV
    set_syserr(ENODEV, "ENODEV");
#endif
#ifdef ENOTDIR
    set_syserr(ENOTDIR, "ENOTDIR");
#endif
#ifdef EISDIR
    set_syserr(EISDIR, "EISDIR");
#endif
#ifdef EINVAL
    set_syserr(EINVAL, "EINVAL");
#endif
#ifdef ENFILE
    set_syserr(ENFILE, "ENFILE");
#endif
#ifdef EMFILE
    set_syserr(EMFILE, "EMFILE");
#endif
#ifdef ENOTTY
    set_syserr(ENOTTY, "ENOTTY");
#endif
#ifdef ETXTBSY
    set_syserr(ETXTBSY, "ETXTBSY");
#endif
#ifdef EFBIG
    set_syserr(EFBIG, "EFBIG");
#endif
#ifdef ENOSPC
    set_syserr(ENOSPC, "ENOSPC");
#endif
#ifdef ESPIPE
    set_syserr(ESPIPE, "ESPIPE");
#endif
#ifdef EROFS
    set_syserr(EROFS, "EROFS");
#endif
#ifdef EMLINK
    set_syserr(EMLINK, "EMLINK");
#endif
#ifdef EPIPE
    set_syserr(EPIPE, "EPIPE");
#endif
#ifdef EDOM
    set_syserr(EDOM, "EDOM");
#endif
#ifdef ERANGE
    set_syserr(ERANGE, "ERANGE");
#endif
#ifdef EDEADLK
    set_syserr(EDEADLK, "EDEADLK");
#endif
#ifdef ENAMETOOLONG
    set_syserr(ENAMETOOLONG, "ENAMETOOLONG");
#endif
#ifdef ENOLCK
    set_syserr(ENOLCK, "ENOLCK");
#endif
#ifdef ENOSYS
    set_syserr(ENOSYS, "ENOSYS");
#endif
#ifdef ENOTEMPTY
    set_syserr(ENOTEMPTY, "ENOTEMPTY");
#endif
#ifdef ELOOP
    set_syserr(ELOOP, "ELOOP");
#endif
#ifdef EWOULDBLOCK
    set_syserr(EWOULDBLOCK, "EWOULDBLOCK");
#endif
#ifdef ENOMSG
    set_syserr(ENOMSG, "ENOMSG");
#endif
#ifdef EIDRM
    set_syserr(EIDRM, "EIDRM");
#endif
#ifdef ECHRNG
    set_syserr(ECHRNG, "ECHRNG");
#endif
#ifdef EL2NSYNC
    set_syserr(EL2NSYNC, "EL2NSYNC");
#endif
#ifdef EL3HLT
    set_syserr(EL3HLT, "EL3HLT");
#endif
#ifdef EL3RST
    set_syserr(EL3RST, "EL3RST");
#endif
#ifdef ELNRNG
    set_syserr(ELNRNG, "ELNRNG");
#endif
#ifdef EUNATCH
    set_syserr(EUNATCH, "EUNATCH");
#endif
#ifdef ENOCSI
    set_syserr(ENOCSI, "ENOCSI");
#endif
#ifdef EL2HLT
    set_syserr(EL2HLT, "EL2HLT");
#endif
#ifdef EBADE
    set_syserr(EBADE, "EBADE");
#endif
#ifdef EBADR
    set_syserr(EBADR, "EBADR");
#endif
#ifdef EXFULL
    set_syserr(EXFULL, "EXFULL");
#endif
#ifdef ENOANO
    set_syserr(ENOANO, "ENOANO");
#endif
#ifdef EBADRQC
    set_syserr(EBADRQC, "EBADRQC");
#endif
#ifdef EBADSLT
    set_syserr(EBADSLT, "EBADSLT");
#endif
#ifdef EDEADLOCK
    set_syserr(EDEADLOCK, "EDEADLOCK");
#endif
#ifdef EBFONT
    set_syserr(EBFONT, "EBFONT");
#endif
#ifdef ENOSTR
    set_syserr(ENOSTR, "ENOSTR");
#endif
#ifdef ENODATA
    set_syserr(ENODATA, "ENODATA");
#endif
#ifdef ETIME
    set_syserr(ETIME, "ETIME");
#endif
#ifdef ENOSR
    set_syserr(ENOSR, "ENOSR");
#endif
#ifdef ENONET
    set_syserr(ENONET, "ENONET");
#endif
#ifdef ENOPKG
    set_syserr(ENOPKG, "ENOPKG");
#endif
#ifdef EREMOTE
    set_syserr(EREMOTE, "EREMOTE");
#endif
#ifdef ENOLINK
    set_syserr(ENOLINK, "ENOLINK");
#endif
#ifdef EADV
    set_syserr(EADV, "EADV");
#endif
#ifdef ESRMNT
    set_syserr(ESRMNT, "ESRMNT");
#endif
#ifdef ECOMM
    set_syserr(ECOMM, "ECOMM");
#endif
#ifdef EPROTO
    set_syserr(EPROTO, "EPROTO");
#endif
#ifdef EMULTIHOP
    set_syserr(EMULTIHOP, "EMULTIHOP");
#endif
#ifdef EDOTDOT
    set_syserr(EDOTDOT, "EDOTDOT");
#endif
#ifdef EBADMSG
    set_syserr(EBADMSG, "EBADMSG");
#endif
#ifdef EOVERFLOW
    set_syserr(EOVERFLOW, "EOVERFLOW");
#endif
#ifdef ENOTUNIQ
    set_syserr(ENOTUNIQ, "ENOTUNIQ");
#endif
#ifdef EBADFD
    set_syserr(EBADFD, "EBADFD");
#endif
#ifdef EREMCHG
    set_syserr(EREMCHG, "EREMCHG");
#endif
#ifdef ELIBACC
    set_syserr(ELIBACC, "ELIBACC");
#endif
#ifdef ELIBBAD
    set_syserr(ELIBBAD, "ELIBBAD");
#endif
#ifdef ELIBSCN
    set_syserr(ELIBSCN, "ELIBSCN");
#endif
#ifdef ELIBMAX
    set_syserr(ELIBMAX, "ELIBMAX");
#endif
#ifdef ELIBEXEC
    set_syserr(ELIBEXEC, "ELIBEXEC");
#endif
#ifdef EILSEQ
    set_syserr(EILSEQ, "EILSEQ");
#endif
#ifdef ERESTART
    set_syserr(ERESTART, "ERESTART");
#endif
#ifdef ESTRPIPE
    set_syserr(ESTRPIPE, "ESTRPIPE");
#endif
#ifdef EUSERS
    set_syserr(EUSERS, "EUSERS");
#endif
#ifdef ENOTSOCK
    set_syserr(ENOTSOCK, "ENOTSOCK");
#endif
#ifdef EDESTADDRREQ
    set_syserr(EDESTADDRREQ, "EDESTADDRREQ");
#endif
#ifdef EMSGSIZE
    set_syserr(EMSGSIZE, "EMSGSIZE");
#endif
#ifdef EPROTOTYPE
    set_syserr(EPROTOTYPE, "EPROTOTYPE");
#endif
#ifdef ENOPROTOOPT
    set_syserr(ENOPROTOOPT, "ENOPROTOOPT");
#endif
#ifdef EPROTONOSUPPORT
    set_syserr(EPROTONOSUPPORT, "EPROTONOSUPPORT");
#endif
#ifdef ESOCKTNOSUPPORT
    set_syserr(ESOCKTNOSUPPORT, "ESOCKTNOSUPPORT");
#endif
#ifdef EOPNOTSUPP
    set_syserr(EOPNOTSUPP, "EOPNOTSUPP");
#endif
#ifdef EPFNOSUPPORT
    set_syserr(EPFNOSUPPORT, "EPFNOSUPPORT");
#endif
#ifdef EAFNOSUPPORT
    set_syserr(EAFNOSUPPORT, "EAFNOSUPPORT");
#endif
#ifdef EADDRINUSE
    set_syserr(EADDRINUSE, "EADDRINUSE");
#endif
#ifdef EADDRNOTAVAIL
    set_syserr(EADDRNOTAVAIL, "EADDRNOTAVAIL");
#endif
#ifdef ENETDOWN
    set_syserr(ENETDOWN, "ENETDOWN");
#endif
#ifdef ENETUNREACH
    set_syserr(ENETUNREACH, "ENETUNREACH");
#endif
#ifdef ENETRESET
    set_syserr(ENETRESET, "ENETRESET");
#endif
#ifdef ECONNABORTED
    set_syserr(ECONNABORTED, "ECONNABORTED");
#endif
#ifdef ECONNRESET
    set_syserr(ECONNRESET, "ECONNRESET");
#endif
#ifdef ENOBUFS
    set_syserr(ENOBUFS, "ENOBUFS");
#endif
#ifdef EISCONN
    set_syserr(EISCONN, "EISCONN");
#endif
#ifdef ENOTCONN
    set_syserr(ENOTCONN, "ENOTCONN");
#endif
#ifdef ESHUTDOWN
    set_syserr(ESHUTDOWN, "ESHUTDOWN");
#endif
#ifdef ETOOMANYREFS
    set_syserr(ETOOMANYREFS, "ETOOMANYREFS");
#endif
#ifdef ETIMEDOUT
    set_syserr(ETIMEDOUT, "ETIMEDOUT");
#endif
#ifdef ECONNREFUSED
    set_syserr(ECONNREFUSED, "ECONNREFUSED");
#endif
#ifdef EHOSTDOWN
    set_syserr(EHOSTDOWN, "EHOSTDOWN");
#endif
#ifdef EHOSTUNREACH
    set_syserr(EHOSTUNREACH, "EHOSTUNREACH");
#endif
#ifdef EALREADY
    set_syserr(EALREADY, "EALREADY");
#endif
#ifdef EINPROGRESS
    set_syserr(EINPROGRESS, "EINPROGRESS");
#endif
#ifdef ESTALE
    set_syserr(ESTALE, "ESTALE");
#endif
#ifdef EUCLEAN
    set_syserr(EUCLEAN, "EUCLEAN");
#endif
#ifdef ENOTNAM
    set_syserr(ENOTNAM, "ENOTNAM");
#endif
#ifdef ENAVAIL
    set_syserr(ENAVAIL, "ENAVAIL");
#endif
#ifdef EISNAM
    set_syserr(EISNAM, "EISNAM");
#endif
#ifdef EREMOTEIO
    set_syserr(EREMOTEIO, "EREMOTEIO");
#endif
#ifdef EDQUOT
    set_syserr(EDQUOT, "EDQUOT");
#endif
}
