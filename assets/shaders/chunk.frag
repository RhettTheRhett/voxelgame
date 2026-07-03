#version 330

//in vec3 fragPosition;   // mesh.vertices
in vec2 fragTexCoord;   // mesh.texcoords
in vec4 fragColor;      // mesh.colors

uniform float sunBrightness;
uniform sampler2D texture0;

out vec4 finalColor;

void main()
{
    vec4 texColor = texture(texture0, fragTexCoord);
    float sunLit = fragColor.r * sunBrightness;
    float blockLit = fragColor.a;
    float finalBrightness = max(sunLit, blockLit);
    finalColor = texColor * vec4(vec3(finalBrightness), 1.0);
}