#include "lzw.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LZW_CHECKEOS            /* include checks for strips w/o EOI code */

#define MAXCODE(n)	((1L<<(n))-1)
/*
* The TIFF spec specifies that encoded bit
* strings range from 9 to 12 bits.
*/
#define BITS_MIN        9               /* start with 9 bits */
#define BITS_MAX        12              /* max of 12 bit strings */
/* predefined codes */
#define CODE_CLEAR      256             /* code to clear string table */
#define CODE_EOI        257             /* end-of-information code */
#define CODE_FIRST      258             /* first free code entry */
#define CODE_MAX        MAXCODE(BITS_MAX)
#define HSIZE           9001L           /* 91% occupancy */
#define HSHIFT          (13-8)
#ifdef LZW_COMPAT
/* NB: +1024 is for compatibility with old files */
#define CSIZE           (MAXCODE(BITS_MAX)+1024L)
#else
#define CSIZE           (MAXCODE(BITS_MAX)+1L)
#endif

typedef int(*decodeFunc)(uint8_t*, uint64_t, uint16_t);

typedef struct {
	//TIFFPredictorState predict;     /* predictor super class */

	unsigned short  nbits;          /* # of bits/code */
	unsigned short  maxcode;        /* maximum code for lzw_nbits */
	unsigned short  free_ent;       /* next free entry in hash table */
	unsigned long   nextdata;       /* next bits of i/o */
	long            nextbits;       /* # of valid bits in lzw_nextdata */

	int             rw_mode;        /* preserve rw_mode from init */
} LZWBaseState;

#define lzw_nbits       base.nbits
#define lzw_maxcode     base.maxcode
#define lzw_free_ent    base.free_ent
#define lzw_nextdata    base.nextdata
#define lzw_nextbits    base.nextbits

typedef struct code_ent {
	struct code_ent *next;
	unsigned short	length;		/* string len, including this token */
	unsigned char	value;		/* data value */
	unsigned char	firstchar;	/* first token of string */
} code_t;

typedef uint16_t hcode_t;			/* codes fit in 16 bits */
typedef struct {
	long	hash;
	hcode_t	code;
} hash_t;


typedef struct {
	LZWBaseState base;

	/* Decoding specific data */
	long    dec_nbitsmask;		/* lzw_nbits 1 bits, right adjusted */
	long    dec_restart;		/* restart count */
#ifdef LZW_CHECKEOS
	uint64_t  dec_bitsleft;		/* available bits in raw data */
#endif
	decodeFunc dec_decode;		/* regular or backwards compatible */
	code_t* dec_codep;		/* current recognized code */
	code_t* dec_oldcodep;		/* previously recognized code */
	code_t* dec_free_entp;		/* next free entry */
	code_t* dec_maxcodep;		/* max available entry */
	code_t* dec_codetab;		/* kept separate for small machines */

								/* Encoding specific data */
	int     enc_oldcode;		/* last code encountered */
	long    enc_checkpoint;		/* point at which to clear table */
#define CHECK_GAP	10000		/* enc_ratio check interval */
	long    enc_ratio;		/* current compression ratio */
	long    enc_incount;		/* (input) data bytes encoded */
	long    enc_outcount;		/* encoded (output) bytes */
	uint8_t*  enc_rawlimit;		/* bound on tif_rawdata buffer */
	hash_t* enc_hashtab;		/* kept separate for small machines */
} LZWCodecState;

#define	CALCRATIO(sp, rat) {					\
	if (incount > 0x007fffff) { /* NB: shift will overflow */\
		rat = outcount >> 8;				\
		rat = (rat == 0 ? 0x7fffffff : incount/rat);	\
	} else							\
		rat = (incount<<8) / outcount;			\
}



/* Explicit 0xff masking to make icc -check=conversions happy */
#define	PutNextCode(op, c) {					\
	nextdata = (nextdata << nbits) | c;			\
	nextbits += nbits;					\
	*op++ = (unsigned char)((nextdata >> (nextbits-8))&0xff);		\
	nextbits -= 8;						\
	if (nextbits >= 8) {					\
		*op++ = (unsigned char)((nextdata >> (nextbits-8))&0xff);	\
		nextbits -= 8;					\
	}							\
	outcount += nbits;					\
}

#define LZWState(tif)		((LZWBaseState*) (tif)->tif_data)
#define DecoderState(tif)	((LZWCodecState*) LZWState(tif))
#define EncoderState(tif)	((LZWCodecState*) LZWState(tif))


