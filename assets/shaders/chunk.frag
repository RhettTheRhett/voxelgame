#version 330

//in vec3 fragPosition;   // mesh.vertices
in vec2 fragTexCoord;   // mesh.texcoords
in vec4 fragColor;      // mesh.colors — RGB = sun shade, A = block light

uniform float sunBrightness;
uniform sampler2D texture0;

out vec4 finalColor;

void main()
{
    vec4 texColor = texture(texture0, fragTexCoord);
    // Fully transparent atlas texels (empty tiles) — never draw.
    if (texColor.a < 0.01) discard;

    float sunLit = fragColor.r * sunBrightness;
    float blockLit = fragColor.a;
    float finalBrightness = max(sunLit, blockLit);
    // Texture alpha drives see-through (glass); vertex A stays block-light only.
    finalColor = vec4(texColor.rgb * finalBrightness, texColor.a);
}
