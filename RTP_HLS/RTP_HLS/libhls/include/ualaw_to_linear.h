#pragma once
#include <stdint.h>
#include <stddef.h>

int16_t ALaw_Decode(int8_t Alaw_Number);
int16_t MuLaw_Decode(int8_t Mulaw_Number);

void ALaw_Sequence_Decode(int8_t *Alaw_Sequence, size_t Alaw_size, int16_t *pcm);
void MuLaw_Sequence_Decode(int8_t *Mulaw_Sequence, size_t Alaw_size, int16_t* pcm);