static void
cl_hash(hash_t * enc_hashtab)
{
	hash_t *hp = &enc_hashtab[HSIZE - 1];
	long i = HSIZE - 8;

	do {
		i -= 8;
		hp[-7].hash = -1;
		hp[-6].hash = -1;
		hp[-5].hash = -1;
		hp[-4].hash = -1;
		hp[-3].hash = -1;
		hp[-2].hash = -1;
		hp[-1].hash = -1;
		hp[0].hash = -1;
		hp -= 8;
	} while (i >= 0);
	for (i += 8; i > 0; i--, hp--)
		hp->hash = -1;
}
static int
LZWPreEncode(LZWCodecState* sp, void* output_data, uint64_t max_output_size)
{
	sp->enc_hashtab = (hash_t*)malloc(HSIZE * sizeof(hash_t));
	sp->lzw_nbits = BITS_MIN;
	sp->lzw_maxcode = MAXCODE(BITS_MIN);
	sp->lzw_free_ent = CODE_FIRST;
	sp->lzw_nextbits = 0;
	sp->lzw_nextdata = 0;
	sp->enc_checkpoint = CHECK_GAP;
	sp->enc_ratio = 0;
	sp->enc_incount = 0;
	sp->enc_outcount = 0;
	/*
	 * The 4 here insures there is space for 2 max-sized
	 * codes in LZWEncode and LZWPostDecode.
	 */
	sp->enc_rawlimit = (uint8_t*)output_data + max_output_size - 1 - 4;
	cl_hash(sp->enc_hashtab);
	sp->enc_oldcode = (hcode_t)-1;
	return 1;
}

static int LZWPostEncode(LZWCodecState* sp, void* output_data, void* output_data_current_position);

int LZWEncode(void* raw_data, uint64_t raw_data_size, uint64_t* raw_data_used_size, void* output_data, uint64_t max_output_size, uint64_t* output_data_used_size)
{
	long fcode;
	hash_t *hp;
	int h, c;
	hcode_t ent;
	long disp;
	long incount, outcount, checkpoint = 0;
	unsigned long nextdata;
	long nextbits;
	int free_ent, maxcode, nbits;
	uint8_t* op;
	uint8_t* limit;

	/***********Add code start*****************/
	uint64_t cc = raw_data_size;
	uint8_t* bp = (uint8_t*)raw_data;
	op = (uint8_t*)output_data;
	limit = op + max_output_size - 1 - 4;

	LZWCodecState state;
	LZWCodecState* sp = &state;
	LZWPreEncode(sp, output_data, max_output_size);
	/***********Add code end*******************/

		/*
		 * Load local state.
		 */
	incount = sp->enc_incount;
	outcount = sp->enc_outcount;
	checkpoint = sp->enc_checkpoint;
	nextdata = sp->lzw_nextdata;
	nextbits = sp->lzw_nextbits;
	free_ent = sp->lzw_free_ent;
	maxcode = sp->lzw_maxcode;
	nbits = sp->lzw_nbits;
	ent = (hcode_t)sp->enc_oldcode;

	if (ent == (hcode_t)-1 && cc > 0) {
		/*
		* NB: This is safe because it can only happen
		*     at the start of a strip where we know there
		*     is space in the data buffer.
		*/
		PutNextCode(op, CODE_CLEAR);
		ent = *bp++; cc--; incount++;
	}
	while (cc > 0) {
		c = *bp++; cc--; incount++;
		fcode = ((long)c << BITS_MAX) + ent;
		h = (c << HSHIFT) ^ ent;	/* xor hashing */
//#ifdef _WINDOWS
									/*
									* Check hash index for an overflow.
									*/
		if (h >= HSIZE)
			h -= HSIZE;
		//#endif
		hp = &sp->enc_hashtab[h];
		if (hp->hash == fcode) {
			ent = hp->code;
			continue;
		}
		if (hp->hash >= 0) {
			/*
			* Primary hash failed, check secondary hash.
			*/
			disp = HSIZE - h;
			if (h == 0)
				disp = 1;
			do {
				/*
				* Avoid pointer arithmetic because of
				* wraparound problems with segments.
				*/
				if ((h -= disp) < 0)
					h += HSIZE;
				hp = &sp->enc_hashtab[h];
				if (hp->hash == fcode) {
					ent = hp->code;
					goto hit;
				}
			} while (hp->hash >= 0);
		}
		/*
		* New entry, emit code and add to table.
		*/
		/*
		* Verify there is space in the buffer for the code
		* and any potential Clear code that might be emitted
		* below.  The value of limit is setup so that there
		* are at least 4 bytes free--room for 2 codes.
		*/
		if (op > limit) {
			//tif->tif_rawcc = (uint64_t)(op - tif->tif_rawdata);
			//if (!TIFFFlushData1(tif))
			*output_data_used_size = op - (uint8_t*)output_data;
			*raw_data_used_size = bp - (uint8_t*)raw_data;
			free(sp->enc_hashtab);
			return 0;
			//op = tif->tif_rawdata;
		}
		PutNextCode(op, ent);
		ent = (hcode_t)c;
		hp->code = (hcode_t)(free_ent++);
		hp->hash = fcode;
		if (free_ent == CODE_MAX - 1) {
			/* table is full, emit clear code and reset */
			cl_hash(sp->enc_hashtab);
			sp->enc_ratio = 0;
			incount = 0;
			outcount = 0;
			free_ent = CODE_FIRST;
			PutNextCode(op, CODE_CLEAR);
			nbits = BITS_MIN;
			maxcode = MAXCODE(BITS_MIN);
		}
		else {
			/*
			* If the next entry is going to be too big for
			* the code size, then increase it, if possible.
			*/
			if (free_ent > maxcode) {
				nbits++;
				//assert(nbits <= BITS_MAX);
				maxcode = (int)MAXCODE(nbits);
			}
			else if (incount >= checkpoint) {
				long rat;
				/*
				* Check compression ratio and, if things seem
				* to be slipping, clear the hash table and
				* reset state.  The compression ratio is a
				* 24+8-bit fractional number.
				*/
				checkpoint = incount + CHECK_GAP;
				CALCRATIO(sp, rat);
				if (rat <= sp->enc_ratio) {
					cl_hash(sp->enc_hashtab);
					sp->enc_ratio = 0;
					incount = 0;
					outcount = 0;
					free_ent = CODE_FIRST;
					PutNextCode(op, CODE_CLEAR);
					nbits = BITS_MIN;
					maxcode = MAXCODE(BITS_MIN);
				}
				else
					sp->enc_ratio = rat;
			}
		}
	hit:
		;
	}

	/*
	 * Restore global state.
	 */
	sp->enc_incount = incount;
	sp->enc_outcount = outcount;
	sp->enc_checkpoint = checkpoint;
	sp->enc_oldcode = ent;
	sp->lzw_nextdata = nextdata;
	sp->lzw_nextbits = nextbits;
	sp->lzw_free_ent = (unsigned short)free_ent;
	sp->lzw_maxcode = (unsigned short)maxcode;
	sp->lzw_nbits = (unsigned short)nbits;
	*output_data_used_size = LZWPostEncode(sp, output_data, op);
	*raw_data_used_size = bp - (uint8_t*)raw_data;
	return (1);
}

