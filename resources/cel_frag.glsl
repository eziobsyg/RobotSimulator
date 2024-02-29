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
	float dotResult = dot(n, e);
	if(dotResult > -0.3 && dotResult < 0.3){
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
	else{
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

		if(result.x < 0.25) {
			result.x = 0.0;
		} else if(result.x < 0.5) {
			result.x = 0.25;
		} else if(result.x < 0.75) {
			result.x = 0.5;
		} else if(result.x < 1.0) {
			result.x = 0.75;
		} else {
			result.x = 1.0;
		}

		if(result.y < 0.25) {
			result.y = 0.0;
		} else if(result.y < 0.5) {
			result.y = 0.25;
		} else if(result.y < 0.75) {
			result.y = 0.5;
		} else if(result.y < 1.0) {
			result.y = 0.75;
		} else {
			result.y = 1.0;
		}

		if(result.z < 0.25) {
			result.z = 0.0;
		} else if(result.z < 0.5) {
			result.z = 0.25;
		} else if(result.z < 0.75) {
			result.z = 0.5;
		} else if(result.z < 1.0) {
			result.z = 0.75;
		} else {
			result.z = 1.0;
		}

		gl_FragColor = vec4(result, 1.0);
	}
}
