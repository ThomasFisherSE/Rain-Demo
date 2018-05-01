#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <stdio.h>
#include <math.h>
#include <vector>

using namespace std;

#ifdef WIN32
//#include <windows.h>
#include "Addon/glew.h"
#include "Addon/glut.h"
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif


/* In case your <GL/gl.h> does not advertise EXT_texture_lod_bias... Copyright NVidia*/
#ifndef GL_EXT_texture_lod_bias
# define GL_MAX_TEXTURE_LOD_BIAS_EXT         0x84fd
# define GL_TEXTURE_FILTER_CONTROL_EXT       0x8500
# define GL_TEXTURE_LOD_BIAS_EXT             0x8501
#endif

/* In case your <GL/gl.h> does not advertise EXT_texture_cube_map... Copyright NVidia*/
#ifndef GL_EXT_texture_cube_map
# define GL_NORMAL_MAP_EXT                   0x8511
# define GL_REFLECTION_MAP_EXT               0x8512
# define GL_TEXTURE_CUBE_MAP_EXT             0x8513
# define GL_TEXTURE_BINDING_CUBE_MAP_EXT     0x8514
# define GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT  0x8515
# define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT  0x8516
# define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT  0x8517
# define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT  0x8518
# define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT  0x8519
# define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT  0x851A
# define GL_PROXY_TEXTURE_CUBE_MAP_EXT       0x851B
# define GL_MAX_CUBE_MAP_TEXTURE_SIZE_EXT    0x851C
#endif

static GLenum faceTarget[6] = { /*Copyright NVidia*/
  GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT,
  GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT,
  GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT,
  GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT,
  GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT,
  GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT
};

// Rain type
typedef struct {
	// Life
	float life;
	float decayRate;

	float r, g, b;
	float x, y, z;
	float speed;

	float width, height;
}rainDrop;

/*---------------------------Global Variables-------------------------------*/

// Constants
const int SCREEN_WIDTH = 720, SCREEN_HEIGHT = 720; // Window size
const int MAX_RAIN = 100000; // Absolute maximum number of raindrops
const int WIND_ANGLE = 60; // Wind angle multiplier
const float FLOOR_Y = 0.03305f; // Height of the ground-plane
const float CLOUD_Y = 1.0f; // Height of the rain spawn-point
rainDrop rain[MAX_RAIN]; // Array for storing raindrops
int totalRaindrops = 10000; // Number of raindrops to render
GLuint waterTexture , groundTexture, furTexture; // Texture IDs
float rainSpeed = 0.8f; // Speed of raindrops in pixels per update
float wind = 0.2f; // Wind multiplier

GLint alpha, useTextures, tex; // Shader variable locations

int cubeMapping=0;
int lighting=1;
int *triangles=NULL;

float *vertices=NULL;
float *normals=NULL;
float *normalsPerFace=NULL;
float middle[3]={0,0,0};
float objectRadius=1;

bool drawTriangles=1;
bool drawRain = 1;
bool drawFloor = 1;
bool drawWireframe=0;
bool drawVertices=0;
bool drawNormals=0;

int cubeMapNumber=1;
int loadCubeMap=1;
int renderingType=3;
int selectedRainTex = 0; // selected rain texture to use

// Texture filepaths and sizes
char* rainTexPath[2] = { "Textures/water.bmp", "Textures/raintexture.bmp" };
const int rainTexSizeX = 256, rainTexSizeY = 256;

char* groundTexPath = "Textures/grass.bmp";
const int groundTexSizeX = 2048, groundTexSizeY = 2048;

char *fileNames[3]={"Models/bun_zipper.ply","Models/dragon.ply","Models/happy.ply"};
int fileProperties[3*2]={35947,69451,437645,871414,543652,1087716};

char *vertexProgramNames[1]={"GLSL/shader.vert"};
char *fragmentProgramNames[1]={"GLSL/shader.frag"};
GLuint vShader=-1, fShader=-1, program=-1;

int model=0;
float zoom=1.5f;
int normal_smoothing_interation=0;

GLuint texName;
int mouseX;
int mouseY;
float orientationMatrix[16];

