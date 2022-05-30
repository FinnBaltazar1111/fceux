attribute vec4 a_0;
attribute vec2 a_2;

varying vec2 v_uv;
varying vec2 v_noiseUV;

void main()
{
	gl_Position = a_0;
	v_uv = a_2;
	v_noiseUV = (vec2(SCREEN_W, SCREEN_H) / vec2(NOISE_W, NOISE_H)) * a_2;
}
