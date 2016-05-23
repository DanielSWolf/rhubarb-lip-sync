/* THIS IS A MODIFIED VERSION.  It was modified by David
   Huggins-Daines <dhd@cepstral.com> in December 2001 for use in the
   Flite and Theta speech synthesis systems. */

/*
 *
 *	RATECONV.C
 *
 *	Convert sampling rate stdin to stdout
 *
 *	Copyright (c) 1992, 1995 by Markus Mummert
 *
 *	Redistribution and use of this software, modifcation and inclusion
 *	into other forms of software are permitted provided that the following
 *	conditions are met:
 *
 *	1. Redistributions of this software must retain the above copyright
 *	   notice, this list of conditions and the following disclaimer.
 *	2. If this software is redistributed in a modified condition
 *	   it must reveal clearly that it has been modified.
 *	
 *	THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
 *	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *	TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *	PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR
 *	CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *	EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *	PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 *	OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *	USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 *	DAMAGE.
 *
 *
 *	history: 2.9.92		begin coding
 *		 5.9.92		fully operational
 *		 14.2.95 	provide BIG_ENDIAN, SWAPPED_BYTES_DEFAULT
 *				switches, Copyright note and References
 *		 25.11.95	changed XXX_ENDIAN to I_AM_XXX_ENDIAN
 *				default gain set to 0.8
 *		 3.12.95	stereo implementation
 *				SWAPPED_BYTES_DEFAULT now HBYTE1ST_DEFAULT
 *				changed [L/2] to (L-1)/2 for exact symmetry
 *
 *
 *	IMPLEMENTATION NOTES
 *
 *	Converting is achieved by interpolating the input samples in
 *	order to evaluate the represented continuous input slope at
 *	sample instances of the new rate (resampling). It is implemented 
 *	as a polyphase FIR-filtering process (see reference). The rate
 *	conversion factor is determined by a rational factor. Its
 *	nominator and denominator are integers of almost arbitrary
 *	value, limited only by coefficient memory size.
 *
 *	General rate conversion formula:
 *
 *	    out(n*Tout) = SUM in(m*Tin) * g((n*d/u-m)*Tin) * Tin
 *		      over all m
 *
 *	FIR-based rate conversion formula for polyphase processing:
 *
 *			  L-1
 *	    out(n*Tout) = SUM in(A(i,n)*Tin) * g(B(i,n)*Tin) * Tin
 *			  i=0
 *
 *	    A(i,n) = i - (L-1)/2 + [n*d/u]              
 *	           = i - (L-1)/2 + [(n%u)*d/u] + [n/u]*d 
 *	    B(i,n) = n*d/u - [n*d/u] + (L-1)/2 - i
 *	           =  ((n%u)*d/u)%1  + (L-1)/2 - i
 *	    Tout   = Tin * d/u
 *
 *	  where:
 *	    n,i		running integers
 *	    out(t)	output signal sampled at t=n*Tout
 *	    in(t)	input signal sampled in intervalls Tin
 *	    u,d		up- and downsampling factor, integers
 *	    g(t)	interpolating function
 *	    L		FIR-length of realized g(t), integer
 *	    /		float-division-operator
 *	    %		float-modulo-operator
 *	    []		integer-operator
 *
 *	  note:
 *	    (L-1)/2	in A(i,n) can be omitted as pure time shift yielding
 *			a causal design with a delay of ((L-1)/2)*Tin.
 *	    n%u		is a cyclic modulo-u counter clocked by out-rate
 *	    [n/u]*d	is a d-increment counter, advanced when n%u resets
 *	    B(i,n)*Tin	can take on L*u differnt values, at which g(t)
 *			has to be sampled as a coefficient array
 *
 *	Interpolation function design:
 *
 * 	    The interpolation function design is based on a sinc-function
 *	    windowed by a gaussian function. The former determines the
 *	    cutoff frequency, the latter limits the necessary FIR-length by
 *	    pushing the outer skirts of the resulting impulse response below
 *	    a certain threshold fast enough. The drawback is a smoothed
 *	    cutoff inducing some aliasing. Due to the symmetry of g(t) the
 *	    group delay of the filtering process is contant (linear phase).
 *
 *	    g(t) = 2*fgK*sinc(pi*2*fgK*t) * exp(-pi*(2*fgG*t)**2)
 *
 *	  where:
 *	    fgK		cutoff frequency of sinc function in f-domain
 *	    fgG		key frequency of gaussian window in f-domain
 *			reflecting the 6.82dB-down point
 *
 * 	  note:	    
 *	    Taking fsin=1/Tin as the input sampling frequncy, it turns out
 *	    that in conjunction with L, u and d only the ratios fgK/(fsin/2)
 *	    and fgG/(fsin/2) specify the whole proces. Requiring fsin, fgK
 *	    and fgG as input purposely keeps the notion of absolute
 *	    frequencies.
 *
 *	Numerical design:
 *
 *	    Samples are expected to be 16bit-signed integers, alternating
 *	    left and right channel in case of stereo mode- The byte order
 *	    per sample is selectable. FIR-filtering is implemented using
 *	    32bit-signed integer arithmetic. Coefficients are scaled to
 *	    find the output sample in the high word of accumulated FIR-sum.
 *
 *	    Interpolation can lead to sample magnitudes exceeding the
 *	    input maximum. Worst case is a full scale step function on the
 *	    input. In this case the sinc-function exhibits an overshoot of
 *	    2*9=18percent (Gibb's phaenomenon). In any case sample overflow
 *	    can be avoided by a gain of 0.8.
 *
 *	    If u=d=1 and if the input stream contains only a single sample,
 *	    the whole length of the FIR-filter will be written to the output.
 *	    In general the resulting output signal will be (L-1)*fsout/fsin
 *	    samples longer than the input signal. The effect is that a 
 *	    finite input sequence is viewed as padded with zeros before the
 *	    `beginning' and after the `end'. 
 *
 *	    The output lags ((L-1)/2)*Tin behind to implement g(t) as a
 *	    causal system corresponding to a causal relationship of the
 *	    discrete-time sequences in(m/fsin) and out(n/fsout) with
 *	    resepect to a sequence time origin at t=n*Tin=m*Tout=0.
 *
 *
 * 	REFERENCES
 *
 *	    Crochiere, R. E., Rabiner, L. R.: "Multirate Digital Signal
 *	    Processing", Prentice-Hall, Englewood Cliffs, New Jersey, 1983
 *
 *	    Zwicker, E., Fastl, H.: "Psychoacoustics - Facts and Models",
 * Springer-Verlag, Berlin, Heidelberg, New-York, Tokyo, 1990 */

