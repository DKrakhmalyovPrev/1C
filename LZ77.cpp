#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <fcntl.h>
#include <io.h>


#define DICBITS   12             
#define DICSIZE   (1<<DICBITS)  
#define THRESHOLD 2             
#define STRBITS   6              
#define STRMAX    ((1<<STRBITS)+THRESHOLD)
#define BUFSIZE   0xff00U       
#define TEXTSIZE  (BUFSIZE-DICSIZE-STRMAX) 
#define YES       1
#define NO        0


long fileleng;
int srcleng = 0;
unsigned char *srcbuf, *srcstart;


void coding(char fnamein[], char fnameout[], char code[], int p = 1) {
	FILE *in = fopen(fnamein, "rb");
	FILE *out = fopen(fnameout, "wb");
	if (in == NULL || out == NULL) return;
	int len = strlen(code);
	int i = 0;
	int ch;
	while ((ch = getc(in)) != EOF)
	{
		int c = (ch + p*code[i%len]) % 256;
		putc(c, out);
		i++;
	}
	fclose(in);
	fclose(out);
}


unsigned int crc16(char fname[])
{
	FILE *in = fopen(fname, "r");
	if (in == NULL) return 0;
	unsigned int crc = 0xFFFF;
	unsigned char i;
	unsigned char ch;
	while (fscanf(in, "%c", &ch) != EOF)
	{
		crc ^= ch << 8;
		for (i = 0; i < 8; i++)
		{
			if (crc & 0x8000)
				crc = (crc << 1) ^ 0x1021;
			else
				crc = crc << 1;
		}
	}
	fclose(in);
	return crc;
}


int putbits(int data, int nbits, char fileout[])
{
	FILE *out = fopen(fileout, "w");
	static int bitcounter = 0;
	static int outdata = 0;
	int bit, error;
	data <<= (16 - nbits);
	for (; nbits > 0; nbits--)
	{
		if (bitcounter == 8)
		{
			bitcounter = 0;
			error = putc(outdata, out);
		}
		outdata <<= 1;
		bit = (data & 0x8000) ? 1 : 0;
		outdata += bit;
		bitcounter++;
		data <<= 1;
	}
}


void compress_stud(char *filein, char *fileout)
{
	unsigned char  *position, *pointer;
	int i, dist, offset = 0, last = NO, cnt, maxleng;
	FILE *in = fopen(filein, "r");
	while ((srcleng = fread(srcstart + offset, 1, TEXTSIZE, in))>0)
	{
		if (srcleng < TEXTSIZE)
		{
			last = YES;
		}
		position = srcstart;
		pointer = srcbuf;
		srcleng += offset;
		printf("\n\nStep - %d\n", srcleng);
		maxleng = 0;
		if ((last == NO) && (srcleng < STRMAX))
		{
			memcpy(srcbuf, pointer, DICSIZE + (int)srcleng);
			offset = (int)srcleng;
			break;
		}
		for (i = DICSIZE - 1; i >= 0; i--)
		{
			for (cnt = 0; cnt < STRMAX; cnt++)
				if (*(position + cnt) != *(pointer + i + cnt))
					break;
			if (cnt <= THRESHOLD)
				continue;
			if (cnt == STRMAX)
			{
				dist = DICSIZE - 1 - i;
				maxleng = STRMAX;
				break;
			}
			if (cnt > maxleng)
			{
				dist = DICSIZE - 1 - i;
				maxleng = cnt;
			}
		}
		if ((last == YES) && (maxleng > srcleng))
		{
			maxleng = (int)srcleng;
		}
		if (maxleng > THRESHOLD)
		{
			putbits(1, 1, fileout);
			putbits(dist, DICBITS, fileout);
			putbits(maxleng - THRESHOLD - 1, STRBITS, fileout);
			position += maxleng;
			srcleng -= maxleng;
			pointer += maxleng;
		}
		else
		{
			putbits(0, 1, fileout);
			putbits(*position, 8, fileout);
			position++;
			srcleng--;
			pointer++;
		}
	}
	putbits(0, 8, fileout);
}


int getbits(int nbits, char filein[])
{
	static int bitcounter = 8;
	static int indata = 0;
	int bit, data = 0;
	FILE *in = fopen(filein, "r");
	for (; nbits > 0; nbits--)
	{
		if (bitcounter == 8)
		{
			bitcounter = 0;
			indata = getc(in);
		}
		bit = (indata & 0x80) ? 1 : 0;
		data <<= 1;
		data += bit;
		bitcounter++;
		indata <<= 1;
	}
	return data;
}


void decompress_stud(char *filein, char *fileout)
{
	unsigned char  *pos;
	int   i, dist, ch, maxleng;
	FILE *in = fopen(filein, "r");
	FILE *out = fopen(fileout, "w");
	read(fileno(in), &fileleng, sizeof(long));
	pos = srcstart;
	while (fileleng > 0)
	{
		if ((ch = getbits(1, filein)) == 0)
		{
			ch = getbits(8, filein);
			putc(ch, out);
			*pos = ch;
			pos++;
			fileleng--;
		}
		else
		{
			dist = getbits(DICBITS, filein) + 1;
			maxleng = getbits(STRBITS, filein) + THRESHOLD + 1;
			for (i = 0; i < maxleng; i++)
			{
				*(pos + i) = *(pos + i - dist);
				putc(*(pos + i - dist), out);
			}
			pos += maxleng;
			fileleng -= maxleng;
		}
		if (pos > srcstart + TEXTSIZE)
		{
			memcpy(srcbuf, pos - DICSIZE, DICSIZE);
			pos = srcstart;
		}
	}
}