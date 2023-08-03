#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

float getMipLevel(in vec2 texCoord)
{
    vec2  dT_dx        	= dFdxFine(texCoord);
    vec2  dT_dy        	= dFdyFine(texCoord);
    float delta 		= 0.5 * max(sqrt(pow(dT_dx[0],2) + pow(dT_dx[1],2)), sqrt(pow(dT_dy[0],2) + pow(dT_dy[1],2)));
    float mipLevel 		= log2(delta);
    return max( 0, mipLevel );
}

void main() {

    float mipLevel = getMipLevel(fragTexCoord * textureSize(texSampler, 0)) - 0.5;
    
    outColor = textureLod(texSampler, fragTexCoord, mipLevel);
    //outColor = texture(texSampler, fragTexCoord);
    if (mipLevel < 0.5f) {
        outColor.r = 1.0f;
    } else if (mipLevel < 1.5f) {
        outColor.g = 1.0f;
    } else if (mipLevel < 2.5f) {
        outColor.r = 1.0f;
        outColor.g = 1.0f;
    } else if (mipLevel < 3.5f) {
        outColor.b = 1.0f;
    } else if (mipLevel < 4.5f) {
        outColor.b = 1.0f;
        outColor.r = 1.0f;
    } else if (mipLevel < 5.5f) {
        outColor.b = 1.0f;
        outColor.g = 1.0f;
    } else if (mipLevel < 6.5f) {
        outColor.b = 1.0f;
        outColor.g = 1.0f;
        outColor.r = 1.0f;
    }
}