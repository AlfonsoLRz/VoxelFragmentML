#define MASK_POSITION 15

uint16_t masked(uint16_t value)
{
	return value | uint16_t(1 << MASK_POSITION);
}

uint16_t unmasked(uint16_t value)
{
	return value & uint16_t(~(1 << MASK_POSITION));
}

int unmasked_f(float value)
{
	return int(value) & (~(1 << MASK_POSITION));
}

bool isBoundary(uint16_t value)
{
	return (value >> MASK_POSITION) != uint16_t(0);
}

float isBoundary_f(float value)
{
	return float((int(value) >> MASK_POSITION) != 0);
}
