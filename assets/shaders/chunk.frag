    #version 330

    in vec3 fragPosition;   // mesh.vertices
    in vec2 fragTexCoord;   // mesh.texcoords
    in vec4 fragColor;      // mesh.colors

    uniform float sunBrightness;
    uniform sampler2D texture0;

    out vec4 finalColor;

    void main() 
    {
        vec4 texColor = texture(texture0, fragTexCoord);
        finalColor =  texColor * vec4(fragColor.rgb * sunBrightness, fragColor.a);
    }