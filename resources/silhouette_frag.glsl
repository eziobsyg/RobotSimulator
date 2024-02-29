#version 120

varying vec4 position;
varying vec4 normal;

void main()
{
	vec3 n = normalize(normal.xyz);
	vec3 e = normalize(- position.xyz);
	float dotResult = dot(n, e);
	if(dotResult > -0.3 && dotResult < 0.3){
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
	else{
		gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
	}
}
