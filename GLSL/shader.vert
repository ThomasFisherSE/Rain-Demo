attribute vec3 inputNormal;
varying vec3 outputNormal;

varying vec2 coord; 

void main() {
	vec4 n = vec4(0,0,0,1);
	n.xyz=inputNormal;
	outputNormal=(gl_ModelViewProjectionMatrix*n).xyz;

	coord=gl_MultiTexCoord0.st;	
	gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;
}