//  Useful Functions
void InverseMatrix(float m[16])
{
	float m00 = m[0];  
	float m01 = m[1];  
	float m02 = m[2];  
	float m03 = m[3];
	float m10 = m[4];  
	float m11 = m[5];  
	float m12 = m[6];  
	float m13 = m[7];
	float m20 = m[8];  
	float m21 = m[9];  
	float m22 = m[10];  
	float m23 = m[11];
	float m30 = m[12];  
	float m31 = m[13];  
	float m32 = m[14];  
	float m33 = m[15];

	float d00 = m11*m22*m33 + m12*m23*m31 + m13*m21*m32 - m31*m22*m13 - m32*m23*m11 - m33*m21*m12;
	float d01 = m10*m22*m33 + m12*m23*m30 + m13*m20*m32 - m30*m22*m13 - m32*m23*m10 - m33*m20*m12;
	float d02 = m10*m21*m33 + m11*m23*m30 + m13*m20*m31 - m30*m21*m13 - m31*m23*m10 - m33*m20*m11;
	float d03 = m10*m21*m32 + m11*m22*m30 + m12*m20*m31 - m30*m21*m12 - m31*m22*m10 - m32*m20*m11;

	float d10 = m01*m22*m33 + m02*m23*m31 + m03*m21*m32 - m31*m22*m03 - m32*m23*m01 - m33*m21*m02;
	float d11 = m00*m22*m33 + m02*m23*m30 + m03*m20*m32 - m30*m22*m03 - m32*m23*m00 - m33*m20*m02;
	float d12 = m00*m21*m33 + m01*m23*m30 + m03*m20*m31 - m30*m21*m03 - m31*m23*m00 - m33*m20*m01;
	float d13 = m00*m21*m32 + m01*m22*m30 + m02*m20*m31 - m30*m21*m02 - m31*m22*m00 - m32*m20*m01;

	float d20 = m01*m12*m33 + m02*m13*m31 + m03*m11*m32 - m31*m12*m03 - m32*m13*m01 - m33*m11*m02;
	float d21 = m00*m12*m33 + m02*m13*m30 + m03*m10*m32 - m30*m12*m03 - m32*m13*m00 - m33*m10*m02;
	float d22 = m00*m11*m33 + m01*m13*m30 + m03*m10*m31 - m30*m11*m03 - m31*m13*m00 - m33*m10*m01;
	float d23 = m00*m11*m32 + m01*m12*m30 + m02*m10*m31 - m30*m11*m02 - m31*m12*m00 - m32*m10*m01;

	float d30 = m01*m12*m23 + m02*m13*m21 + m03*m11*m22 - m21*m12*m03 - m22*m13*m01 - m23*m11*m02;
	float d31 = m00*m12*m23 + m02*m13*m20 + m03*m10*m22 - m20*m12*m03 - m22*m13*m00 - m23*m10*m02;
	float d32 = m00*m11*m23 + m01*m13*m20 + m03*m10*m21 - m20*m11*m03 - m21*m13*m00 - m23*m10*m01;
	float d33 = m00*m11*m22 + m01*m12*m20 + m02*m10*m21 - m20*m11*m02 - m21*m12*m00 - m22*m10*m01;

	float D = m00*d00 - m01*d01 + m02*d02 - m03*d03;
	if (D<0) D=-D;

	if ( D>0.0000001 )
	{
		D=1.f/D;
		m[0] =  d00*D; 
		m[1] = -d10*D;  
		m[2] =  d20*D; 
		m[3] = -d30*D;
		m[4] = -d01*D; 
		m[5] =  d11*D;  
		m[6] = -d21*D; 
		m[7] =  d31*D;
		m[8] =  d02*D; 
		m[9] = -d12*D;  
		m[10] =  d22*D; 
		m[11] = -d32*D;
		m[12] = -d03*D; 
		m[13] =  d13*D;  
		m[14] = -d23*D; 
		m[15] =  d33*D;
	}
	else
	{
	}
}

void BMPReader(char *name, unsigned char **image, int &sizeX, int &sizeY)
{
	int i;
	FILE *f;
	int hdr[0x36];
	
	if (*image!=NULL)
	{
		delete[] *image;
		*image=NULL;
	}

	f=fopen(name,"rb");
	if (f==NULL) 
	{
		return;
	}
	fread(hdr,1,0x12,f);
	fread(&sizeX,1,4,f);
	fread(&sizeY,1,4,f);
	fread(hdr,1,0x1C,f);
	*image=new unsigned char [3*sizeX*sizeY];
	fread(*image,1,sizeX*sizeY*3,f);
	for (i=0;i<sizeX*sizeY;i++)
	{
		static unsigned char aux;
		aux=(*image)[3*i];
		(*image)[3*i]=(*image)[3*i+2];
		(*image)[3*i+2]=aux;
	}
	fclose(f);	
}

GLuint LoadTexture(char* tex_name, int width, int height)
{
	GLuint texture;

	unsigned char* imageData;
	imageData = (unsigned char*)malloc(width*height * 3);

	BMPReader(tex_name, &imageData, width, height);

	if (!imageData)
	{
		printf("Texture %s not loaded correctly.\n", tex_name);
	}

	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);

	free(imageData);

	return texture;
}

void PrintLog(GLuint obj)
{
	int infologLength = 0;
	int maxLength;
	
	if(glIsShader(obj))
		glGetShaderiv(obj,GL_INFO_LOG_LENGTH,&maxLength);
	else
		glGetProgramiv(obj,GL_INFO_LOG_LENGTH,&maxLength);
			
	char *infoLog=new char[maxLength];
 
	if (glIsShader(obj))
		glGetShaderInfoLog(obj, maxLength, &infologLength, infoLog);
	else
		glGetProgramInfoLog(obj, maxLength, &infologLength, infoLog);
 
	if (infologLength > 0)
		printf("%s\n",infoLog);
	delete[] infoLog;
}

