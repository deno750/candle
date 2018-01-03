
#include "common.frag"
#line 4

layout (location = 0) out vec4 FragColor;
uniform pass_t transp;

void main()
{
	vec4 color = pass_sample(transp, texcoord);

	/* vec4 color = vec4(0.0f); */
	vec4 refl = textureLod(gbuffer.reflection, texcoord, 0);
	vec4 ssred = ssr(transp.texture);

	/* vec3 final = mix(color.rgb, ssred.rgb, ssred.a); */
	vec3 final = (color.rgb) + ssred.rgb * ssred.a * refl.rgb * refl.a;

	/* FragColor = vec4(ssred.rgb, 1.0f); return; */
	/* FragColor = vec4(color.rgb, 1.0f); return; */
	/* FragColor = vec4(textureLod(gbuffer.transparency, texcoord, 0).rgb, 1.0f); return; */

	/* final = clamp(final * 1.6f - 0.10f, 0.0, 3.0); */
	final = final * pow(2.0f, exposure);

	FragColor = vec4(final, 1.0f);
}

// vim: set ft=c:
