#define PHI 0.618033988749895f

float getHueValue(uint id)
{
	const float h = id * PHI;

	return mod(h, 1.0f);
}

vec3 HSVtoRGB(float h, float s, float v)
{
	const int h_i = int(h * 6.0f);
	const float f = h * 6.0f - h_i;
	const float p = v * (1.0f - s);
	const float q = v * (1.0f - f * s);
	const float t = v * (1.0f - (1.0f - f) * s);

	switch (h_i)
	{
	case 0:
		return vec3(v, t, p);
	case 1:
		return vec3(q, v, p);
	case 2:
		return vec3(p, v, t);
	case 3:
		return vec3(p, q, v);
	case 4:
		return vec3(t, p, v);
	case 5:
		return vec3(v, p, q);
	}

	return vec3(.0f);
}