/* Copyright (c) 1997-1999 Miller Puckette and others.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* sampling */

/* LATER make tabread4 and tabread~ */

#include "m_pd.h"


/* ------------------------- tabwrite~ -------------------------- */

static t_class *tabwrite_tilde_class;

typedef struct _tabwrite_tilde
{
    t_object x_obj;
    int x_phase;
    int x_nsampsintab;
    float *x_vec;
    t_symbol *x_arrayname;
    t_clock *x_clock;
    float x_f;
} t_tabwrite_tilde;

static void tabwrite_tilde_tick(t_tabwrite_tilde *x);

static void *tabwrite_tilde_new(t_symbol *s)
{
    t_tabwrite_tilde *x = (t_tabwrite_tilde *)pd_new(tabwrite_tilde_class);
    x->x_clock = clock_new(x, (t_method)tabwrite_tilde_tick);
    x->x_phase = 0x7fffffff;
    x->x_arrayname = s;
    x->x_f = 0;
    return (x);
}

static t_int *tabwrite_tilde_perform(t_int *w)
{
    t_tabwrite_tilde *x = (t_tabwrite_tilde *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    int n = (int)(w[3]), phase = x->x_phase, endphase = x->x_nsampsintab;
    if (!x->x_vec) goto bad;
    
    if (endphase > phase)
    {
    	int nxfer = endphase - phase;
    	float *fp = x->x_vec + phase;
    	if (nxfer > n) nxfer = n;
    	phase += nxfer;
    	while (nxfer--)
	{
	    float f = *in++;
    	    if (PD_BIGORSMALL(f))
	    	f = 0;
	    *fp++ = f;
    	}
	if (phase >= endphase)
    	{
    	    clock_delay(x->x_clock, 0);
    	    phase = 0x7fffffff;
    	}
    	x->x_phase = phase;
    }
bad:
    return (w+4);
}

void tabwrite_tilde_set(t_tabwrite_tilde *x, t_symbol *s)
{
    t_garray *a;

    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
    	if (*s->s_name) pd_error(x, "tabwrite~: %s: no such array",
    	    x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else if (!garray_getfloatarray(a, &x->x_nsampsintab, &x->x_vec))
    {
    	pd_error(x, "%s: bad template for tabwrite~", x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else garray_usedindsp(a);
}

static void tabwrite_tilde_dsp(t_tabwrite_tilde *x, t_signal **sp)
{
    tabwrite_tilde_set(x, x->x_arrayname);
    dsp_add(tabwrite_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void tabwrite_tilde_bang(t_tabwrite_tilde *x)
{
    x->x_phase = 0;
}

static void tabwrite_tilde_stop(t_tabwrite_tilde *x)
{
    if (x->x_phase != 0x7fffffff)
    {
    	tabwrite_tilde_tick(x);
    	x->x_phase = 0x7fffffff;
    }
}

static void tabwrite_tilde_tick(t_tabwrite_tilde *x)
{
    t_garray *a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class);
    if (!a) bug("tabwrite_tilde_tick");
    else garray_redraw(a);
}

static void tabwrite_tilde_free(t_tabwrite_tilde *x)
{
    clock_free(x->x_clock);
}

static void tabwrite_tilde_setup(void)
{
    tabwrite_tilde_class = class_new(gensym("tabwrite~"),
    	(t_newmethod)tabwrite_tilde_new, (t_method)tabwrite_tilde_free,
    	sizeof(t_tabwrite_tilde), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(tabwrite_tilde_class, t_tabwrite_tilde, x_f);
    class_addmethod(tabwrite_tilde_class, (t_method)tabwrite_tilde_dsp,
    	gensym("dsp"), 0);
    class_addmethod(tabwrite_tilde_class, (t_method)tabwrite_tilde_set,
    	gensym("set"), A_SYMBOL, 0);
    class_addmethod(tabwrite_tilde_class, (t_method)tabwrite_tilde_stop,
    	gensym("stop"), 0);
    class_addbang(tabwrite_tilde_class, tabwrite_tilde_bang);
}

/* ------------ tabplay~ - non-transposing sample playback --------------- */

static t_class *tabplay_tilde_class;

typedef struct _tabplay_tilde
{
    t_object x_obj;
    t_outlet *x_bangout;
    int x_phase;
    int x_nsampsintab;
    int x_limit;
    float *x_vec;
    t_symbol *x_arrayname;
    t_clock *x_clock;
} t_tabplay_tilde;

static void tabplay_tilde_tick(t_tabplay_tilde *x);

static void *tabplay_tilde_new(t_symbol *s)
{
    t_tabplay_tilde *x = (t_tabplay_tilde *)pd_new(tabplay_tilde_class);
    x->x_clock = clock_new(x, (t_method)tabplay_tilde_tick);
    x->x_phase = 0x7fffffff;
    x->x_limit = 0;
    x->x_arrayname = s;
    outlet_new(&x->x_obj, &s_signal);
    x->x_bangout = outlet_new(&x->x_obj, &s_bang);
    return (x);
}

static t_int *tabplay_tilde_perform(t_int *w)
{
    t_tabplay_tilde *x = (t_tabplay_tilde *)(w[1]);
    t_float *out = (t_float *)(w[2]), *fp;
    int n = (int)(w[3]), phase = x->x_phase,
    	endphase = (x->x_nsampsintab < x->x_limit ?
	    x->x_nsampsintab : x->x_limit), nxfer, n3;
    if (!x->x_vec || phase >= endphase)
    	goto zero;
    
    nxfer = endphase - phase;
    fp = x->x_vec + phase;
    if (nxfer > n)
    	nxfer = n;
    n3 = n - nxfer;
    phase += nxfer;
    while (nxfer--)
    	*out++ = *fp++;
    if (phase >= endphase)
    {
    	clock_delay(x->x_clock, 0);
    	x->x_phase = 0x7fffffff;
	while (n3--)
	    *out++ = 0;
    }
    else x->x_phase = phase;
    
    return (w+4);
zero:
    while (n--) *out++ = 0;
    return (w+4);
}

void tabplay_tilde_set(t_tabplay_tilde *x, t_symbol *s)
{
    t_garray *a;

    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
    	if (*s->s_name) pd_error(x, "tabplay~: %s: no such array",
    	    x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else if (!garray_getfloatarray(a, &x->x_nsampsintab, &x->x_vec))
    {
    	pd_error(x, "%s: bad template for tabplay~", x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else garray_usedindsp(a);
}

static void tabplay_tilde_dsp(t_tabplay_tilde *x, t_signal **sp)
{
    tabplay_tilde_set(x, x->x_arrayname);
    dsp_add(tabplay_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void tabplay_tilde_list(t_tabplay_tilde *x, t_symbol *s,
    int argc, t_atom *argv)
{
    long start = atom_getfloatarg(0, argc, argv);
    long length = atom_getfloatarg(1, argc, argv);
    if (start < 0) start = 0;
    if (length <= 0)
    	x->x_limit = 0x7fffffff;
    else
    	x->x_limit = start + length;
    x->x_phase = start;
}

static void tabplay_tilde_stop(t_tabplay_tilde *x)
{
    x->x_phase = 0x7fffffff;
}

static void tabplay_tilde_tick(t_tabplay_tilde *x)
{
    outlet_bang(x->x_bangout);
}

static void tabplay_tilde_free(t_tabplay_tilde *x)
{
    clock_free(x->x_clock);
}

static void tabplay_tilde_setup(void)
{
    tabplay_tilde_class = class_new(gensym("tabplay~"),
    	(t_newmethod)tabplay_tilde_new, (t_method)tabplay_tilde_free,
    	sizeof(t_tabplay_tilde), 0, A_DEFSYM, 0);
    class_addmethod(tabplay_tilde_class, (t_method)tabplay_tilde_dsp,
    	gensym("dsp"), 0);
    class_addmethod(tabplay_tilde_class, (t_method)tabplay_tilde_stop,
    	gensym("stop"), 0);
    class_addmethod(tabplay_tilde_class, (t_method)tabplay_tilde_set,
    	gensym("set"), A_DEFSYM, 0);
    class_addlist(tabplay_tilde_class, tabplay_tilde_list);
}

/******************** tabread~ ***********************/

static t_class *tabread_tilde_class;

typedef struct _tabread_tilde
{
    t_object x_obj;
    int x_npoints;
    float *x_vec;
    t_symbol *x_arrayname;
    float x_f;
} t_tabread_tilde;

static void *tabread_tilde_new(t_symbol *s)
{
    t_tabread_tilde *x = (t_tabread_tilde *)pd_new(tabread_tilde_class);
    x->x_arrayname = s;
    x->x_vec = 0;
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_f = 0;
    return (x);
}

static t_int *tabread_tilde_perform(t_int *w)
{
    t_tabread_tilde *x = (t_tabread_tilde *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    int n = (int)(w[4]);    
    int maxindex;
    float *buf = x->x_vec, *fp;
    int i;
    
    maxindex = x->x_npoints - 1;
    if (!buf) goto zero;

    for (i = 0; i < n; i++)
    {
	int index = *in++;
	if (index < 0)
	    index = 0;
	else if (index > maxindex)
	    index = maxindex;
	*out++ = buf[index];
    }
    return (w+5);
 zero:
    while (n--) *out++ = 0;

    return (w+5);
}

void tabread_tilde_set(t_tabread_tilde *x, t_symbol *s)
{
    t_garray *a;
    
    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
    	if (*s->s_name)
    	    pd_error(x, "tabread~: %s: no such array", x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else if (!garray_getfloatarray(a, &x->x_npoints, &x->x_vec))
    {
    	pd_error(x, "%s: bad template for tabread~", x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else garray_usedindsp(a);
}

static void tabread_tilde_dsp(t_tabread_tilde *x, t_signal **sp)
{
    tabread_tilde_set(x, x->x_arrayname);

    dsp_add(tabread_tilde_perform, 4, x,
    	sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);

}

static void tabread_tilde_free(t_tabread_tilde *x)
{
}

static void tabread_tilde_setup(void)
{
    tabread_tilde_class = class_new(gensym("tabread~"),
    	(t_newmethod)tabread_tilde_new, (t_method)tabread_tilde_free,
    	sizeof(t_tabread_tilde), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(tabread_tilde_class, t_tabread_tilde, x_f);
    class_addmethod(tabread_tilde_class, (t_method)tabread_tilde_dsp,
    	gensym("dsp"), 0);
    class_addmethod(tabread_tilde_class, (t_method)tabread_tilde_set,
    	gensym("set"), A_SYMBOL, 0);
}

/******************** tabread4~ ***********************/

static t_class *tabread4_tilde_class;

typedef struct _tabread4_tilde
{
    t_object x_obj;
    int x_npoints;
    float *x_vec;
    t_symbol *x_arrayname;
    float x_f;
} t_tabread4_tilde;

static void *tabread4_tilde_new(t_symbol *s)
{
    t_tabread4_tilde *x = (t_tabread4_tilde *)pd_new(tabread4_tilde_class);
    x->x_arrayname = s;
    x->x_vec = 0;
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_f = 0;
    return (x);
}

static t_int *tabread4_tilde_perform(t_int *w)
{
    t_tabread4_tilde *x = (t_tabread4_tilde *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    int n = (int)(w[4]);    
    int maxindex;
    float *buf = x->x_vec, *fp;
    int i;
    
    maxindex = x->x_npoints - 3;

    if (!buf) goto zero;

#if 0	    /* test for spam -- I'm not ready to deal with this */
    for (i = 0,  xmax = 0, xmin = maxindex,  fp = in1; i < n; i++,  fp++)
    {
	float f = *in1;
	if (f < xmin) xmin = f;
	else if (f > xmax) xmax = f;
    }
    if (xmax < xmin + x->c_maxextent) xmax = xmin + x->c_maxextent;
    for (i = 0, splitlo = xmin+ x->c_maxextent, splithi = xmax - x->c_maxextent,
	fp = in1; i < n; i++,  fp++)
    {
	float f = *in1;
	if (f > splitlo && f < splithi) goto zero;
    }
#endif

    for (i = 0; i < n; i++)
    {
	float findex = *in++;
	int index = findex;
	float frac,  a,  b,  c,  d, cminusb;
	static int count;
	if (index < 1)
	    index = 1, frac = 0;
	else if (index > maxindex)
	    index = maxindex, frac = 1;
	else frac = findex - index;
	fp = buf + index;
	a = fp[-1];
	b = fp[0];
	c = fp[1];
	d = fp[2];
	/* if (!i && !(count++ & 1023))
	    post("fp = %lx,  shit = %lx,  b = %f",  fp, buf->b_shit,  b); */
	cminusb = c-b;
	*out++ = b + frac * (
	    cminusb - 0.1666667f * (1.-frac) * (
		(d - a - 3.0f * cminusb) * frac + (d + 2.0f*a - 3.0f*b)
	    )
	);
    }
    return (w+5);
 zero:
    while (n--) *out++ = 0;

    return (w+5);
}

void tabread4_tilde_set(t_tabread4_tilde *x, t_symbol *s)
{
    t_garray *a;
    
    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
    	if (*s->s_name)
    	    pd_error(x, "tabread4~: %s: no such array", x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else if (!garray_getfloatarray(a, &x->x_npoints, &x->x_vec))
    {
    	pd_error(x, "%s: bad template for tabread4~", x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else garray_usedindsp(a);
}

static void tabread4_tilde_dsp(t_tabread4_tilde *x, t_signal **sp)
{
    tabread4_tilde_set(x, x->x_arrayname);

    dsp_add(tabread4_tilde_perform, 4, x,
    	sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);

}

static void tabread4_tilde_free(t_tabread4_tilde *x)
{
}

static void tabread4_tilde_setup(void)
{
    tabread4_tilde_class = class_new(gensym("tabread4~"),
    	(t_newmethod)tabread4_tilde_new, (t_method)tabread4_tilde_free,
    	sizeof(t_tabread4_tilde), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(tabread4_tilde_class, t_tabread4_tilde, x_f);
    class_addmethod(tabread4_tilde_class, (t_method)tabread4_tilde_dsp,
    	gensym("dsp"), 0);
    class_addmethod(tabread4_tilde_class, (t_method)tabread4_tilde_set,
    	gensym("set"), A_SYMBOL, 0);
}

/******************** tabosc4~ ***********************/

/* this is all copied from d_osc.c... what include file could this go in? */
#define UNITBIT32 1572864.  /* 3*2^19; bit 32 has place value 1 */

    /* machine-dependent definitions.  These ifdefs really
    should have been by CPU type and not by operating system! */
#ifdef IRIX
    /* big-endian.  Most significant byte is at low address in memory */
#define HIOFFSET 0    /* word offset to find MSB */
#define LOWOFFSET 1    /* word offset to find LSB */
#define int32 long  /* a data type that has 32 bits */
#else
#ifdef MSW
    /* little-endian; most significant byte is at highest address */
#define HIOFFSET 1
#define LOWOFFSET 0
#define int32 long
#else
#ifdef __FreeBSD__
#include <machine/endian.h>
#if BYTE_ORDER == LITTLE_ENDIAN
#define HIOFFSET 1
#define LOWOFFSET 0
#else
#define HIOFFSET 0    /* word offset to find MSB */
#define LOWOFFSET 1    /* word offset to find LSB */
#endif /* BYTE_ORDER */
#include <sys/types.h>
#define int32 int32_t
#endif

#ifdef  __linux__
#include <endian.h>
#if !defined(__BYTE_ORDER) || !defined(__LITTLE_ENDIAN)                         
#error No byte order defined                                                    
#endif                                                                          
                                                                                
#if __BYTE_ORDER == __LITTLE_ENDIAN                                             
#define HIOFFSET 1                                                              
#define LOWOFFSET 0                                                             
#else                                                                           
#define HIOFFSET 0    /* word offset to find MSB */                             
#define LOWOFFSET 1    /* word offset to find LSB */                            
#endif /* __BYTE_ORDER */                                                       

#include <sys/types.h>
#define int32 int32_t

#else
#ifdef MACOSX
#define HIOFFSET 0    /* word offset to find MSB */
#define LOWOFFSET 1    /* word offset to find LSB */
#define int32 int  /* a data type that has 32 bits */

#endif /* MACOSX */
#endif /* __linux__ */
#endif /* MSW */
#endif /* SGI */

union tabfudge
{
    double tf_d;
    int32 tf_i[2];
};

static t_class *tabosc4_tilde_class;

typedef struct _tabosc4_tilde
{
    t_object x_obj;
    float x_fnpoints;
    float x_finvnpoints;
    float *x_vec;
    t_symbol *x_arrayname;
    float x_f;
    double x_phase;
    float x_conv;
} t_tabosc4_tilde;

static void *tabosc4_tilde_new(t_symbol *s)
{
    t_tabosc4_tilde *x = (t_tabosc4_tilde *)pd_new(tabosc4_tilde_class);
    x->x_arrayname = s;
    x->x_vec = 0;
    x->x_fnpoints = 512.;
    x->x_finvnpoints = (1./512.);
    outlet_new(&x->x_obj, gensym("signal"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
    x->x_f = 0;
    return (x);
}

static t_int *tabosc4_tilde_perform(t_int *w)
{
    t_tabosc4_tilde *x = (t_tabosc4_tilde *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    int n = (int)(w[4]);
    int normhipart;
    union tabfudge tf;
    float fnpoints = x->x_fnpoints;
    int mask = fnpoints - 1;
    float conv = fnpoints * x->x_conv;
    int maxindex;
    float *tab = x->x_vec, *addr;
    int i;
    double dphase = fnpoints * x->x_phase + UNITBIT32;

    if (!tab) goto zero;
    tf.tf_d = UNITBIT32;
    normhipart = tf.tf_i[HIOFFSET];

#if 1
    while (n--)
    {
	float frac,  a,  b,  c,  d, cminusb;
	tf.tf_d = dphase;
    	dphase += *in++ * conv;
	addr = tab + (tf.tf_i[HIOFFSET] & mask);
	tf.tf_i[HIOFFSET] = normhipart;
	frac = tf.tf_d - UNITBIT32;
	a = addr[0];
	b = addr[1];
	c = addr[2];
	d = addr[3];
	cminusb = c-b;
	*out++ = b + frac * (
	    cminusb - 0.1666667f * (1.-frac) * (
		(d - a - 3.0f * cminusb) * frac + (d + 2.0f*a - 3.0f*b)
	    )
	);
    }
#endif

    tf.tf_d = UNITBIT32 * fnpoints;
    normhipart = tf.tf_i[HIOFFSET];
    tf.tf_d = dphase + (UNITBIT32 * fnpoints - UNITBIT32);
    tf.tf_i[HIOFFSET] = normhipart;
    x->x_phase = (tf.tf_d - UNITBIT32 * fnpoints)  * x->x_finvnpoints;
    return (w+5);
 zero:
    while (n--) *out++ = 0;

    return (w+5);
}

void tabosc4_tilde_set(t_tabosc4_tilde *x, t_symbol *s)
{
    t_garray *a;
    int npoints, pointsinarray;

    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
    	if (*s->s_name)
    	    pd_error(x, "tabosc4~: %s: no such array", x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else if (!garray_getfloatarray(a, &pointsinarray, &x->x_vec))
    {
    	pd_error(x, "%s: bad template for tabosc4~", x->x_arrayname->s_name);
    	x->x_vec = 0;
    }
    else if ((npoints = pointsinarray - 3) != (1 << ilog2(pointsinarray - 3)))
    {
    	pd_error(x, "%s: number of points (%d) not a power of 2 plus three",
	    x->x_arrayname->s_name, pointsinarray);
    	x->x_vec = 0;
	garray_usedindsp(a);
    }
    else
    {
    	x->x_fnpoints = npoints;
    	x->x_finvnpoints = 1./npoints;
	garray_usedindsp(a);
    }
}

static void tabosc4_tilde_ft1(t_tabosc4_tilde *x, t_float f)
{
    x->x_phase = f;
}

static void tabosc4_tilde_dsp(t_tabosc4_tilde *x, t_signal **sp)
{
    x->x_conv = 1. / sp[0]->s_sr;
    tabosc4_tilde_set(x, x->x_arrayname);

    dsp_add(tabosc4_tilde_perform, 4, x,
    	sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

static void tabosc4_tilde_setup(void)
{
    tabosc4_tilde_class = class_new(gensym("tabosc4~"),
    	(t_newmethod)tabosc4_tilde_new, 0,
    	sizeof(t_tabosc4_tilde), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(tabosc4_tilde_class, t_tabosc4_tilde, x_f);
    class_addmethod(tabosc4_tilde_class, (t_method)tabosc4_tilde_dsp,
    	gensym("dsp"), 0);
    class_addmethod(tabosc4_tilde_class, (t_method)tabosc4_tilde_set,
    	gensym("set"), A_SYMBOL, 0);
    class_addmethod(tabosc4_tilde_class, (t_method)tabosc4_tilde_ft1,
    	gensym("ft1"), A_FLOAT, 0);
}

/* ------------------------ tabsend~ ------------------------- */

static t_class *tabsend_class;

typedef struct _tabsend
{
    t_object x_obj;
    float *x_vec;
    int x_graphperiod;
    int x_graphcount;
    t_symbol *x_arrayname;
    t_clock *x_clock;
    float x_f;
} t_tabsend;

static void tabsend_tick(t_tabsend *x);

static void *tabsend_new(t_symbol *s)
{
    t_tabsend *x = (t_tabsend *)pd_new(tabsend_class);
    x->x_graphcount = 0;
    x->x_arrayname = s;
    x->x_clock = clock_new(x, (t_method)tabsend_tick);
    x->x_f = 0;
    return (x);
}

static t_int *tabsend_perform(t_int *w)
{
    t_tabsend *x = (t_tabsend *)(w[1]);
    t_float *in = (t_float *)(w[2]);
    int n = w[3];
    t_float *dest = x->x_vec;
    int i = x->x_graphcount;
    if (!x->x_vec) goto bad;

    while (n--)
    {	
    	float f = *in++;
    	if (PD_BIGORSMALL(f))
	    f = 0;
	 *dest++ = f;
    }
    if (!i--)
    {
    	clock_delay(x->x_clock, 0);
    	i = x->x_graphperiod;
    }
    x->x_graphcount = i;
bad:
    return (w+4);
}

static void tabsend_dsp(t_tabsend *x, t_signal **sp)
{
    int i, vecsize;
    t_garray *a;

    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
    	if (*x->x_arrayname->s_name)
	    pd_error(x, "tabsend~: %s: no such array", x->x_arrayname->s_name);
    }
    else if (!garray_getfloatarray(a, &vecsize, &x->x_vec))
    	pd_error(x, "%s: bad template for tabsend~", x->x_arrayname->s_name);
    else
    {
    	int n = sp[0]->s_n;
    	int ticksper = sp[0]->s_sr/n;
    	if (ticksper < 1) ticksper = 1;
    	x->x_graphperiod = ticksper;
    	if (x->x_graphcount > ticksper) x->x_graphcount = ticksper;
    	if (n < vecsize) vecsize = n;
    	garray_usedindsp(a);
    	dsp_add(tabsend_perform, 3, x, sp[0]->s_vec, vecsize);
    }
}

static void tabsend_tick(t_tabsend *x)
{
    t_garray *a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class);
    if (!a) bug("tabsend_tick");
    else garray_redraw(a);
}

static void tabsend_free(t_tabsend *x)
{
    clock_free(x->x_clock);
}

static void tabsend_setup(void)
{
    tabsend_class = class_new(gensym("tabsend~"), (t_newmethod)tabsend_new,
    	(t_method)tabsend_free, sizeof(t_tabsend), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(tabsend_class, t_tabsend, x_f);
    class_addmethod(tabsend_class, (t_method)tabsend_dsp, gensym("dsp"), 0);
}

/* ------------------------ tabreceive~ ------------------------- */

static t_class *tabreceive_class;

typedef struct _tabreceive
{
    t_object x_obj;
    float *x_vec;
    t_symbol *x_arrayname;
} t_tabreceive;

static t_int *tabreceive_perform(t_int *w)
{
    t_tabreceive *x = (t_tabreceive *)(w[1]);
    t_float *out = (t_float *)(w[2]);
    int n = w[3];
    t_float *from = x->x_vec;
    if (from) while (n--) *out++ = *from++;
    else while (n--) *out++ = 0;
    return (w+4);
}

static void tabreceive_dsp(t_tabreceive *x, t_signal **sp)
{
    t_garray *a;
    int vecsize;
    
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
    	if (*x->x_arrayname->s_name)
    	    pd_error(x, "tabsend~: %s: no such array", x->x_arrayname->s_name);
    }
    else if (!garray_getfloatarray(a, &vecsize, &x->x_vec))
    	pd_error(x, "%s: bad template for tabreceive~", x->x_arrayname->s_name);
    else 
    {
    	int n = sp[0]->s_n;
    	if (n < vecsize) vecsize = n;
    	garray_usedindsp(a);
    	dsp_add(tabreceive_perform, 3, x, sp[0]->s_vec, vecsize);
    }
}

static void *tabreceive_new(t_symbol *s)
{
    t_tabreceive *x = (t_tabreceive *)pd_new(tabreceive_class);
    x->x_arrayname = s;
    outlet_new(&x->x_obj, &s_signal);
    return (x);
}

static void tabreceive_setup(void)
{
    tabreceive_class = class_new(gensym("tabreceive~"),
    	(t_newmethod)tabreceive_new, 0,
    	sizeof(t_tabreceive), 0, A_DEFSYM, 0);
    class_addmethod(tabreceive_class, (t_method)tabreceive_dsp,
    	gensym("dsp"), 0);
}


/* ---------- tabread: control, non-interpolating ------------------------ */

static t_class *tabread_class;

typedef struct _tabread
{
    t_object x_obj;
    t_symbol *x_arrayname;
} t_tabread;

static void tabread_float(t_tabread *x, t_float f)
{
    t_garray *a;
    int npoints;
    t_float *vec;

    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    	pd_error(x, "%s: no such array", x->x_arrayname->s_name);
    else if (!garray_getfloatarray(a, &npoints, &vec))
    	pd_error(x, "%s: bad template for tabread", x->x_arrayname->s_name);
    else
    {
    	int n = f;
    	if (n < 0) n = 0;
    	else if (n >= npoints) n = npoints - 1;
    	outlet_float(x->x_obj.ob_outlet, (npoints ? vec[n] : 0));
    }
}

static void tabread_set(t_tabread *x, t_symbol *s)
{
    x->x_arrayname = s;
}

static void *tabread_new(t_symbol *s)
{
    t_tabread *x = (t_tabread *)pd_new(tabread_class);
    x->x_arrayname = s;
    outlet_new(&x->x_obj, &s_float);
    return (x);
}

static void tabread_setup(void)
{
    tabread_class = class_new(gensym("tabread"), (t_newmethod)tabread_new,
    	0, sizeof(t_tabread), 0, A_DEFSYM, 0);
    class_addfloat(tabread_class, (t_method)tabread_float);
    class_addmethod(tabread_class, (t_method)tabread_set, gensym("set"),
    	A_SYMBOL, 0);
}

/* ---------- tabread4: control, non-interpolating ------------------------ */

static t_class *tabread4_class;

typedef struct _tabread4
{
    t_object x_obj;
    t_symbol *x_arrayname;
} t_tabread4;

static void tabread4_float(t_tabread4 *x, t_float f)
{
    t_garray *a;
    int npoints;
    t_float *vec;

    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    	pd_error(x, "%s: no such array", x->x_arrayname->s_name);
    else if (!garray_getfloatarray(a, &npoints, &vec))
    	pd_error(x, "%s: bad template for tabread4", x->x_arrayname->s_name);
    else if (npoints < 4)
    	outlet_float(x->x_obj.ob_outlet, 0);
    else if (f <= 1)
    	outlet_float(x->x_obj.ob_outlet, vec[1]);
    else if (f >= npoints - 2)
    	outlet_float(x->x_obj.ob_outlet, vec[npoints - 2]);
    else
    {
    	int n = f;
	float a, b, c, d, cminusb, frac, *fp;
	if (n >= npoints - 2)
	    n = npoints - 3;
	fp = vec + n;
	frac = f - n;
	a = fp[-1];
	b = fp[0];
	c = fp[1];
	d = fp[2];
	cminusb = c-b;
	outlet_float(x->x_obj.ob_outlet, b + frac * (
	    cminusb - 0.1666667f * (1.-frac) * (
		(d - a - 3.0f * cminusb) * frac + (d + 2.0f*a - 3.0f*b))));
    }
}

static void tabread4_set(t_tabread4 *x, t_symbol *s)
{
    x->x_arrayname = s;
}

static void *tabread4_new(t_symbol *s)
{
    t_tabread4 *x = (t_tabread4 *)pd_new(tabread4_class);
    x->x_arrayname = s;
    outlet_new(&x->x_obj, &s_float);
    return (x);
}

static void tabread4_setup(void)
{
    tabread4_class = class_new(gensym("tabread4"), (t_newmethod)tabread4_new,
    	0, sizeof(t_tabread4), 0, A_DEFSYM, 0);
    class_addfloat(tabread4_class, (t_method)tabread4_float);
    class_addmethod(tabread4_class, (t_method)tabread4_set, gensym("set"),
    	A_SYMBOL, 0);
}

/* ------------------ tabwrite: control ------------------------ */

static t_class *tabwrite_class;

typedef struct _tabwrite
{
    t_object x_obj;
    t_symbol *x_arrayname;
    t_clock *x_clock;
    float x_ft1;
    double x_updtime;
    int x_set;
} t_tabwrite;

static void tabwrite_tick(t_tabwrite *x)
{
    t_garray *a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class);
    if (!a) bug("tabwrite_tick");
    else garray_redraw(a);
    x->x_set = 0;
    x->x_updtime = clock_getsystime();
}

static void tabwrite_float(t_tabwrite *x, t_float f)
{
    int i, vecsize;
    t_garray *a;
    t_float *vec;

    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    	pd_error(x, "%s: no such array", x->x_arrayname->s_name);
    else if (!garray_getfloatarray(a, &vecsize, &vec))
    	pd_error(x, "%s: bad template for tabwrite", x->x_arrayname->s_name);
    else
    {
    	int n = x->x_ft1;
    	double timesince = clock_gettimesince(x->x_updtime);
    	if (n < 0) n = 0;
    	else if (n >= vecsize) n = vecsize-1;
    	vec[n] = f;
    	if (timesince > 1000)
    	{
    	    tabwrite_tick(x);
    	}
    	else
    	{
    	    if (x->x_set == 0)
    	    {
    	    	clock_delay(x->x_clock, 1000 - timesince);
    	    	x->x_set = 1;
    	    }
    	}
    }
}

static void tabwrite_set(t_tabwrite *x, t_symbol *s)
{
    x->x_arrayname = s;
}

static void tabwrite_free(t_tabwrite *x)
{
    clock_free(x->x_clock);
}

static void *tabwrite_new(t_symbol *s)
{
    t_tabwrite *x = (t_tabwrite *)pd_new(tabwrite_class);
    x->x_ft1 = 0;
    x->x_arrayname = s;
    x->x_updtime = clock_getsystime();
    x->x_clock = clock_new(x, (t_method)tabwrite_tick);
    floatinlet_new(&x->x_obj, &x->x_ft1);
    return (x);
}

void tabwrite_setup(void)
{
    tabwrite_class = class_new(gensym("tabwrite"), (t_newmethod)tabwrite_new,
    	(t_method)tabwrite_free, sizeof(t_tabwrite), 0, A_DEFSYM, 0);
    class_addfloat(tabwrite_class, (t_method)tabwrite_float);
    class_addmethod(tabwrite_class, (t_method)tabwrite_set, gensym("set"), A_SYMBOL, 0);
}

/* ------------------------ global setup routine ------------------------- */

void d_array_setup(void)
{
    tabwrite_tilde_setup();
    tabplay_tilde_setup();
    tabread_tilde_setup();
    tabread4_tilde_setup();
    tabosc4_tilde_setup();
    tabsend_setup();
    tabreceive_setup();
    tabread_setup();
    tabread4_setup();
    tabwrite_setup();
}

