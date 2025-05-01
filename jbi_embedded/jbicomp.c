/*****************************************************************************/
/*                                                                           */
/* Module:           jbicomp.c                                               */
/*                                                                           */
/*                   Copyright 2025 Altera Corporation                       */
/*                                                                           */
/* SPDX-License-Identifier: MIT-0                                            */
/*                                                                           */
/* Permission is hereby granted, free of charge, to any person obtaining a   */
/* copy of this software and associated documentation files (the             */
/* "Software"),to deal in the Software without restriction, including        */
/* without limitation the rights to use, copy, modify, merge, publish,       */
/* distribute, sublicense, and/or sell copies of the Software, and to permit */
/* persons to whom the Software is furnished to do so.                       */
/*                                                                           */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS   */
/* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF                */
/* MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY      */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT */
/* OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR  */
/* THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                */
/*                                                                           */
/* Description:      Contains the code for compressing and uncompressing     */
/*                   Boolean array data.                                     */
/*                                                                           */
/*                   This algorithm works by searching previous bytes in the */
/*                   data that match the current data. If a match is found,  */
/*                   then the offset and length of the matching data can     */
/*                   replace the actual data in the output.                  */
/*                                                                           */
/* Revisions:        2.2  fixed /W4 warnings                                 */
/*                                                                           */
/*****************************************************************************/

#include "jbiexprt.h"
#include "jbicomp.h"

#define	SHORT_BITS			16
#define	CHAR_BITS			8
#define	DATA_BLOB_LENGTH	3
#define	MATCH_DATA_LENGTH	8192
#define JBI_ACA_REQUEST_SIZE 1024
#define JBI_ACA_BUFFER_SIZE	(MATCH_DATA_LENGTH + JBI_ACA_REQUEST_SIZE)

unsigned long jbi_in_length = 0L;
unsigned long jbi_in_index = 0L;	/* byte index into compressed array */
unsigned int jbi_bits_avail = CHAR_BITS;


/****************************************************************************/
/*																			*/
/*	The following functions implement incremental decompression of Boolean	*/
/*	array data, using a small memory window.								*/
/*																			*/
/*	This algorithm works by searching previous bytes in the data that match	*/
/*	the current data. If a match is found, then the offset and length of	*/
/*	the matching data can replace the actual data in the output.			*/
/*																			*/
/*	Memory usage is reduced by maintaining a "window" buffer which contains	*/
/*	the uncompressed data for one 8K page, plus some extra amount specified	*/
/*	by JBI_ACA_REQUEST_SIZE.  The function jbi_uncompress_page() is used to	*/
/*	request a subrange of the uncompressed data, starting at a particular	*/
/*	bit position and extending a maximum of JBI_ACA_REQUEST_SIZE bytes.		*/
/*																			*/
/****************************************************************************/

/****************************************************************************/
/*																			*/

unsigned int jbi_bits_required(unsigned int n)

/*																			*/
/*	Description:	Calculate the minimum number of bits required to		*/
/*					represent n.											*/
/*																			*/
/*	Returns:		Number of bits.											*/
/*																			*/
/****************************************************************************/
{
	unsigned int result = SHORT_BITS;

	if (n == 0)
	{
		result = 1;
	}
	else
	{
		/* Look for the highest non-zero bit position */
		while ((n & (1 << (SHORT_BITS - 1))) == 0)
		{
			n <<= 1;
			--result;
		}
	}

	return (result);
}

/****************************************************************************/
/*																			*/

unsigned int jbi_read_packed
(
	unsigned char *buffer,
	unsigned int bits
)

/*																			*/
/*	Description:	Read the next value from the input array "buffer".		*/
/*					Read only "bits" bits from the array. The amount of		*/
/*					bits that have already been read from "buffer" is		*/
/*					stored internally to this function.					 	*/
/*																			*/
/*	Returns:		Up to 16 bit value. -1 if buffer overrun.				*/
/*																			*/
/****************************************************************************/
{
	unsigned int result = 0;
	unsigned int shift = 0;
	unsigned int databyte = 0;

	while (bits > 0)
	{

		databyte = buffer[jbi_in_index];

		result |= (((databyte >> (CHAR_BITS - jbi_bits_avail))
			& (0xFF >> (CHAR_BITS - jbi_bits_avail))) << shift);

		if (bits <= jbi_bits_avail)
		{
			result &= (0xFFFF >> (SHORT_BITS - (bits + shift)));
			jbi_bits_avail -= bits;
			bits = 0;
		}
		else
		{
			++jbi_in_index;
			shift += jbi_bits_avail;
			bits -= jbi_bits_avail;
			jbi_bits_avail = CHAR_BITS;
		}
	}

	return (result);
}



/****************************************************************************/
/*																			*/

unsigned long jbi_uncompress
(
	unsigned char *in, 
	unsigned long in_length, 
	unsigned char *out, 
	unsigned long out_length,
	int version
)

/*																			*/
/*	Description:	Uncompress data in "in" and write result to	"out".		*/
/*																			*/
/*	Returns:		Length of uncompressed data. -1 if:						*/
/*						1) out_length is too small							*/
/*						2) Internal error in the code						*/
/*						3) in doesn't contain ACA compressed data.			*/
/*																			*/
/****************************************************************************/
{
	unsigned long i, j, data_length = 0L;
	unsigned int offset, length;
	unsigned int match_data_length = MATCH_DATA_LENGTH;

	if (version > 0) --match_data_length;

	jbi_in_length = in_length;
	jbi_bits_avail = CHAR_BITS;
	jbi_in_index = 0L;
	for (i = 0; i < out_length; ++i) out[i] = 0;

	/* Read number of bytes in data. */
	for (i = 0; i < sizeof (in_length); ++i) 
	{
		data_length = data_length | ((unsigned long)
			jbi_read_packed(in, CHAR_BITS) << (i * CHAR_BITS));
	}

	if (data_length > out_length)
	{
		data_length = 0L;
	}
	else
	{
		i = 0;
		while (i < data_length)
		{
			/* A 0 bit indicates literal data. */
			if (jbi_read_packed(in, 1) == 0)
			{
				for (j = 0; j < DATA_BLOB_LENGTH; ++j)
				{
					if (i < data_length)
					{
						out[i] = (unsigned char) jbi_read_packed(in, CHAR_BITS);
						i++;
					}
				}
			}
			else
			{
				/* A 1 bit indicates offset/length to follow. */
				offset = jbi_read_packed(in, jbi_bits_required((short) (i > match_data_length ? match_data_length : i)));
				length = jbi_read_packed(in, CHAR_BITS);

				for (j = 0; j < length; ++j)
				{
					if (i < data_length)
					{
						out[i] = out[i - offset];
						i++;
					}
				}
			}
		}
	}

	return (data_length);
}