/*
 * Finish off an encoded strip by flushing the last
 * string and tacking on an End Of Information code.
 */
static int
LZWPostEncode(LZWCodecState* sp, void* output_data, void* output_data_current_position)
{

	uint8_t* op = (uint8_t*)output_data_current_position;
	long nextbits = sp->lzw_nextbits;
	unsigned long nextdata = sp->lzw_nextdata;
	long outcount = sp->enc_outcount;
	int nbits = sp->lzw_nbits;

	if (op > sp->enc_rawlimit) {
		//	tif->tif_rawcc = (tmsize_t)(op - tif->tif_rawdata);
		//	if (!TIFFFlushData1(tif))
		free(sp->enc_hashtab);
		return 0;
		//	op = tif->tif_rawdata;
	}
	if (sp->enc_oldcode != (hcode_t)-1) {
		int free_ent = sp->lzw_free_ent;

		PutNextCode(op, sp->enc_oldcode);
		sp->enc_oldcode = (hcode_t)-1;
		free_ent++;

		if (free_ent == CODE_MAX - 1) {
			/* table is full, emit clear code and reset */
			outcount = 0;
			PutNextCode(op, CODE_CLEAR);
			nbits = BITS_MIN;
		}
		else {
			/*
			* If the next entry is going to be too big for
			* the code size, then increase it, if possible.
			*/
			if (free_ent > sp->lzw_maxcode) {
				nbits++;
				//assert(nbits <= BITS_MAX);
			}
		}
	}
	PutNextCode(op, CODE_EOI);
	/* Explicit 0xff masking to make icc -check=conversions happy */
	if (nextbits > 0)
		*op++ = (unsigned char)((nextdata << (8 - nextbits)) & 0xff);

	free(sp->enc_hashtab);
	return (int32_t)(op - (uint8_t*)output_data);
}


