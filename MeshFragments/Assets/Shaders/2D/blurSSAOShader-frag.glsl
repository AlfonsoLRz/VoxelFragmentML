#version 450

// ********** PARAMETERS & VARIABLES ***********

#define NEIGHBORHOOD_SIZE 16.0f

in vec2 textCoord;

uniform vec3 backgroundColor;
uniform sampler2D texAlbedo;
uniform sampler2D texSSAO;

layout (location = 0) out vec4 fColor;

void main(void)
{
    vec2 texelSize = 1.0f / vec2(textureSize(texSSAO, 0));

    // 4 x 4 neighborhood
    float result = 0.0f;
    for (int x = -2; x < 2; ++x) 
    {
        for (int y = -2; y < 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(texSSAO, textCoord + offset).r;
        }
    }

    result /= NEIGHBORHOOD_SIZE;

    vec4 colorRGBA = texture(texAlbedo, textCoord);
	fColor = vec4(colorRGBA.rgb * result, float(distance(backgroundColor, colorRGBA.rgb) > .01f));
    //fColor = vec4(vec3(texture(texSSAO, textCoord).rgb), 1.0f);
}
