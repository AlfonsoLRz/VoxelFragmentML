#define MASK_BOUNDARY_POSITION uint(15)
#define MASK_ID_POSITION uint(12)

uint16_t maskedBit(uint16_t value, uint position)
{
	return value | uint16_t(1 << position);
}

uint16_t unmaskedBit(uint16_t value, uint position)
{
	return value & uint16_t(~(1 << position));
}

uint16_t unmasked(uint16_t value, uint position)
{
	return value & uint16_t((1 << position) - 1);
}

int unmasked_f(float value, uint position)
{
	return int(value) & (~(1 << position));
}

bool isBoundary(uint16_t value, uint position)
{
	return (value >> position) != uint16_t(0);
}

float isBoundary_f(float value, uint position)
{
	return float((int(value) >> position) != 0);
}
