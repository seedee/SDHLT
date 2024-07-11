#include "cmdlib.h"
#include "messages.h"
#include "log.h"
#include "hlassert.h"
#include "mathtypes.h"
#include "mathlib.h"
#include "win32fix.h"

const vec3_t    vec3_origin = { 0, 0, 0 };

#define IA 16807
#define IM 2147483647
#define IQ 127773
#define IR 2836
#define NTAB 32
#define NDIV (1+(IM-1)/NTAB)
#define MAX_RANDOM_RANGE 0x7FFFFFFFUL

// fran1 -- return a random floating-point number on the interval [0,1)
//
#define AM (1.0/IM)
#define EPS 1.2e-7
#define RNMX (1.0-EPS)

static int m_idum = 0;
static int m_iy = 0;
static int m_iv[NTAB];

static int GenerateRandomNumber( void )
{
	int	j, k;
	
	if( m_idum <= 0 || !m_iy )
	{
		if( -(m_idum) < 1 ) 
			m_idum = 1;
		else  m_idum = -(m_idum);

		for( j = NTAB + 7; j >= 0; j-- )
		{
			k = (m_idum) / IQ;
			m_idum = IA * (m_idum - k * IQ) - IR * k;

			if( m_idum < 0 ) 
				m_idum += IM;

			if( j < NTAB )
				m_iv[j] = m_idum;
		}

		m_iy = m_iv[0];
	}

	k = (m_idum) / IQ;
	m_idum = IA * (m_idum - k * IQ) - IR * k;

	if( m_idum < 0 ) 
		m_idum += IM;
	j = m_iy / NDIV;

	// We're seeing some strange memory corruption in the contents of s_pUniformStream. 
	// Perhaps it's being caused by something writing past the end of this array? 
	// Bounds-check in release to see if that's the case.
	if( j >= NTAB || j < 0 )
	{
		Warning( "GenerateRandomNumber had an array overrun: tried to write to element %d of 0..31.\n", j );
		j = ( j % NTAB ) & 0x7fffffff;
	}

	m_iy = m_iv[j];
	m_iv[j] = m_idum;

	return m_iy;
}

float RandomFloat( float flLow, float flHigh )
{
	// float in [0,1)
	float fl = AM * GenerateRandomNumber();
	if( fl > RNMX ) fl = RNMX;

	return (fl * ( flHigh - flLow ) ) + flLow; // float in [low,high)
}

vec_t ColorNormalize (const vec3_t in, vec3_t out)
{
	float	max, scale;

	max = in[0];
	if (in[1] > max)
		max = in[1];
	if (in[2] > max)
		max = in[2];

	if (max == 0)
		return 0;

	scale = 255.0 / max;

	VectorScale (in, scale, out);

	return max;
}

unsigned short FloatToHalf( float v )
{
	unsigned int	i = *((unsigned int *)&v);
	unsigned int	e = (i >> 23) & 0x00ff;
	unsigned int	m = i & 0x007fffff;
	unsigned short	h;

	if( e <= 127 - 15 )
		h = ((m | 0x00800000) >> (127 - 14 - e)) >> 13;
	else h = (i >> 13) & 0x3fff;

	h |= (i >> 16) & 0xc000;

	return h;
}

float HalfToFloat( unsigned short h )
{
	unsigned int	f = (h << 16) & 0x80000000;
	unsigned int	em = h & 0x7fff;

	if( em > 0x03ff )
	{
		f |= (em << 13) + ((127 - 15) << 23);
	}
	else
	{
		unsigned int m = em & 0x03ff;

		if( m != 0 )
		{
			unsigned int e = (em >> 10) & 0x1f;

			while(( m & 0x0400 ) == 0 )
			{
				m <<= 1;
				e--;
			}

			m &= 0x3ff;
			f |= ((e + (127 - 14)) << 23) | (m << 13);
		}
	}

	return *((float *)&f);
}