#include "cst_string.h"
#include "cst_math.h"
#include "cst_alloc.h"
#include "cst_error.h"
#include "cst_wave.h"

/*
 *	adaptable defines and globals
 */

#define DPRINTF(l, x)

#define FIXSHIFT 16
#define FIXMUL (1<<FIXSHIFT)

#ifndef M_PI
#define M_PI 3.1415926535
#endif

#define sqr(a)	((a)*(a))

/*
 *	FIR-routines, mono and stereo
 *	this is where we need all the MIPS
 */
static void
fir_mono(int *inp, int *coep, int firlen, int *outp)
{
	register int akku = 0, *endp;
	int n1 = (firlen / 8) * 8, n0 = firlen % 8;

	endp = coep + n1;
	while (coep != endp) {
		akku += *inp++ * *coep++;
		akku += *inp++ * *coep++;
		akku += *inp++ * *coep++;
		akku += *inp++ * *coep++;
		akku += *inp++ * *coep++;
		akku += *inp++ * *coep++;
		akku += *inp++ * *coep++;
		akku += *inp++ * *coep++;
	}

	endp = coep + n0;
	while (coep != endp) {
		akku += *inp++ * *coep++;
	}
	*outp = akku;
}

static void
fir_stereo(int *inp, int *coep,
	   int firlen, int *out1p, int *out2p)
{
	register int akku1 = 0, akku2 = 0, *endp;
	int n1 = (firlen / 8) * 8, n0 = firlen % 8;

	endp = coep + n1;
	while (coep != endp) {
		akku1 += *inp++ * *coep;
		akku2 += *inp++ * *coep++;
		akku1 += *inp++ * *coep;
		akku2 += *inp++ * *coep++;
		akku1 += *inp++ * *coep;
		akku2 += *inp++ * *coep++;
		akku1 += *inp++ * *coep;
		akku2 += *inp++ * *coep++;
		akku1 += *inp++ * *coep;
		akku2 += *inp++ * *coep++;
		akku1 += *inp++ * *coep;
		akku2 += *inp++ * *coep++;
		akku1 += *inp++ * *coep;
		akku2 += *inp++ * *coep++;
		akku1 += *inp++ * *coep;
		akku2 += *inp++ * *coep++;
	}

	endp = coep + n0;
	while (coep != endp) {
		akku1 += *inp++ * *coep;
		akku2 += *inp++ * *coep++;
	}
	*out1p = akku1;
	*out2p = akku2;
}