void CrossProduct(float *n1, float *n2, float *n /*result*/)
{
			n[0]=n1[1]*n2[2]-n1[2]*n2[1];
			n[1]=n1[2]*n2[0]-n1[0]*n2[2];
			n[2]=n1[0]*n2[1]-n1[1]*n2[0];
}

//  GLSL Functions

char *ReadProgram (char *programName)
{
	FILE *file;
	char *program=NULL;
	long l,l2;

	file=fopen(programName,"r");
	if (file==NULL) {
		printf("Cannot open file\n");
		return NULL;
	}
	fseek(file, 0, SEEK_END);
	l = ftell(file);
	program=new char[l+1];
	if (l==NULL) {
		printf("Cannot allocate memory file\n");
		return NULL;
	}
	fseek(file, 0, SEEK_SET);
	l2=fread(program,sizeof(char),l,file);
	program[l2]=0;
	return program;
}

void PrepareShaderVariables()
{
	alpha = glGetUniformLocation(program, "alpha"); // Alpha level (for transparency)
	useTextures = glGetUniformLocation(program, "useTextures"); // Flag for ignoring / using textures
	tex = glGetUniformLocation(program, "tex"); // Texture to use

	// Default values
	glUniform1f(alpha, 1.0f);
	glUniform1i(tex, 0);
}

void LoadShaders()
{
	const char *myVertexProgram;
	const char *myFragmentProgram;

	myVertexProgram = ReadProgram(vertexProgramNames[0]);
	myFragmentProgram = ReadProgram(fragmentProgramNames[0]);
	fShader = glCreateShader(GL_FRAGMENT_SHADER);
	vShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vShader, 1, &myVertexProgram, NULL);
	glShaderSource(fShader, 1, &myFragmentProgram, NULL);
	
	printf("\n");
	glCompileShader(vShader);
	printf("---Vertex Log:---\n\n");
	PrintLog(vShader);
	printf("-----------------\n\n");

	printf("\n");
	glCompileShader(fShader);
	printf("---Fragment Log:---\n\n");
	PrintLog(fShader);
	printf("-------------------\n\n\n");

	delete[] myVertexProgram;
	delete[] myFragmentProgram;
	program = glCreateProgram(); //Creates a program object
	glAttachShader(program, vShader);
	glAttachShader(program, fShader);
	glLinkProgram(program);
	glUseProgram(program);
	PrintLog(program);

	PrepareShaderVariables();
}

void InitMatrix(float *m)
{
	int i;
	for (i=0;i<16;i++)
		m[i]=0;
	m[0]=m[5]=m[10]=m[15]=1.;
}

void SetupTextureMatrix()
{
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	InverseMatrix(orientationMatrix);
	glMultMatrixf(orientationMatrix);
	glMultMatrixf(orientationMatrix);
//	glScalef(1,-1,-1);
	glTranslatef(-middle[0],-middle[1],-middle[2]);//Center the object!
	InverseMatrix(orientationMatrix);
}

void ResetTextureMatrix()
{
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
}

void InitialCameraSetup()
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(20, 1, 0, 0);
	glMultMatrixf(orientationMatrix);
	glGetFloatv(GL_MODELVIEW_MATRIX, orientationMatrix);

	glClearColor(0.6f, 0.85f, 1.0f, 0); // Set the color of the 'sky' (background)
}

void SetupModelview()
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf(orientationMatrix);
	glTranslatef(-middle[0],-middle[1],-middle[2]);//Center the object!
}

void SetupProjection()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-objectRadius*zoom,objectRadius*zoom,-objectRadius*zoom,objectRadius*zoom,-objectRadius,objectRadius);
}

void SetupLighting()
{
	float ambiant[4]= {0.2f,0.2f,0.2f,1.};
	float diffuse[4]= {0.7f,0.7f,0.7f,1.};
	float specular[4]= {1,1,1,1.};
	float exponent=4;
	float lightDir[4] = {0,0,1,0};

	glEnable (GL_COLOR);
	glEnable (GL_COLOR_MATERIAL);
	glEnable(GL_FRONT_AND_BACK);
	if (lighting)
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);
	glLightfv(GL_LIGHT0,GL_AMBIENT,ambiant);
	glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuse);
	glLightfv(GL_LIGHT0,GL_SPECULAR,specular);
	glLightf(GL_LIGHT0,GL_SPOT_EXPONENT,exponent);
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, lightDir);
	glEnable(GL_LIGHT0);
}

void DrawWireframe(int nbTriangles)
{
	glBegin(GL_LINES);
	glColor3f(0,0,1);
	for (int i=0;i<nbTriangles;i++) //For all triangles
	{
		int v0=3*triangles[3*i];
		int v1=3*triangles[3*i+1];
		int v2=3*triangles[3*i+2];
		//1st edge
		glVertex3fv(vertices+v0); 
		glVertex3fv(vertices+v1);
		//2nd edge
		glVertex3fv(vertices+v0);
		glVertex3fv(vertices+v2);
		//3rd edge
		glVertex3fv(vertices+v1);
		glVertex3fv(vertices+v2);
	}
	glEnd();
}

