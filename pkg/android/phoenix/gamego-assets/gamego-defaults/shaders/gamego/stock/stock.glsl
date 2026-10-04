#if __VERSION__ >= 130
#define ATTRIBUTE in
#define TEXTURE texture
#if defined(VERTEX)
#define VARYING out
#else
#define VARYING in
out vec4 FragColor;
#endif
#else
#define ATTRIBUTE attribute
#define VARYING varying
#define TEXTURE texture2D
#define FragColor gl_FragColor
#endif
#ifdef GL_ES
precision mediump float;
#endif
#if defined(VERTEX)
ATTRIBUTE vec4 VertexCoord;
ATTRIBUTE vec4 TexCoord;
uniform mat4 MVPMatrix;
VARYING vec2 tex_coord;
void main()
{
   gl_Position = MVPMatrix * VertexCoord;
   tex_coord = TexCoord.xy;
}
#elif defined(FRAGMENT)
VARYING vec2 tex_coord;
uniform sampler2D Texture;
void main()
{
   FragColor = TEXTURE(Texture, tex_coord);
}
#endif