/*
 * 	filtering from input buffer to output buffer;
 *	returns number of processed samples in output buffer:
 *	if it is not equal to output buffer size,
 *	the input buffer is expected to be refilled upon entry, so that
 *	the last firlen numbers of the old input buffer are
 *	the first firlen numbers of the new input buffer;
 *	if it is equal to output buffer size, the output buffer
 *	is full and is expected to be stowed away;
 *
 */
static int
filtering_on_buffers(cst_rateconv *filt)
{
	int insize;

	DPRINTF(0, ("filtering_on_buffers(%d)\n", filt->incount));
	insize = filt->incount + filt->lag;
	if (filt->channels == 1) {
		while (1) {
			filt->inoffset = (filt->cycctr * filt->down)/filt->up;
			if ((filt->inbaseidx + filt->inoffset + filt->len) > insize) {
				filt->inbaseidx -= insize - filt->len + 1;
				memcpy(filt->sin, filt->sin + insize - filt->lag,
				       filt->lag * sizeof(int));
				/* Prevent people from re-filtering the same stuff. */
				filt->incount = 0;
				return 0;
			}
			fir_mono(filt->sin + filt->inoffset + filt->inbaseidx,
				 filt->coep + filt->cycctr * filt->len,
				 filt->len, filt->sout + filt->outidx);
			DPRINTF(1, ("in(%d + %d) = %d cycctr %d out(%d) = %d\n",
				    filt->inoffset, filt->inbaseidx,
				    filt->sin[filt->inoffset + filt->inbaseidx],
				    filt->cycctr, filt->outidx,
				    filt->sout[filt->outidx] >> FIXSHIFT));
			++filt->outidx;
			++filt->cycctr;
			if (!(filt->cycctr %= filt->up))
				filt->inbaseidx += filt->down;
			if (!(filt->outidx %= filt->outsize))
				return filt->outsize;
		}
	} else if (filt->channels == 2) {
		/*
		 * rule how to convert mono routine to stereo routine:
		 * firlen, up, down and cycctr relate to samples in general,
		 * wether mono or stereo; inbaseidx, inoffset and outidx as
		 * well as insize and outsize still account for mono samples.
		 */
		while (1) {
			filt->inoffset = 2*((filt->cycctr * filt->down)/filt->up);
			if ((filt->inbaseidx + filt->inoffset + 2*filt->len) > insize) {
				filt->inbaseidx -= insize - 2*filt->len + 2;
				return filt->outidx;
			}
			fir_stereo(filt->sin + filt->inoffset + filt->inbaseidx,
				   filt->coep + filt->cycctr * filt->len,
				   filt->len,
				   filt->sout + filt->outidx,
				   filt->sout + filt->outidx + 1);
			filt->outidx += 2;
			++filt->cycctr;
			if (!(filt->cycctr %= filt->up))
				filt->inbaseidx += 2*filt->down;
			if (!(filt->outidx %= filt->outsize))
				return filt->outsize;
		}
	} else {
		cst_errmsg("filtering_on_buffers: only 1 or 2 channels supported!\n");
		cst_error();
	}
	return 0;
}

/*
 *	convert buffer of n samples to ints
 */
static void
sample_to_int(short *buff, int n)
{
	short *s, *e;
	int *d;

	if (n < 1)
		return;	
	s = buff + n;
	d = (int*)buff + n;
	e = buff;
	while (s != e) {
		*--d = (int)(*--s); 
	}
}