void DrawVertices(int nbOfVertices)
{
	glDisable(GL_LIGHTING);	
	glBegin(GL_POINTS);
	glColor3f(1.f, 1.f, 0.f);
	for (int i = 0; i < nbOfVertices; i++) 
		glVertex3fv(&vertices[3*i]);
	glEnd();
}

void DrawTriangles(int nbTriangles) 
{
	glDisable(GL_LIGHTING);
	glBegin(GL_TRIANGLES);
	glColor3f(1,0,0);
	for (int i=0;i<nbTriangles;i++)
	{
		int *vert=triangles+3*i;
		float *pt1=vertices+3*vert[0];
		float *pt2=vertices+3*vert[1];
		float *pt3=vertices+3*vert[2];
		glVertex3fv(pt1);
		glVertex3fv(pt2);
		glVertex3fv(pt3);
	}
	glEnd();
}

void InitialiseTextures()
{
	glEnable(GL_DEPTH_TEST);

	// Enable alpha blending for transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Blending function to use

	// Enable textures
	glEnable(GL_TEXTURE_2D);
	
	// Load textures
	waterTexture = LoadTexture(rainTexPath[selectedRainTex], rainTexSizeX, rainTexSizeY);
	groundTexture = LoadTexture(groundTexPath, groundTexSizeX, groundTexSizeY);
}

void initialiseRaindrop(int i) {
	rain[i].life = 1.0f; // Life of the randrop which decays over time
	
	// Decay rate, used to create stuttered spawn times 
	// (prevents a lot of rain falling and then all re-spawning all at once)
	// The every time the raindrop's position is updated, it decays a small amount
	rain[i].decayRate = float(rand() % 100) / 2000.0f; // Random rate (0 to 0.05)

	/* 
	 * Threshold the decay rate so that slowly decaying raindrops no longer decay at all
	 * Note: I added this so that rain closer to the ground doesn't just disappear all of a sudden.
	 * With this thresholding, raindrops that fade away before hitting the ground will not be seen 
	 * near the ground.
	 */
	if (rain[i].decayRate < 0.015)
	{
		rain[i].decayRate = 0;
	}

	// Coordinates of raindrop centre
	rain[i].x = ((float)rand() / (RAND_MAX)) * 2 - 1; // Random location (-1 to +1)
	rain[i].y = CLOUD_Y; // Start rain at the top of the screen
	rain[i].z = ((float)rand() / (RAND_MAX)) * 2 - 1; // Random location (-1 to +1)

	// Colour (To be used if shaders are disabled)
	rain[i].r = 0.0f; // red channel
	rain[i].g = 0.3f; // green channel
	rain[i].b = 1.0f; // blue channel

	// Size of raindrop
	rain[i].width = 0.002f; 
	rain[i].height = 0.01f;

	// Falling speed of raindrop
	rain[i].speed = rainSpeed;
}

void DrawRain()
{
	glUniform1f(alpha, 0.4f); // Set rain to be semi-transparent

	// Tell shaders to use textures to determine fragment colour
	glUniform1i(useTextures, true); 

	// Bind the texture to use
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTexture);
	glUniform1i(tex, 0);

	float x, y, z; // Position
	float sizeX, sizeY; // Size

 	for (int i = 0; i < totalRaindrops; i++) {
		// Initialise size and position
		x = rain[i].x;
		y = rain[i].y;
		z = rain[i].z;

		sizeX = rain[i].width;
		sizeY = rain[i].height;

		glColor4f(rain[i].r, rain[i].g, rain[i].b, 0.4f); // Default colour to use if shaders disabled / fail

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef(x, y, z); // Centre rotation about raindrop anchor-point
		glRotatef(wind * WIND_ANGLE, 0, 0, 1); // Rotate raindrop about z-axis based on the wind

		glBegin(GL_QUADS);

		// Front-on view
		glTexCoord2f(0, 0);
		glVertex3f(x-sizeX/2, y-sizeY/2, z); // lower left

		glTexCoord2f(0, 1);
		glVertex3f(x-sizeX/2, y+sizeY/2, z); // upper left

		glTexCoord2f(1, 1);
		glVertex3f(x+sizeX/2, y+sizeY/2, z); // upper right

		glTexCoord2f(1, 0);
		glVertex3f(x+sizeX/2, y-sizeY/2, z); // lower right

		// Side-on view
		glTexCoord2f(0, 0);
		glVertex3f(x, y - sizeY / 2, z-sizeX/2); // lower left

		glTexCoord2f(0, 1);
		glVertex3f(x, y + sizeY / 2, z-sizeX/2); // upper left

		glTexCoord2f(1, 1);
		glVertex3f(x, y + sizeY / 2, z+sizeX/2); // upper right

		glTexCoord2f(1, 0);
		glVertex3f(x, y - sizeY / 2, z+sizeX/2); // lower right 

		glEnd();

		glPopMatrix();

		// Make the raindrop fall
		if (rain[i].y >= FLOOR_Y)
		{
			// Move down the screen based on the speed
			rain[i].y -= sizeY*rain[i].speed;

			// Move across the screen by a factor of the wind (which is constant across the scene)
			rain[i].x += (wind*rain[i].speed) / 100; 
		}
		else {
			// If the rain has passed the ground plane, destroy it
			rain[i].life = 0;
		}
			
		// Decay the life of the raindrop
		rain[i].life -= rain[i].decayRate;

		// If the raindrop was destroyed, create a new raindrop at the top of the screen
		if (rain[i].life <= 0.0) {
			initialiseRaindrop(i);
		}

		glutPostRedisplay();
	}
}

