#version 450
layout(push_constant) uniform Push
{
   mat4 MVP;
} params;
#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
void main()
{
   gl_Position = params.MVP * Position;
   vTexCoord = TexCoord;
}
#pragma stage fragment
layout(binding = 2) uniform sampler2D Source;
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
void main()
{
   FragColor = texture(Source, vTexCoord);
}
