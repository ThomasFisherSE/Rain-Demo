varying vec3 outputNormal;
varying vec2 coord;

uniform sampler2D tex; 

uniform bool useTextures;
uniform float alpha;

void main()
{
	vec4 col = vec4(abs(normalize(outputNormal)),1);

	if (useTextures)
	{
		col = texture2D(tex, coord) * vec4(1,1,1,alpha);
	}
	
	gl_FragColor = col;
}