void DrawGroundPlane()
{
	glUniform1f(alpha, 1.0f); // Tell shader to colour the ground opaque
	glUniform1i(useTextures, true); // Tell the shader to use texture for the ground

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, groundTexture); // Bind the ground texture

	glBegin(GL_QUADS);
	glColor3f(0., 0.7f, 0.1f); // Default texture if shaders / texturing fail

	glTexCoord2f(0, 0);
	glVertex3f(-1.0, FLOOR_Y, -1.0); // Front Left

	glTexCoord2f(0, 1);
	glVertex3f(-1.0, FLOOR_Y, 1.0); // Back Left

	glTexCoord2f(1, 1);
	glVertex3f(1.0, FLOOR_Y, 1.0); // Back Right

	glTexCoord2f(1, 0);
	glVertex3f(1.0, FLOOR_Y, -1.0); // Front Right
	glEnd();
}

void DrawTrianglesQ2(int nbTriangles) 
{
	glEnable(GL_LIGHTING);

	glBegin(GL_TRIANGLES);
	glColor3f(1.0,1.0,1.0);
	for (int i=0;i<nbTriangles;i++)
	{
		int *vert=triangles+3*i;
		float *pt1=vertices+3*vert[0];
		float *pt2=vertices+3*vert[1];
		float *pt3=vertices+3*vert[2];
		glNormal3fv(normalsPerFace+3*i); //1 normal for all three vertices: Normal only expressed once before passing the vertices
		glVertex3fv(pt1);
		glVertex3fv(pt2);
		glVertex3fv(pt3);
	}
	glEnd();
}

void DrawTrianglesQ3(int nbTriangles) 
{
	glBegin(GL_TRIANGLES);
	glColor3f(1.0,1.0,1.0);
	for (int i=0;i<nbTriangles;i++)
	{
		int *vert=triangles+3*i;
		float *pt1=vertices+3*vert[0];
		float *pt2=vertices+3*vert[1];
		float *pt3=vertices+3*vert[2];
		float *n1=normals+3*vert[0];
		float *n2=normals+3*vert[1];
		float *n3=normals+3*vert[2];
		glNormal3fv(n1); //first normal's coordinates for the first vertex.
		glVertex3fv(pt1);
		glNormal3fv(n2);
		glVertex3fv(pt2);
		glNormal3fv(n3);
		glVertex3fv(pt3);
	}
	glEnd();
}

void DrawRabbit(int nbTriangles)
{
	PrintLog(program);
	int inputNormal = glGetAttribLocation(program, "inputNormal"); // Pass the input normal to the shaders

	glUniform1i(useTextures, false); // Tell shaders not to use textures

	glBegin(GL_TRIANGLES);
	glColor3f(1, 1, 1); // Default colour if shaders and textures fail

	for (int i = 0; i<nbTriangles; i++)
	{
		int *vert = triangles + 3 * i;
		float *pt1 = vertices + 3 * vert[0];
		float *pt2 = vertices + 3 * vert[1];
		float *pt3 = vertices + 3 * vert[2];
		float *n1 = normals + 3 * vert[0];
		float *n2 = normals + 3 * vert[1];
		float *n3 = normals + 3 * vert[2];

		glVertexAttrib3fv(inputNormal, n1);
		glVertex3fv(pt1);

		glVertexAttrib3fv(inputNormal, n2);
		glVertex3fv(pt2);
		
		glVertexAttrib3fv(inputNormal, n3);
		glVertex3fv(pt3);
	}
	glEnd();
}

void DrawNormals(int nbVertices)
{
	glDisable(GL_LIGHTING);	
	glBegin(GL_LINES);
	glColor3f(1.f, 0 , 1.f);
	for (int i = 0; i < nbVertices; i++) 
	{
		glColor3f(1.f, 0 , 0.f);
		glVertex3fv(&vertices[3*i]);
		glColor3f(0.4f, 0 , 0.6f);
		glVertex3f(
			vertices[3*i]+normals[3*i]*objectRadius*0.05,
			vertices[3*i+1]+normals[3*i+1]*objectRadius*0.05,
			vertices[3*i+2]+normals[3*i+2]*objectRadius*0.05
			);
	}
	glEnd();
}

