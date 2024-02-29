#version 120

#define MAX_LIGHTS 3

struct Light{
	vec3 position;
	vec3 color;
};

uniform Light lights[MAX_LIGHTS];
uniform int lightNum;

uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float s;

varying vec4 position;
varying vec4 normal;

void main()
{
	vec3 n = normalize(normal.xyz);
	vec3 e = normalize(- position.xyz);
	vec3 ambient = ka;
	vec3 result = vec3(0.0);

	for(int i = 0; i < lightNum; i++){
		vec3 l = normalize(lights[i].position - position.xyz);
		vec3 h = normalize(l + e);

		float diff = max(dot(n, l), 0.0);
		vec3 diffuse = kd * diff;

		float spec = pow(max(dot(h, n), 0.0), s);
		vec3 specular = ks * spec;

		vec3 tmpResult = ambient + diffuse + specular;
		result += lights[i].color * tmpResult;
	}

	gl_FragColor = vec4(result, 1.0);
}
