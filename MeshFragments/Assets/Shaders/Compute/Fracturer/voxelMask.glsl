#define MASK_POSITION 15

uint8_t masked(uint8_t value)
{
	return value | uint8_t(1 << MASK_POSITION);
}

uint8_t unmasked(uint8_t value)
{
	return value & uint8_t(~(1 << MASK_POSITION));
}

int unmasked_f(float value)
{
	return int(value) & (~(1 << MASK_POSITION));
}

bool isBoundary(uint8_t value)
{
	return (value >> MASK_POSITION) != uint8_t(0);
}

float isBoundary_f(float value)
{
	return float((int(value) >> MASK_POSITION) != 0);
}