void ReadFile(char *name,int nbOfVertices, int nbOfTriangles)
{
	FILE *f; //Using stdio but other libraries available as well
	int i,j;
	float aux1, aux2;
	int polygonNb;
	float min[3]={99999999.0f,99999999.0f,99999999.0f};
	float max[3]={-99999999.0f,-99999999.0f,-99999999.0f};
	if (triangles!=NULL)
		delete[] triangles; //new and delete operators are specific to c++!
	if (vertices!=NULL)
		delete[] vertices;//Delete previously allocated structures if needed!
	triangles=new int[nbOfTriangles*3];//Allocate triangles
	vertices=new float[nbOfVertices*3];

	f=fopen(name,"r");
	if (f==NULL) //File not found
	{
		printf("File not found\n");
		return;
	}
	if (model==0)
		for (i=0;i<nbOfVertices;i++)
			fscanf(f,"%f %f %f %f %f\t",vertices+3*i,vertices+3*i+1,vertices+3*i+2,&aux1,&aux2);
	else 
		for (i=0;i<nbOfVertices;i++)
			fscanf(f,"%f %f %f\t",vertices+3*i,vertices+3*i+1,vertices+3*i+2);
	for (i=0;i<nbOfTriangles;i++)
		fscanf(f,"%d %d %d %d\t",&polygonNb,triangles+3*i,triangles+3*i+1,triangles+3*i+2);
	fclose(f);
	//Here Compute middle point and radius by computin min max values of our object on x y and z
	//Compute middle point and radius now
	for (i=0;i<nbOfVertices;i++)
		for (j=0;j<3;j++) //Loop on each axis
		{
			if (min[j]>vertices[3*i+j])
				min[j]=vertices[3*i+j];
			if (max[j]<vertices[3*i+j])
				max[j]=vertices[3*i+j];
		}

	for (j=0;j<3;j++) //Loop on each axis
		middle[j]=(min[j]+max[j])/2.;
	objectRadius=sqrt(	(min[0]-middle[0])*(min[0]-middle[0])+
					(min[1]-middle[1])*(min[1]-middle[1])+
(min[2] - middle[2])*(min[2] - middle[2]));
printf("Approximate Radius:%f\n", objectRadius);
}

void ComputeNormals(int nbVertices, int nbTriangles)
{
	if (normals != NULL)
		delete[] normals; //new and delete operators are specific to c++!
	if (normalsPerFace != NULL)
		delete[] normalsPerFace; //new and delete operators are specific to c++!
	normalsPerFace = new float[3 * nbTriangles];
	normals = new float[3 * nbVertices];
	for (int i = 0; i < nbVertices; i++)
		normals[3 * i] = normals[3 * i + 1] = normals[3 * i + 2] = 0; //initializing normals' sum to 0
	for (int i = 0; i < nbTriangles; i++)
	{
		float v1[3], v2[3], n[3];
		int *vert = triangles + 3 * i;
		float *pt1 = vertices + 3 * vert[0];
		float *pt2 = vertices + 3 * vert[1];
		float *pt3 = vertices + 3 * vert[2];
		for (int j = 0; j < 3; j++)
		{
			v1[j] = pt1[j] - pt2[j];
			v2[j] = pt1[j] - pt3[j];
		}
		CrossProduct(v1, v2, n);
		float l = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
		if (l > 0)
		{
			l = 1. / l;
			n[0] *= l;
			n[1] *= l;
			n[2] *= l;
			for (int j = 0; j < 3; j++)
			{
				normalsPerFace[3 * i + j] = n[j];
				normals[3 * triangles[3 * i] + j] += n[j];//Add normal for the first vertex
				normals[3 * triangles[3 * i + 1] + j] += n[j];//Add normal for the second vertex
				normals[3 * triangles[3 * i + 2] + j] += n[j];//Add normal for the third vertex
			}
		}
	}
	for (int i = 0; i < nbVertices; i++) //Normalizes Normals ||n|| = 1
	{
		float l = sqrt(normals[3 * i] * normals[3 * i] + normals[3 * i + 1] * normals[3 * i + 1] + normals[3 * i + 2] * normals[3 * i + 2]);
		if (l > 0)
		{
			l = 1. / l;
			normals[3 * i] *= l;
			normals[3 * i + 1] *= l;
			normals[3 * i + 2] *= l;
		}
	}
}

/*---------------------------Glut Callback functions---------------------------------*/

void Idlefunc()
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(4, 0, 1, 0);
	glMultMatrixf(orientationMatrix);
	glGetFloatv(GL_MODELVIEW_MATRIX, orientationMatrix);
	glutPostRedisplay();
}

