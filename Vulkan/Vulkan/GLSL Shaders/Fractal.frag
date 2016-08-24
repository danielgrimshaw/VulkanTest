#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 mbrot_pos;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 color;

layout(binding = 1) uniform sampler2D texSampler;

const vec4 K = vec4(1.0, 0.66, 0.33, 3.0);

vec4 hsv_to_rgb(float hue, float saturation, float value) {
	vec4 p = abs(fract(vec4(hue) + K) * 6.0 - K.wwww);
	return vec4(value * mix(K.xxx, clamp(p.xyz - K.xxx, 0.0, 1.0), saturation), 1.0);
}

vec4 i_to_rgb(int i) {
	float hue = float(i) / 100.0;
	return hsv_to_rgb(hue, 0.5, 0.8);
}

vec2 pow_2_complex(vec2 c) {
	return vec2(c.x*c.x - c.y*c.y, 2.0*c.x*c.y); // x^2 + y^2, 2xyi
}

vec2 mandelbrot(vec2 c, vec2 c0) {
	return pow_2_complex(c) + c0;
}

vec4 iterate_pixel(vec2 position) {
	vec2 c = vec2(0);
	for (int i = 0; i < 100; i++) {
		if (c.x*c.x + c.y*c.y > 2.0*2.0) {
			return i_to_rgb(i);
		}
		c = mandelbrot(c, position);
	}
	return vec4(0, 0, 0, 1);
}

void main() {
	color = iterate_pixel(mbrot_pos.xy);
}