/*
 *	convert buffer of n ints to samples
 */
static void
int_to_sample(short *buff, int n)
{
	int *s;
	short *e, *d;

	if (n < 1)
		return;	
	s = (int *)buff;
	d = buff;
	e = buff + n;
	while (d != e)
		*d++ = (*s++ >> FIXSHIFT);
}

/*
 *	read and convert input sample format to integer
 */
int
cst_rateconv_in(cst_rateconv *filt, const short *inptr, int max)
{
	if (max > filt->insize - filt->lag)
		max = filt->insize - filt->lag;
	if (max > 0) {
		memcpy(filt->sin + filt->lag, inptr, max * sizeof(short));
		sample_to_int((short *)(filt->sin + filt->lag), max);
	}
	filt->incount = max;
	return max;
}

/*
 *	do some conversion jobs and write
 */
int
cst_rateconv_out(cst_rateconv *filt, short *outptr, int max)
{
	int outsize;

	if ((outsize = filtering_on_buffers(filt)) == 0)
		return 0;
	if (max > outsize)
		max = outsize;
	int_to_sample((short *)filt->sout, max);
	memcpy(outptr, filt->sout, max * sizeof(short));
	return max;
}

int
cst_rateconv_leadout(cst_rateconv *filt)
{
	memset(filt->sin + filt->lag, 0, filt->lag * sizeof(int));
	filt->incount = filt->lag;
	return filt->lag;
}

/*
 *	evaluate sinc(x) = sin(x)/x safely
 */
static double
sinc(double x)
{
	return(fabs(x) < 1E-50 ? 1.0 : sin(fmod(x,2*M_PI))/x);
}

/*
 *	evaluate interpolation function g(t) at t
 *	integral of g(t) over all times is expected to be one
 */
static double
interpol_func(double t, double fgk, double fgg)
{
	return (2*fgk*sinc(M_PI*2*fgk*t)*exp(-M_PI*sqr(2*fgg*t)));
}

/*
 *	evaluate coefficient from i, q=n%u by sampling interpolation function 
 *	and scale it for integer multiplication used by FIR-filtering
 */
static int
coefficient(int i, int q, cst_rateconv *filt)
{
	return (int)(FIXMUL * filt->gain *
		     interpol_func((fmod(q* filt->down/(double)filt->up,1.0)
				    + (filt->len-1)/2.0 - i) / filt->fsin,
				   filt->fgk, filt->fgg) / filt->fsin);
}

/*
 *	set up coefficient array
 */
static void
make_coe(cst_rateconv *filt)
{
	int i, q;

	filt->coep = cst_alloc(int, filt->len * filt->up);
	for (i = 0; i < filt->len; i++) {
	    for (q = 0; q < filt->up; q++) {
		filt->coep[q * filt->len + i]
			= coefficient(i, q, filt);
	    }
	}
}

cst_rateconv *
new_rateconv(int up, int down, int channels)
{
	cst_rateconv *filt;

	if (!(channels == 1 || channels == 2)) {
		cst_errmsg("new_rateconv: channels must be 1 or 2\n");
		cst_error();
	}

	filt = cst_alloc(cst_rateconv, 1);
	filt->fsin = 1.0;
	filt->gain = 0.8;
	filt->fgg = 0.0116;
	filt->fgk = 0.461;
	filt->len = 162;
	filt->down = down;
	filt->up = up;
	filt->channels = channels;

	if (down > up) {
		filt->fgg *= (double) up / down;
		filt->fgk *= (double) up / down;
		filt->len = filt->len * down / up;
	}

	make_coe(filt);
	filt->lag = (filt->len - 1) * channels;
	filt->insize = channels*filt->len + filt->lag;
	filt->outsize = channels*filt->len;
	filt->sin = cst_alloc(int, filt->insize);
	filt->sout = cst_alloc(int, filt->outsize);

	return filt;
}

void
delete_rateconv(cst_rateconv *filt)
{
	cst_free(filt->coep);
	cst_free(filt->sin);
	cst_free(filt->sout);
	cst_free(filt);
}
