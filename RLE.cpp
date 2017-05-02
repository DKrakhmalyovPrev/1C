#include <iostream>
#include <stdio.h>
#include <tchar.h>

using namespace std;

void encoderle(char mass[], char * out, int size)
{
	int counter = 0;
	int recurring_count = 0;
	for (int i = 0; i < size; i++)
	{
		if (mass[i] != mass[i + 1])
		{
			recurring_count++;
			out[counter++] = mass[i];
			out[counter++] = recurring_count + '0';
			recurring_count = 0;
		}
		else {
			recurring_count++;
		}
	}
}

void decoderle(char mass[], char * out, int size)
{
	int counter = 0;
	for (int i = 0; i < size; i++)
	{
		if (i & 1)
		{
			int num = mass[i] - '0';
			for (int j = 0; j < num; j++) {
				out[counter++] = mass[i - 1];
			}
		}
	}
}
