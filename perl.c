#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

#include <gnumake.h>

int plugin_is_GPL_compatible;

static PerlInterpreter *my_perl;
static HV *gmk_funcs;
static void init_perl();

static char *
func_perl(const char *funcname, unsigned int argc, char **argv)
{
    char *str;

    if (!my_perl)
	init_perl();

    if (argv[0] && argv[0][0] != '\0') {
	PERL_DEBUG(PerlIO_printf(Perl_debug_log, "perl: '%s'\n", argv[0]));
	SV *sv = eval_pv(argv[0], FALSE);
	char *val = SvANY(sv) ? SvPV_nolen(sv) : "";
	if (SvTRUE(ERRSV)) {
	    printf(SvPV_nolen(ERRSV));
	    return NULL;
	}
	str = gmk_alloc(strlen(val));
	strcpy (str, val);
	return str;
    }

    return NULL;
}

static char *
func_perl_load(const char *funcname, unsigned int argc, char **argv)
{
    PerlIO *fp;
    SV *sv;
    struct stat st;
    if (!my_perl)
	init_perl();

    if (argv[0] && argv[0][0] != '\0') {
	PERL_DEBUG(PerlIO_printf(Perl_debug_log, "loading: '%s'\n", argv[0]));
#if 0
	if (stat(argv[0], &st)) {
	    printf("stat() failed: %d\n", errno);
	    return NULL;
	}

	fp = PerlIO_open(argv[0], "r");
	if (fp == NULL) {
	    printf("Couldn't open file: %s..\n", argv[0]);
	    return NULL;
	}

	sv = newSV(st.st_size);
	sv_gets(sv, fp, 0);
	PerlIO_close(fp);
	eval_sv(sv, G_EVAL);
#endif
	require_pv(argv[0]);
	if (SvTRUE(ERRSV)) {
	    printf(SvPV_nolen(ERRSV));
	}
    }

    return NULL;
}

static char *
func_perl_free(const char *funcname, unsigned int argc, char **argv)
{
    PERL_DEBUG(PerlIO_printf(Perl_debug_log, "func_perl_free\n"));
    if (my_perl) {
	PERL_DEBUG(PerlIO_printf(Perl_debug_log, "freeing!\n"));
	perl_destruct(my_perl);
	perl_free(my_perl);
	PERL_SYS_TERM();
	my_perl = NULL;
    }
    return NULL;
}

static char *
func_perl_call_wrapper(const char *funcname, unsigned int argc, char **argv)
{
    dSP;
    int count;
    char *res = NULL;
    PERL_DEBUG(PerlIO_printf(Perl_debug_log, "func_perl_call_wrapper\n"));

    if (!my_perl)
	init_perl();

    ENTER;
    SAVETMPS;

    PUSHMARK(SP);
    for (int i = 0; i < argc; ++i) {
	XPUSHs(sv_2mortal(newSVpv(argv[i], 0)));
    }
    PUTBACK;

    {
	SV *sv = NULL;
	SV **svp = hv_fetch(gmk_funcs, funcname, strlen(funcname), 0);
	if (svp && *svp) {
	    if (SvROK(*svp) && SvTYPE(SvRV(*svp)) == SVt_PVCV)
		sv = *svp;
	    else if (SvTYPE(*svp) == SVt_PV)
		funcname = SvPV_nolen(*svp);
	}
	if (sv)
	    count = call_sv(sv, G_EVAL|G_SCALAR);
	else
	    count = call_pv(funcname, G_EVAL|G_SCALAR);
    }
    SPAGAIN;

    if (SvTRUE(ERRSV)) {
	printf("call_pv() failed: %s\n", SvPV_nolen(ERRSV));
	POPs;

    } else if (count != 1) {
	printf("Big trouble\n");

    } else {
	SV *sv = POPs;
	STRLEN len;
	char *str = SvPV(sv, len);
	res = gmk_alloc(len);
	strcpy(res, str);
    }

    PUTBACK;
    FREETMPS;
    LEAVE;

    return res;
}

XS(XS_gmk_expand)
{
    dXSARGS;
    if (items != 1)
	printf("Usage: gmk_expand(arg)\n");

    PERL_UNUSED_VAR(cv);
    {
	char *str = SvPV_nolen(ST(0));
	char *res = gmk_expand(str);
	PERL_DEBUG(PerlIO_printf(Perl_debug_log, "expand: '%s'\n", str));
	ST(0) = sv_2mortal(newSVpv(res, 0));
	if (res)
	    gmk_free(res);
    }
    XSRETURN(1);
}

XS(XS_gmk_eval)
{
    dXSARGS;
    if (items != 1)
	printf("Usage: gmk_eval(arg)\n");

    PERL_UNUSED_VAR(cv);
    {
	char *str = SvPV_nolen(ST(0));
	PERL_DEBUG(PerlIO_printf(Perl_debug_log, "eval: '%s'\n", str));
	gmk_eval(str, 0);
    }
    // For avoiding `missing separator...` error
    XSRETURN_PV("");
}

XS(XS_gmk_add_function)
{
    dXSARGS;
    if (items < 1)
	printf("Usage: gmk_add_function(arg, ...)\n");

    PERL_UNUSED_VAR(cv);
    {
	SV *funcname = ST(0);
	SV *func = NULL;
	int arg_min = 0;
	int arg_max = 0;

	if (items > 1)
	    func = ST(1);
	if (items > 2)
	    arg_min = SvIV(ST(2));
	if (items > 3)
	    arg_max = SvIV(ST(3));

	gmk_add_function(SvPV_nolen(funcname), func_perl_call_wrapper, arg_min, arg_max, GMK_FUNC_DEFAULT);
	hv_store_ent(gmk_funcs, SvREFCNT_inc(funcname), func, 0);
    }
    XSRETURN_PV("");
}

static void init_perl()
{
#ifdef DEBUGGING
    char *argv[] = {"", "-we", "0", "-D"};
    {
	char *level = gmk_expand("$(PERL_DEBUG)");
	if (!(level && atol(level)))
	    argv[3] = "";
	if (level)
	    gmk_free(level);
    }
#else
    char *argv[] = {"", "-we", "0"};
#endif
    int argc = sizeof(argv)/sizeof(char*);
    PERL_SYS_INIT(&argc, (char***)&argv);
    my_perl = perl_alloc();
    perl_construct(my_perl);
    PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
    perl_parse(my_perl, NULL, argc, argv, (char **)NULL);
    perl_run(my_perl);

    newXS("gmk_expand", XS_gmk_expand, __FILE__);
    newXS("gmk_eval", XS_gmk_eval, __FILE__);
    newXS("gmk_add_function", XS_gmk_add_function, __FILE__);

    gmk_funcs = newHV();
}


int perl_gmk_setup()
{
    gmk_add_function("perl", func_perl, 1, 1, GMK_FUNC_DEFAULT);
    gmk_add_function("perl-load", func_perl_load, 0, 0, GMK_FUNC_DEFAULT);
    gmk_add_function("perl-free", func_perl_free, 0, 0, GMK_FUNC_DEFAULT);
    return 1;
}
