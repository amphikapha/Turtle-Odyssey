#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uiTexture;

void main()
{
    vec4 tex = texture(uiTexture, TexCoord);
    // Premultiplied alpha isn't assumed; use normal blending
    FragColor = tex;
}
