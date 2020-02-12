#include "../include/ualaw_to_linear.h"

int16_t ALaw_Decode(int8_t number)
{
	uint8_t sign = 0x00;
	uint8_t position = 0;
	int16_t decoded = 0;
	number ^= 0x55;
	if (number & 0x80)
	{
		number &= ~(1 << 7);
		sign = -1;
	}
	position = ((number & 0xF0) >> 4) + 4;
	if (position != 4)
	{
		decoded = ((1 << position) | ((number & 0x0F) << (position - 4))
			| (1 << (position - 5)));
	}
	else
	{
		decoded = (number << 1) | 1;
	}
	return (sign == 0) ? (decoded) : (-decoded);
}

int16_t MuLaw_Decode(int8_t number)
{
	const uint16_t MULAW_BIAS = 33;
	uint8_t sign = 0, position = 0;
	int16_t decoded = 0;
	number = ~number;
	if (number & 0x80)
	{
		number &= ~(1 << 7);
		sign = -1;
	}
	position = ((number & 0xF0) >> 4) + 5;
	decoded = ((1 << position) | ((number & 0x0F) << (position - 4))
		| (1 << (position - 5))) - MULAW_BIAS;
	return (sign == 0) ? (decoded) : (-(decoded));
}

void ALaw_Sequence_Decode(int8_t *Alaw_Sequence, size_t Alaw_size, int16_t* pcm)
{
	for (int i = 0; i < Alaw_size; ++i)
		pcm[i] = ALaw_Decode(Alaw_Sequence[i]);
}

void MuLaw_Sequence_Decode(int8_t * Mulaw_Sequence, size_t Alaw_size, int16_t * pcm)
{
	for (int i = 0; i < Alaw_size; ++i)
		pcm[i] = MuLaw_Decode(Mulaw_Sequence[i]);
}