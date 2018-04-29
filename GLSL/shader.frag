varying vec3 outputNormal;
varying vec2 coord;

uniform sampler2D tex; 

uniform bool useTextures;
uniform float alpha;

void main()
{
	// gl_FragColor.xyz=vec3(1,0,0); 
	// gl_FragColor.xyz=abs(normalize(outputNormal)); 
	// gl_FragColor.xyz=abs(dot(gl_LightSource[0].diffuse.xyx,normalize(outputNormal)));
	//transparency = 1.0;

	vec4 col = vec4(abs(normalize(outputNormal)),1);
	//col.xyz = outputNormal;

	if (useTextures)
	{
		col = texture2D(tex, coord) * vec4(1,1,1,alpha);
	}
	
	gl_FragColor = col;
}