void Key(unsigned char c, int mousex, int mousey)
{
	static bool automatic_rotation = 0;
	switch (c)
	{
		case 27://Escape
			glFinish();
			glutPostRedisplay();
			exit(0);
			break;
		case 13://Return
			glutPostRedisplay();
			fflush(stdin);
			break;
		case '+':
			/*
			normal_smoothing_interation++;
			ComputeNormals(fileProperties[2*model],fileProperties[2*model+1]);
			glutPostRedisplay ();
			*/
			rainSpeed *= 1.1f;
			printf("Rain Speed Multiplier: %f\n", rainSpeed);
			break;
		case '-':
			/*
			normal_smoothing_interation--;
			ComputeNormals(fileProperties[2*model],fileProperties[2*model+1]);
			glutPostRedisplay ();
			*/
			rainSpeed *= 0.9f;
			printf("Rain Speed Multiplier: %f\n", rainSpeed);
			break;
		case '>':
			if ((totalRaindrops*1.05f) > MAX_RAIN)
			{
				totalRaindrops = MAX_RAIN;
			}
			else {
				totalRaindrops *= 1.05f;
			}

			printf("Number of raindrops being rendered: %d\n", totalRaindrops);
			break;
		case '<':
			if ((totalRaindrops*0.95f) <= 1.)
			{
				totalRaindrops = 1.;
			}
			else {
				totalRaindrops *= 0.95f;
			}

			printf("Number of raindrops being rendered: %d\n", totalRaindrops);

			break;
		case ')':
			wind *= 1.05f;
			printf("Wind: %f\n", wind);
			break;
		case '(':
			wind *= 0.95f;
			printf("Wind: %f\n", wind);
			break;
		case '*' :
			zoom*=0.8f;
			glutPostRedisplay ();
			break;
	    case '/' :
			zoom*=1.25f;
			glutPostRedisplay ();
			break;
	    case 'c' :
			cubeMapping=!cubeMapping;		
			glutPostRedisplay ();
			break;
	    case 'l' :
			lighting=!lighting;
			glutPostRedisplay ();
			break;
	    case 'm' :
			cubeMapNumber=(cubeMapNumber+1)%4;
			loadCubeMap=1;
			glutPostRedisplay ();
			break;
	    case 'w' :
			drawWireframe=!drawWireframe;
			glutPostRedisplay ();
			break;
	    case 'v' :
			drawVertices=!drawVertices;
			glutPostRedisplay ();
	    case 'n' :
			drawNormals=!drawNormals;
			glutPostRedisplay ();
			break;
	    case 't' :
			drawTriangles=!drawTriangles;
			glutPostRedisplay ();
			break;
	    case 'r' :
			renderingType++;
			if (renderingType>3)
				renderingType=0;
			glutPostRedisplay();
			break;
	    case 'g' :
			glClearColor(0.0f,0.1f,0.0f,0.0f);
			glutPostRedisplay();
			break;
	    case 'b' :
			glClearColor(0.4f,1.0f,1.0f,0.0f);
			glutPostRedisplay();
			break;
		case '1':
			selectedRainTex = 0;
			waterTexture = LoadTexture(rainTexPath[selectedRainTex], rainTexSizeX, rainTexSizeY);
			glutPostRedisplay();
			break;
		case '2':
			selectedRainTex = 1;
			waterTexture = LoadTexture(rainTexPath[selectedRainTex], rainTexSizeX, rainTexSizeY);
			glutPostRedisplay();
			break;
	    case '0' :
			glClearColor(0,0,0,0);
			glutPostRedisplay();
			break;
		case 'p' :
			drawRain = !drawRain;
			glutPostRedisplay();
		case ' ' :
			automatic_rotation=!automatic_rotation;
			if (automatic_rotation)
				glutIdleFunc(Idlefunc);
			else
				glutIdleFunc(NULL);
			break;
		default:
			break;
	}
}

void Arrow(int key, int mousex, int mousey)
{
    switch (key)
	{
	    case GLUT_KEY_LEFT:
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glRotatef(-4,0,1,0);
			glMultMatrixf(orientationMatrix);
			glGetFloatv(GL_MODELVIEW_MATRIX,orientationMatrix);
			glutPostRedisplay();
		break;
	    case GLUT_KEY_RIGHT:
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glRotatef(4,0,1,0);
			glMultMatrixf(orientationMatrix);
			glGetFloatv(GL_MODELVIEW_MATRIX,orientationMatrix);
			glutPostRedisplay();
		break;
	    case GLUT_KEY_UP:
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glRotatef(-4,1,0,0);
			glMultMatrixf(orientationMatrix);
			glGetFloatv(GL_MODELVIEW_MATRIX,orientationMatrix);
			glutPostRedisplay();
		break;
	    case GLUT_KEY_DOWN:
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glRotatef(4,1,0,0);
			glMultMatrixf(orientationMatrix);
			glGetFloatv(GL_MODELVIEW_MATRIX,orientationMatrix);
			glutPostRedisplay();
		break;
	    default :
		break;
	}
}

void mouse(int button, int state, int x, int y)
{
	//Record the mouse location when a button is pushed down
	mouseX=x;
	mouseY=y;	
}