#ifdef LZW_CHECKEOS
/*
* This check shouldn't be necessary because each
* strip is suppose to be terminated with CODE_EOI.
*/
#define	NextCode(_dec_bitsleft, _bp, _code, _get) {				\
	if (_dec_bitsleft < (uint64_t)nbits) {			\
		_code = CODE_EOI;					\
	} else {							\
		_get(_bp,_code);					\
		_dec_bitsleft -= nbits;				\
	}								\
}
#else
#define	NextCode(tif, sp, bp, code, get) get(sp, bp, code)
#endif

/*
* Decode a "hunk of data".
*/
#define	GetNextCode(bp, code) {				\
	nextdata = (nextdata<<8) | *(bp)++;			\
	nextbits += 8;						\
	if (nextbits < nbits) {					\
		nextdata = (nextdata<<8) | *(bp)++;		\
		nextbits += 8;					\
	}							\
	code = (hcode_t)((nextdata >> (nextbits-nbits)) & nbitsmask);	\
	nextbits -= nbits;					\
}

//LZWDecode(uint8* op0, tmsize_t occ0, uint16 s)
int LZWDecode(void* input_data, uint64_t input_data_size, void* output_data, uint64_t max_output_size)
{
	char *op = (char*)output_data;
	long occ = (long)max_output_size;
	char *tp;
	unsigned char *bp;
	hcode_t code;
	int len;
	long nbits = 0, nextbits = BITS_MIN, nbitsmask = MAXCODE(BITS_MIN);;
	unsigned long nextdata;
	code_t *codep, *free_entp = NULL, *maxcodep = NULL, *oldcodep = NULL;
	code_t* dec_codetab = (code_t*)malloc(CSIZE * sizeof(code_t));

	/*
	* Pre-load the table.
	*/
	code = 255;
	do {
		dec_codetab[code].value = (unsigned char)code;
		dec_codetab[code].firstchar = (unsigned char)code;
		dec_codetab[code].length = 1;
		dec_codetab[code].next = NULL;
	} while (code--);
	/*
	* Zero-out the unused entries
	*/
	memset(&dec_codetab[CODE_CLEAR], 0,
		(CODE_FIRST - CODE_CLEAR) * sizeof(code_t));
	free_entp = dec_codetab + CODE_FIRST;
	oldcodep = &dec_codetab[-1];
	maxcodep = &dec_codetab[nbitsmask - 1];
	/*
	Fail if value does not fit in long.
	*/
	//if ((tmsize_t)occ != occ0)
	//	return (0);
	/*
	* Restart interrupted output operation.
	*/
	//if (sp->dec_restart) {
	//	long residue;

	//	codep = sp->dec_codep;
	//	residue = codep->length - sp->dec_restart;
	//	if (residue > occ) {
	//		/*
	//		* Residue from previous decode is sufficient
	//		* to satisfy decode request.  Skip to the
	//		* start of the decoded string, place decoded
	//		* values in the output buffer, and return.
	//		*/
	//		sp->dec_restart += occ;
	//		do {
	//			codep = codep->next;
	//		} while (--residue > occ && codep);
	//		if (codep) {
	//			tp = op + occ;
	//			do {
	//				*--tp = codep->value;
	//				codep = codep->next;
	//			} while (--occ && codep);
	//		}
	//		return (1);
	//	}
	//	/*
	//	* Residue satisfies only part of the decode request.
	//	*/
	//	op += residue;
	//	occ -= residue;
	//	tp = op;
	//	do {
	//		int t;
	//		--tp;
	//		t = codep->value;
	//		codep = codep->next;
	//		*tp = (char)t;
	//	} while (--residue && codep);
	//	sp->dec_restart = 0;
	//}

	bp = (unsigned char *)input_data;
#ifdef LZW_CHECKEOS
	uint64_t dec_bitsleft = (input_data_size << 3);
#endif
	nbits = BITS_MIN;
	nextdata = 0;
	nextbits = 0;
	while (occ > 0) {
		NextCode(dec_bitsleft, bp, code, GetNextCode);
		if (code == CODE_EOI)
			break;
		if (code == CODE_CLEAR) {
			do {
				free_entp = dec_codetab + CODE_FIRST;
				memset(free_entp, 0,
					(CSIZE - CODE_FIRST) * sizeof(code_t));
				nbits = BITS_MIN;
				nbitsmask = MAXCODE(BITS_MIN);
				maxcodep = dec_codetab + nbitsmask - 1;
				NextCode(dec_bitsleft, bp, code, GetNextCode);
			} while (code == CODE_CLEAR);	/* consecutive CODE_CLEAR codes */
			if (code == CODE_EOI)
				break;
			if (code > CODE_CLEAR) {
				/*			TIFFErrorExt(tif->tif_clientdata, tif->tif_name,
								"LZWDecode: Corrupted LZW table at scanline %d",
								tif->tif_row);*/
				free(dec_codetab);
				return (0);
			}
			*op++ = (char)code;
			occ--;
			oldcodep = dec_codetab + code;
			continue;
		}
		codep = dec_codetab + code;

		/*
		* Add the new entry to the code table.
		*/
		if (free_entp < &dec_codetab[0] ||
			free_entp >= &dec_codetab[CSIZE]) {
			//TIFFErrorExt(tif->tif_clientdata, module,
			//	"Corrupted LZW table at scanline %d",
			//	tif->tif_row);
			free(dec_codetab);
			return (0);
		}

		free_entp->next = oldcodep;
		if (free_entp->next < &dec_codetab[0] ||
			free_entp->next >= &dec_codetab[CSIZE]) {
			//TIFFErrorExt(tif->tif_clientdata, module,
			//	"Corrupted LZW table at scanline %d",
			//	tif->tif_row);
			free(dec_codetab);
			return (0);
		}
		free_entp->firstchar = free_entp->next->firstchar;
		free_entp->length = free_entp->next->length + 1;
		free_entp->value = (codep < free_entp) ?
			codep->firstchar : free_entp->firstchar;
		if (++free_entp > maxcodep) {
			if (++nbits > BITS_MAX)		/* should not happen */
				nbits = BITS_MAX;
			nbitsmask = MAXCODE(nbits);
			maxcodep = dec_codetab + nbitsmask - 1;
		}
		oldcodep = codep;
		if (code >= 256) {
			/*
			* Code maps to a string, copy string
			* value to output (written in reverse).
			*/
			if (codep->length == 0) {
				//TIFFErrorExt(tif->tif_clientdata, module,
				//	"Wrong length of decoded string: "
				//	"data probably corrupted at scanline %d",
				//	tif->tif_row);
				free(dec_codetab);
				return (0);
			}
			//if (codep->length > occ) {
			//	/*
			//	* String is too long for decode buffer,
			//	* locate portion that will fit, copy to
			//	* the decode buffer, and setup restart
			//	* logic for the next decoding call.
			//	*/
			//	sp->dec_codep = codep;
			//	do {
			//		codep = codep->next;
			//	} while (codep && codep->length > occ);
			//	if (codep) {
			//		sp->dec_restart = (long)occ;
			//		tp = op + occ;
			//		do {
			//			*--tp = codep->value;
			//			codep = codep->next;
			//		} while (--occ && codep);
			//		if (codep)
			//			codeLoop(tif, module);
			//	}
			//	break;
			//}
			len = codep->length;
			tp = op + len;
			do {
				int t;
				--tp;
				t = codep->value;
				codep = codep->next;
				*tp = (char)t;
			} while (codep && tp > op);
			//if (codep) {
			//	codeLoop(tif, module);
			//	break;
			//}
			//assert(occ >= len);
			op += len;
			occ -= len;
		}
		else {
			*op++ = (char)code;
			occ--;
		}
	}

	//tif->tif_rawcc -= (tmsize_t)((uint8*)bp - tif->tif_rawcp);
	//tif->tif_rawcp = (uint8*)bp;
	//sp->lzw_nbits = (unsigned short)nbits;
	//sp->lzw_nextdata = nextdata;
	//sp->lzw_nextbits = nextbits;
	//sp->dec_nbitsmask = nbitsmask;
	//sp->dec_oldcodep = oldcodep;
	//sp->dec_free_entp = free_entp;
	//sp->dec_maxcodep = maxcodep;

//	if (occ > 0) {
//#if defined(__WIN32__) && (defined(_MSC_VER) || defined(__MINGW32__))
//		TIFFErrorExt(tif->tif_clientdata, module,
//			"Not enough data at scanline %d (short %I64d bytes)",
//			tif->tif_row, (unsigned __int64)occ);
//#else
//		TIFFErrorExt(tif->tif_clientdata, module,
//			"Not enough data at scanline %d (short %llu bytes)",
//			tif->tif_row, (unsigned long long) occ);
//#endif
//		return (0);
//	}
	free(dec_codetab);
	return (int)(op - (char*)output_data);
}
