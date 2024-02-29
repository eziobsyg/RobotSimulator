#version 120

uniform mat4 P;
uniform mat4 MV;

attribute vec4 aPos; // in object space
attribute vec3 aNor; // in object space

varying vec3 color; // Pass to fragment shader

void main()
{
	gl_Position = P * (MV * aPos);
	vec4 newNor = normalize(MV * vec4(aNor, 0.0));
	color = 0.5 * newNor.xyz + vec3(0.5, 0.5, 0.5);
}