void motion(int x, int y)
{
	//Perform a rotation. The new orientation matrix should be  m_orientation=m_orientation*m_mouse_interaction
	//Where m_mouse_interaction is representative of the mouse interactions in the image plane
	glMatrixMode(GL_MODELVIEW);//Use OpenGL capabilites to multiply matrices. The modelview Matrix is used here
	glLoadIdentity();
	glRotatef(sqrt((float) ((x-mouseX)*(x-mouseX)+(y-mouseY)*(y-mouseY))),(y-mouseY),(x-mouseX),0); //Rotation around an inclined axis
	glMultMatrixf(orientationMatrix);
	glGetFloatv(GL_MODELVIEW_MATRIX,orientationMatrix); //Get the multiplied matrix
	mouseX=x;	//Reinitialize the new position	
	mouseY=y;		
	glutPostRedisplay();// Generates a refresh event (which will call draw)
}

void Draw()
{
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	SetupLighting();
	SetupModelview();
	SetupProjection();
	if (drawTriangles)
	{
		switch (renderingType)
		{
			case 0:	
				DrawTriangles(fileProperties[2*model+1]);
				break;
			case 1:	
				DrawTrianglesQ2(fileProperties[2*model+1]);
				break;
			case 2:	
				DrawTrianglesQ3(fileProperties[2*model+1]);
				break;
			case 3:	
				DrawRabbit(fileProperties[2*model+1]);
				break;
		}
	}
	if (drawWireframe)
		DrawWireframe(fileProperties[2*model+1]);
	if (drawVertices)
		DrawVertices(fileProperties[2*model]);
	if (drawNormals)
		DrawNormals(fileProperties[2*model]);

	if (drawFloor)
		DrawGroundPlane();

	if (drawRain)
		DrawRain();

	glutSwapBuffers();
}

void Init_GL(int sizex, int sizey)
{
    glutInitWindowPosition(0, 0);//ToDo:change it to 0
    glutInitWindowSize(sizex, sizey);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutCreateWindow("Rain Demo");
	//glutIdleFunc(Idlefunc); //Setup a function that is called after every rendering
	glutDisplayFunc(Draw);
    glutKeyboardFunc(Key);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
    glutSpecialFunc(Arrow);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	//glGenTextures(1, &texName); //Creates a texture name for the mirror...
}

void PrintFeatureList()
{
	printf("\n\n-------------------------------------- Important Features --------------------------------------\n");
	printf(" > This program demonstrates a simulation of rain falling onto a rabbit and the ground.\n");
	printf(" > Drops are randomly located within the scene, and their speed is constant.\n");
	printf(" > The number of raindrops rendered can be increased / decreased with '>' or '<'.\n");
	printf(" > The speed of the raindrops can be increased / decreased with '+' or '-'.\n");
	printf(" > Raindrops fall at an angle depending on the wind. This angles both the raindrops aswell as their motion.\n");
	printf(" > The wind level can be increased / decreased with ')' or '('.\n");
	printf(" > A ground plane is drawn under the rabbit model. Raindrops dissapear after hitting the ground.\n");
	printf(" > Raindrops are made semi-transparent using alpha-blending so that they do not look like quads.\n");
}

void PrintKeys()
{
	printf("\n\n-------------------------------------- Interaction Keys --------------------------------------\n");
	printf("l: Toggles lighting model on and off.\n");
	printf("r: Switches between various rendering modes (uniform, flat and smooth).\n");
	printf("t,v,n: Toggles triangles, vertices and normal visualisations.\n");
	printf("*,/: Zoom in and out.\n");
	printf("Spacebar: Toggles automatic rotation.\n");
	printf("+,-: Speeds up or slows down rainfall by 10%%.\n");
	printf(">,<: Increase or decrease the total number of raindrops being rendered by 5%%.\n");
	printf("),(: Increase or decrease the wind level by 5%%.\n");
	printf("0, 1: Change raindrop texture between water.bmp and raintexture.bmp respectively.\n");
	printf("See the Key function for more options.\n\n");
}


int main(int argc, char **argv)
{
	ReadFile(fileNames[model],fileProperties[2*model],fileProperties[2*model+1]);
	ComputeNormals(fileProperties[2*model],fileProperties[2*model+1]);
	InitMatrix(orientationMatrix);

	
	Init_GL(SCREEN_WIDTH, SCREEN_HEIGHT);
	
	printf("%s %s\n",(char *) glGetString(GL_VERSION),(char *) glGetString(GL_EXTENSIONS));
	printf("Initializing glew\n");
	GLenum err =glewInit();
	
	if (err==GLEW_OK)
		fprintf(stderr, "INFO: OpenGL 2.0 supported, proceeding\n");
	else
	{
		fprintf(stderr, "INFO: OpenGL 2.0 not supported. Exit\n");
		return EXIT_FAILURE;
	}
	
	LoadShaders();
	InitialiseTextures();

	// Initialize raindrops
	for (int i = 0; i < MAX_RAIN; i++) {
		initialiseRaindrop(i);
	}

	InitialCameraSetup();

	PrintFeatureList();
	PrintKeys();

	glutMainLoop();
}



