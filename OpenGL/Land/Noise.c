#define GL_GLEXT_PROTOTYPES

//g++ main.c -lglut -lGL -lGLEW
//glxinfo | grep "version"

#include <GL/glew.h>
#include <GL/freeglut.h>
#include "../SOIL.h"

#include <stdio.h>
#include <vector>
#include <iostream>
#include <cstring>
#include <cmath>

using namespace std;

const int WIDTH = 640;
const int HEIGHT = 480;

static const char* vertex_source = 
    "   #version 130 \n" 

    "   uniform vec4 u_Translation; \n"
    " 	uniform mat4 u_ViewMatrix; \n"
  	"	uniform mat4 u_ProjMatrix; \n"
  	"	uniform mat4 u_ModelMatrix; \n"

    "   in vec4 a_Position; \n" 
    "   in vec4 a_Color; \n"
    "   in vec2 a_TexCoord; \n"
    "   out vec4 v_Color; \n"

    "   varying vec2 v_TexCoord;\n"

    "   void main() { \n" 
    //"		gl_Position = u_ProjMatrix * u_ViewMatrix * u_ModelMatrix * a_Position; \n"
    "       gl_Position = a_Position * u_ModelMatrix * u_ViewMatrix * u_ProjMatrix;  \n" 
    //"       gl_Position = a_Position + u_Translation;  \n"
    "       v_Color = a_Color; \n"
    "       v_TexCoord = a_TexCoord;\n"
    "   } \n";

//http://www.science-and-fiction.org/rendering/noise.html

static const char* fragment_source =
    "   #version 130 \n"
    "   in vec4 v_Color; \n"

    "	uniform sampler2D u_Sampler;\n"
    "	varying vec2 v_TexCoord;\n"

    "   float random(in vec2 st){ \n"
    "		return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123); \n"
    "	} \n"


    "   float noise(in vec2 st){ \n"
    "		vec2 i = floor(st); \n"
    "		vec2 f = fract(st); \n"
    // Four corners in 2D of a tile
    "		float a = random(i); \n"
    "		float b = random(i + vec2(1.0, 0.0)); \n"
    "		float c = random(i + vec2(0.0, 1.0)); \n"
    "		float d = random(i + vec2(1.0, 1.0)); \n"
    // Smooth Interpolation
    // Cubic Hermine Curve.  Same as SmoothStep()
    "		vec2 u = f*f*f*(f*(f*6.-15.)+10.); \n"
    // u = smoothstep(0.,1.,f);
    // Mix 4 coorners percentages
    "		return mix(a, b, u.x) + \n"
    "       	 (c - a)* u.y * (1.0 - u.x) + \n"
    "       	 (d - b) * u.x * u.y; \n"
    "	} \n"

    "   void main(){ \n" 
    "       vec4 color = texture2D(u_Sampler, v_TexCoord); \n"
    //"       gl_FragColor = v_Color * vec4(color.rgb, color.a); \n"
    //"       gl_FragColor = vec4(color.rgb, color.a); \n"
    "		float coordx = v_TexCoord.x; \n"
    "		float coordy = v_TexCoord.y; \n"
    "		float n = noise( vec2(coordx*5 ,coordy*5) ); \n"
    //"		vec2 st = gl_FragCoord.xy/u.resolution.xy;"
    "       gl_FragColor = vec4( vec3(n) ,1.0); \n"
    "   }\n";

typedef enum {
    a_Position,
    a_Color,
    a_TexCoord,
} attrib_id;

struct Matrix4 {
    float elements[16] = {1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1};

	void setElements(float (&e)[16]){
		for (int n = 0; n < 16; n++){
            elements[n] = e[n];
        }
	}

    //Set default matrix
    void setIdentity(){
        float e[16];
        e[0] = 1;   e[4] = 0;   e[8]  = 0;   e[12] = 0;
        e[1] = 0;   e[5] = 1;   e[9]  = 0;   e[13] = 0;
        e[2] = 0;   e[6] = 0;   e[10] = 1;   e[14] = 0;
        e[3] = 0;   e[7] = 0;   e[11] = 0;   e[15] = 1;
        setElements(e);
    }

    //Copies Matrix to another
    void copyFrom(Matrix4 old){
        for(int n = 0; n < 16; n++){
            elements[n] = old.elements[16-n];
        }
    }

    //Print Matrix
    void print(){
        for(int n = 0; n < 16; n++){
            cout << elements[n] << " ";
            if (n%4 == 3) cout << endl;
        }
        cout << endl;
    }

    //Reverse matrix
	void transpose(){
		float t;
		float e[16];
		for (int n = 0; n < 16; n++){
            e[n] = elements[n];
        }
		t = e[ 1];  e[ 1] = e[ 4];  e[ 4] = t;
		t = e[ 2];  e[ 2] = e[ 8];  e[ 8] = t;
		t = e[ 3];  e[ 3] = e[12];  e[12] = t;
		t = e[ 6];  e[ 6] = e[ 9];  e[ 9] = t;
		t = e[ 7];  e[ 7] = e[13];  e[13] = t;
		t = e[11];  e[11] = e[14];  e[14] = t;
		setElements(e);
	}

	//SetTranslate on Translation matrix
	void setTranslate(float x, float y, float z) {
		float e[16];
		for (int n = 0; n < 16; n++){
            e[n] = elements[n];
        }
		e[0] = 1;  e[4] = 0;  e[8]  = 0;  e[12] = x;
		e[1] = 0;  e[5] = 1;  e[9]  = 0;  e[13] = y;
		e[2] = 0;  e[6] = 0;  e[10] = 1;  e[14] = z;
		e[3] = 0;  e[7] = 0;  e[11] = 0;  e[15] = 1;
		setElements(e);
		return;
	};

	//Translate matrix by x, y, z - multiply by x, y, z
	void translate(float x, float y, float z) {
		float e[16]; 
  		for (int n = 0; n < 16; n++){
            e[n] = elements[n];
        }
		e[12] += e[0] * x + e[4] * y + e[8]  * z;
		e[13] += e[1] * x + e[5] * y + e[9]  * z;
		e[14] += e[2] * x + e[6] * y + e[10] * z;
		e[15] += e[3] * x + e[7] * y + e[11] * z;
		setElements(e);
		return;
	}

	//SetScale on Model matrix
	void setScale(float x, float y, float z) {
		float e[16];
		for (int n = 0; n < 16; n++){
            e[n] = elements[n];
        }
		e[0] = x;  e[4] = 0;  e[8]  = 0;  e[12] = 0;
		e[1] = 0;  e[5] = y;  e[9]  = 0;  e[13] = 0;
		e[2] = 0;  e[6] = 0;  e[10] = z;  e[14] = 0;
		e[3] = 0;  e[7] = 0;  e[11] = 0;  e[15] = 1;
		setElements(e);
		return;
	};

	//Scale on Model matrix, multiply by x y z
	void scale(float x, float y, float z) {
		float e[16];
		for (int n = 0; n < 16; n++){
            e[n] = elements[n];
        }
		e[0] *= x;  e[4] *= y;  e[8]  *= z;
		e[1] *= x;  e[5] *= y;  e[9]  *= z;
		e[2] *= x;  e[6] *= y;  e[10] *= z;
		e[3] *= x;  e[7] *= y;  e[11] *= z;
		setElements(e);
		return;
	};

	void setRotate( float angle, float x, float y, float z) {
		float s, c, len, rlen, nc, xy, yz, zx, xs, ys, zs;
		float e[16];
		for (int n = 0; n < 16; n++){
	        e[n] = elements[n];
	    }

		angle = M_PI * angle / 180.0;

		s = sin(angle);
		c = cos(angle);

		if (0 != x && 0 == y && 0 == z) {
	    	// Rotation around X axis
			if (x < 0) {
				s = -s;
			}
			e[0] = 1;  e[4] = 0;  e[ 8] = 0;  e[12] = 0;
			e[1] = 0;  e[5] = c;  e[ 9] =-s;  e[13] = 0;
			e[2] = 0;  e[6] = s;  e[10] = c;  e[14] = 0;
			e[3] = 0;  e[7] = 0;  e[11] = 0;  e[15] = 1;
		} else if (0 == x && 0 != y && 0 == z) {
		    // Rotation around Y axis
		    if (y < 0) {
				s = -s;
		    }
		    e[0] = c;  e[4] = 0;  e[ 8] = s;  e[12] = 0;
		    e[1] = 0;  e[5] = 1;  e[ 9] = 0;  e[13] = 0;
		    e[2] =-s;  e[6] = 0;  e[10] = c;  e[14] = 0;
		    e[3] = 0;  e[7] = 0;  e[11] = 0;  e[15] = 1;
		} else if (0 == x && 0 == y && 0 != z) {
	    	// Rotation around Z axis
	    	if (z < 0) {
				s = -s;
			}
		    e[0] = c;  e[4] =-s;  e[ 8] = 0;  e[12] = 0;
		    e[1] = s;  e[5] = c;  e[ 9] = 0;  e[13] = 0;
		    e[2] = 0;  e[6] = 0;  e[10] = 1;  e[14] = 0;
		    e[3] = 0;  e[7] = 0;  e[11] = 0;  e[15] = 1;
		} else {
	    	// Rotation around another axis
	    	len = sqrt(x*x + y*y + z*z);
	    	if (len != 1) {
				rlen = 1 / len;
				x *= rlen;
				y *= rlen;
				z *= rlen;
			}
			nc = 1 - c;
			xy = x * y;
			yz = y * z;
			zx = z * x;
			xs = x * s;
			ys = y * s;
			zs = z * s;

			e[ 0] = x*x*nc +  c;
			e[ 1] = xy *nc + zs;
			e[ 2] = zx *nc - ys;
			e[ 3] = 0;

			e[ 4] = xy *nc - zs;
			e[ 5] = y*y*nc +  c;
			e[ 6] = yz *nc + xs;
			e[ 7] = 0;

			e[ 8] = zx *nc + ys;
			e[ 9] = yz *nc - xs;
			e[10] = z*z*nc +  c;
			e[11] = 0;

			e[12] = 0;
			e[13] = 0;
			e[14] = 0;
			e[15] = 1;
		}
		setElements(e);

		return;
	};

	void rotate( float angle, float x, float y, float z ){
		Matrix4 temp;
		temp.setRotate(angle, x, y, z);
		concat( temp.elements );
	}

	void concat( float (&other)[16] ) {
		int i;
		float ai0, ai1, ai2, ai3;
		float e[16];
		float a[16];
		float b[16];
		// Calculate e = a * b
		for (int n = 0; n < 16; n++){
	        e[n] = elements[n];
	        a[n] = elements[n];
	        b[n] = other[n];
	    }
  
		for (i = 0; i < 4; i++) {
			ai0=a[i];  ai1=a[i+4];  ai2=a[i+8];  ai3=a[i+12];
			e[i]    = ai0 * b[0]  + ai1 * b[1]  + ai2 * b[2]  + ai3 * b[3];
			e[i+4]  = ai0 * b[4]  + ai1 * b[5]  + ai2 * b[6]  + ai3 * b[7];
			e[i+8]  = ai0 * b[8]  + ai1 * b[9]  + ai2 * b[10] + ai3 * b[11];
			e[i+12] = ai0 * b[12] + ai1 * b[13] + ai2 * b[14] + ai3 * b[15];
		}
		setElements(e);

		return;
	};

	// Set Perspective for Perspective Matrix
	void setPerspective(float fovy, float aspect, float near, float far) {
		float rd, s, ct;

		if (near == far || aspect == 0) {
			cerr << 'null frustum' << endl;
		}
		if (near <= 0) {
			cerr << 'near <= 0' << endl;
		}
		if (far <= 0) {
			cerr << 'far <= 0' << endl;
		}

		fovy = M_PI * fovy / 180.0 / 2.0;
  		s = sin(fovy);
		if (s == 0) {
			cerr << 'null frustum' << endl;
		}

		rd = 1.0 / (far - near);
		ct = cos(fovy) / s;

		float e[16]; 
  		for (int n = 0; n < 16; n++){
            e[n] = elements[n];
        }
		e[0]  = ct / aspect;
		e[1]  = 0;
		e[2]  = 0;
		e[3]  = 0;

		e[4]  = 0;
		e[5]  = ct;
		e[6]  = 0;
		e[7]  = 0;

		e[8]  = 0;
		e[9]  = 0;
		e[10] = -(far + near) * rd;
		e[11] = -1;

		e[12] = 0;
		e[13] = 0;
		e[14] = -2 * near * far * rd;
		e[15] = 0;
		setElements(e);
		return;
	}

	//Set Look At for Projection Matrix
	void setLookAt(float eyeX, float eyeY, float eyeZ, 
				float centerX, float centerY, float centerZ, 
				float upX, float upY, float upZ) {
		float fx, fy, fz, rlf, sx, sy, sz, rls, ux, uy, uz;

		fx = centerX - eyeX;
		fy = centerY - eyeY;
		fz = centerZ - eyeZ;

		// Normalize f.
		rlf = 1.0 / sqrt(fx*fx + fy*fy + fz*fz);
		fx *= rlf;
		fy *= rlf;
		fz *= rlf;

		// Calculate cross product of f and up.
		sx = fy * upZ - fz * upY;
		sy = fz * upX - fx * upZ;
		sz = fx * upY - fy * upX;

		// Normalize s.
		rls = 1.0 / sqrt(sx*sx + sy*sy + sz*sz);
		sx *= rls;
		sy *= rls;
		sz *= rls;

		// Calculate cross product of s and f.
		ux = sy * fz - sz * fy;
		uy = sz * fx - sx * fz;
		uz = sx * fy - sy * fx;

		// Set to this.
		float e[16]; 
  		for (int n = 0; n < 16; n++){
            e[n] = elements[n];
        }
		e[0] = sx;
		e[1] = ux;
		e[2] = -fx;
		e[3] = 0;

		e[4] = sy;
		e[5] = uy;
		e[6] = -fy;
		e[7] = 0;

		e[8] = sz;
		e[9] = uz;
		e[10] = -fz;
		e[11] = 0;

		e[12] = 0;
		e[13] = 0;
		e[14] = 0;
		e[15] = 1;
		setElements(e);
		// Translate.
		translate(-eyeX, -eyeY, -eyeZ);
		return;
	};

};

GLuint vao;

struct uniformStruct {
    GLuint Translation;
    GLuint ViewMatrix;
    GLuint ProjMatrix;
    GLuint ModelMatrix;
    GLuint Sampler;
    float Tx = 0.0;
    float Ty = 0.0;
    float Tz = 0.0;
} u;


struct Primitives {
    GLuint vertexBuffer;
    GLuint colorBuffer;
    GLuint indexBuffer;
    GLuint texCoordBuffer;
    GLuint textureID;
    int numIndices;
};

Primitives oneCube;
Primitives onePlane;

struct moveMent {
	float px = 0; //Player Position
	float py = 0;
	float pz = 1;
	float lx = 0; //Look
	float ly = 0;
	float lz = 0;
	float turn = -90;
	int moveUp = 0;
	int moveDown = 0;
	int moveLeft = 0;
	int moveRight = 0;
} user;

void NormalKeyHandler(unsigned char key, int x, int y){
	if (key == 32){ //Space
		cout << "Jump" << endl;
	}
}
void SpecialKeyUpHandler(int key, int x, int y){
	if (key == GLUT_KEY_RIGHT) user.moveRight = 0;
	if (key == GLUT_KEY_LEFT) user.moveLeft = 0;
	if (key == GLUT_KEY_UP) user.moveUp = 0;
	if (key == GLUT_KEY_DOWN) user.moveDown = 0;
}
void SpecialKeyHandler(int key, int x, int y){
	if (key == GLUT_KEY_RIGHT) user.moveRight = 1;
	if (key == GLUT_KEY_LEFT) user.moveLeft = 1;
	if (key == GLUT_KEY_UP) user.moveUp = 1;
	if (key == GLUT_KEY_DOWN) user.moveDown = 1;
}

void smoothNavigate(){
	float e = .15;
	if (user.moveRight == 1){
		user.turn += 5;
	}
	if (user.moveLeft == 1){
		user.turn -= 5;
	}
	if (user.moveUp){
		user.px = user.px + cos(user.turn*3.14/180)*e;
        user.pz = user.pz + sin(user.turn*3.14/180)*e; 
	}
	if (user.moveDown){
		user.px = user.px - cos(user.turn*3.14/180)*e;
        user.pz = user.pz - sin(user.turn*3.14/180)*e; 
	}
    user.lx = user.px + cos(user.turn*3.14/180);
    user.lz = user.pz + sin(user.turn*3.14/180);
}


GLuint initShader( GLenum type, const char* source ){

    GLuint shader;
    shader = glCreateShader( type );

    ///Compile Vertex shader
    GLint status;
    int length = strlen( source );
    glShaderSource( shader, 1, ( const GLchar ** )&source, &length );
    glCompileShader( shader );
    glGetShaderiv( shader, GL_COMPILE_STATUS, &status );

    if( status == GL_FALSE )
    {
        fprintf( stderr, "Fragment shader compilation failed.\n" );

        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> errorLog(maxLength);
        glGetShaderInfoLog(shader,  maxLength, &maxLength, &errorLog[0]);

        for(int n = 0; n < maxLength; n++){
            cerr << errorLog[n];
        }
        cerr << endl;

        glDeleteShader( shader ); // Don't leak the shader.
        return -1;
    }   
    return shader;
}

//https://www.opengl.org/archives/resources/code/samples/glut_examples/examples/examples.html

void initPlane(Primitives &o){
	// Create a cube
	//  v1------v0
	//  |       | 
	//  |       |
	//  |       |
	//  v2------v3

    const GLfloat vertices[] = {
     //  X   Y  Z
		.5, .5,  0,  -.5, .5,  0,  -.5,-.5,  0,   .5,-.5,  0,  
    };

    const GLfloat colors[] = {
     // R, G, B, A,
        1, 1, 0, 1,
        0, 1, 0, 1, 
        0, 0, 1, 1,

        1, 1, 0, 1, 
        0, 0, 1, 1, 
        1, 1, 1, 1,
    };

    float n = 3;
    const GLfloat texCoords[] = {
		n,   0.0,   n,n,        0.0, n,     0.0, 0.0,    // v0-v5-v6-v1 up
    };

    const GLuint indices[] = {
		 0, 1, 2,   0, 2, 3,    // front 
    };

    o.numIndices = sizeof(indices) / 4;

    // Create buffer objects
	glGenBuffers( 1, &o.vertexBuffer);
	glGenBuffers( 1, &o.colorBuffer );
	glGenBuffers( 1, &o.indexBuffer );
	glGenBuffers( 1, &o.texCoordBuffer );
	glGenTextures( 1, &o.textureID );

    //Position
    glBindBuffer( GL_ARRAY_BUFFER, o.vertexBuffer);
    glBufferData( GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(a_Position, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(a_Position);

    //Color
    glBindBuffer( GL_ARRAY_BUFFER, o.colorBuffer);
    glBufferData( GL_ARRAY_BUFFER, sizeof(colors), &colors[0], GL_STATIC_DRAW);
    glVertexAttribPointer(a_Color, 4, GL_FLOAT, GL_FALSE, 0, 0 );
    glEnableVertexAttribArray(a_Color);

    //Index Buffer
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, o.indexBuffer);
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices[0], GL_STATIC_DRAW);

	//Texture Coordinates
    glBindBuffer( GL_ARRAY_BUFFER, o.texCoordBuffer);
    glBufferData( GL_ARRAY_BUFFER, sizeof(texCoords), &texCoords[0], GL_STATIC_DRAW);
    glVertexAttribPointer(a_TexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(a_TexCoord);

    //Bind Texture
    glActiveTexture( GL_TEXTURE0);
    glBindTexture(   GL_TEXTURE_2D, o.textureID);

    int w, h;
    unsigned char* image = SOIL_load_image("../old_trinity.png", &w, &h, 0, SOIL_LOAD_RGB);

    float pixels[] = {
    	1.0f, 1.0f, 1.0f,   0.0f, 0.0f, 0.0f,
    	0.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f,
    };
    //Target active unit, level, internalformat, width, height, border, format, type, data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64,
    	0, GL_RGB, GL_FLOAT, image);

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D);

    //No Buffer Bound
	glBindBuffer( GL_ARRAY_BUFFER, NULL);
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, NULL);
}

void initCube(Primitives &o) {
	// Create a cube
	//    v6----- v5
	//   /|      /|
	//  v1------v0|
	//  | |     | |
	//  | |v7---|-|v4
	//  |/      |/
	//  v2------v3

	const GLfloat vertices[] = {
	    /*X,  Y,  Z  */
		 .5, .5, .5,  -.5, .5, .5,  -.5,-.5, .5,   .5,-.5, .5,  
		 .5, .5, .5,   .5,-.5, .5,   .5,-.5,-.5,   .5, .5,-.5,
		 .5, .5, .5,   .5, .5,-.5,  -.5, .5,-.5,  -.5, .5, .5,
		-.5, .5, .5,  -.5, .5,-.5,  -.5,-.5,-.5,  -.5,-.5, .5, 
		-.5,-.5,-.5,   .5,-.5,-.5,   .5,-.5, .5,  -.5,-.5, .5, 
		 .5,-.5,-.5,  -.5,-.5,-.5,  -.5, .5,-.5,   .5, .5,-.5 
    };

    float n = 3;
    const GLfloat texCoords[] = {
		1.0, 1.0,   0.0, 1.0,   0.0, 0.0,   1.0, 0.0,    // v0-v1-v2-v3 front
		0.0, 1.0,   0.0, 0.0,   1.0, 0.0,   1.0, 1.0,    // v0-v3-v4-v5 right
		n,   0.0,   n,n,        0.0, n,     0.0, 0.0,    // v0-v5-v6-v1 up
		1.0, 1.0,   0.0, 1.0,   0.0, 0.0,   1.0, 0.0,    // v1-v6-v7-v2 left
		0.0, 0.0,   1.0, 0.0,   1.0, 1.0,   0.0, 1.0,    // v7-v4-v3-v2 down
		0.0, 0.0,   1.0, 0.0,   1.0, 1.0,   0.0, 1.0     // v4-v7-v6-v5 back
    };

    const GLfloat colors[] = {
     /* R,   G,   B,  A, */
	    0.1, 0.1, 1.0,1,  0.1, 0.1, 1.0,1,  0.1, 0.1, 1.0,1,  0.1, 0.1, 1.0,1,
	    0.1, 1.0, 0.1,1,  0.1, 1.0, 0.1,1,  0.1, 1.0, 0.1,1,  0.1, 1.0, 0.1,1,
	    1.0, 0.1, 0.1,1,  1.0, 0.1, 0.1,1,  1.0, 0.1, 0.1,1,  1.0, 0.1, 0.1,1,
	    1.0, 1.0, 0.1,1,  1.0, 1.0, 0.1,1,  1.0, 1.0, 0.1,1,  1.0, 1.0, 0.1,1,
	    1.0, 1.0, 1.0,1,  1.0, 1.0, 1.0,1,  1.0, 1.0, 1.0,1,  1.0, 1.0, 1.0,1,
	    0.1, 1.0, 1.0,1,  0.1, 1.0, 1.0,1,  0.1, 1.0, 1.0,1,  0.1, 1.0, 1.0,1
    };

    const GLuint indices[] = {
		 0, 1, 2,   0, 2, 3,    // front
		 4, 5, 6,   4, 6, 7,    // right
		 8, 9,10,   8,10,11,    // up
		12,13,14,  12,14,15,    // left
		16,17,18,  16,18,19,    // down
		20,21,22,  20,22,23     // back    	
    };
  
    o.numIndices = sizeof(indices) / 4;

    // Create buffer objects
	glGenBuffers( 1, &o.vertexBuffer );
	glGenBuffers( 1, &o.colorBuffer );
	glGenBuffers( 1, &o.indexBuffer );
	glGenBuffers( 1, &o.texCoordBuffer );
	glGenTextures( 1, &o.textureID );

    //Position
    glBindBuffer( GL_ARRAY_BUFFER, o.vertexBuffer);
    glBufferData( GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(a_Position, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(a_Position);

    //Color
    glBindBuffer( GL_ARRAY_BUFFER, o.colorBuffer);
    glBufferData( GL_ARRAY_BUFFER, sizeof(colors), &colors[0], GL_STATIC_DRAW);
    glVertexAttribPointer(a_Color, 4, GL_FLOAT, GL_FALSE, 0, 0 );
    glEnableVertexAttribArray(a_Color);

    //Texture Coordinates
    glBindBuffer( GL_ARRAY_BUFFER, o.texCoordBuffer);
    glBufferData( GL_ARRAY_BUFFER, sizeof(texCoords), &texCoords[0], GL_STATIC_DRAW);
    glVertexAttribPointer(a_TexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(a_TexCoord);

    //Bind Texture
    glActiveTexture( GL_TEXTURE0);
    glBindTexture(   GL_TEXTURE_2D, o.textureID);

    int w, h;
    unsigned char* image = SOIL_load_image("../old_trinity.png", &w, &h, 0, SOIL_LOAD_RGB);

    float pixels[] = {
    	1.0f, 1.0f, 1.0f,   0.0f, 0.0f, 0.0f,
    	0.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f,
    };
    //Target active unit, level, internalformat, width, height, border, format, type, data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2,
    	0, GL_RGB, GL_FLOAT, pixels);

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D);

    //Index Buffer
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, o.indexBuffer);
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), &indices[0], GL_STATIC_DRAW);

    //No Buffer Bound
	glBindBuffer( GL_ARRAY_BUFFER, NULL);
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, NULL);

}




Matrix4 viewMatrix;
Matrix4 projMatrix;
Matrix4 modelMatrix;

void render(Primitives &o){

	//Activate Vertex Coordinates
    glBindBuffer( GL_ARRAY_BUFFER, o.vertexBuffer);
    glVertexAttribPointer(a_Position, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(a_Position);

  	//Activate Color Coordinates
	glBindBuffer( GL_ARRAY_BUFFER, o.colorBuffer);
	glVertexAttribPointer(a_Color, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(a_Color);

	//Activate Texture Coordinates
	glBindBuffer( GL_ARRAY_BUFFER, o.texCoordBuffer);
	glVertexAttribPointer(a_TexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(a_TexCoord);

	//Bind Texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture( GL_TEXTURE_2D, o.textureID);

	//Bind Indices
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, o.indexBuffer);

	//Update uniforms in frag vertex  //1 denotes number of matrixes to update
    glUniformMatrix4fv( u.ViewMatrix, 1, GL_TRUE, viewMatrix.elements);
    glUniformMatrix4fv( u.ProjMatrix, 1, GL_TRUE, projMatrix.elements);
    glUniformMatrix4fv( u.ModelMatrix, 1, GL_TRUE, modelMatrix.elements);
    glUniform1i( u.Sampler, 0);

    //DrawElements allows to display Cube, etc, with fewer indices
    glDrawElements( GL_TRIANGLE_STRIP, o.numIndices, GL_UNSIGNED_INT, 0);

   
}


void display(int te){

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glutTimerFunc(1000.0/60.0, display, 1);
    smoothNavigate(); //Update user movement

    u.Tx += 1;
    glUniform4f(u.Translation, u.Tx, u.Ty, u.Tz, 0.0);

    //Initialize Matrices
	projMatrix.setPerspective(90.0, (float)WIDTH/(float)HEIGHT, 0.1, 250.0);
	//eyeX, eyeY, eyeZ, (at)centerX, (at)centerY, (at)centerZ, upX, upY, upZ
	viewMatrix.setLookAt(user.px, user.py, user.pz,    user.lx, user.ly, user.lz,    0.0, 1.0, 0.0);

	//Cube
	modelMatrix.setTranslate(-1,0,-1);
	modelMatrix.rotate(u.Tx, 0,1,0);
	render(oneCube);
    
    //Plane
	modelMatrix.setTranslate(1,0,-1);
	//modelMatrix.rotate(u.Tx*.9, 1,1,1);
	render(onePlane);
   
    glutSwapBuffers();

}


int main(int argc, char** argv)
{

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitContextVersion (3, 2);
    glutInitContextFlags (GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);
    
    glutInitWindowSize(WIDTH, HEIGHT);
    glutInitWindowPosition(100,100);
    glutCreateWindow("OpenGL - First window demo");
    glutSpecialFunc(SpecialKeyHandler);
    glutSpecialUpFunc(SpecialKeyUpHandler);
    glutKeyboardFunc(NormalKeyHandler);

    // Initialize GLEW
    glewExperimental = GL_TRUE; 
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    if(GLEW_VERSION_3_0)
    {
        //cerr << "GlEW Available";
    }else
        return 0;

    GLuint vs, fs, program;
    vs = initShader( GL_VERTEX_SHADER, vertex_source );
    fs = initShader( GL_FRAGMENT_SHADER, fragment_source );
    if (vs == -1 || fs == -1){ return 0; }

    //Create and use shader program
    program = glCreateProgram();
    glAttachShader( program, vs );
    glAttachShader( program, fs );

    //Must link after BindAttrib
    glLinkProgram( program );
    glUseProgram( program );

	glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );
    glClearColor( 0.0, 0.0, 0.5, 1.0 );
    glViewport( 0, 0, WIDTH, HEIGHT );

    //VAO?
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );


    //Storage Locations for Uniforms
    u.Translation = glGetUniformLocation( program, "u_Translation" );
	u.ProjMatrix = glGetUniformLocation( program, "u_ProjMatrix");
	u.ViewMatrix = glGetUniformLocation( program, "u_ViewMatrix");
	u.ModelMatrix = glGetUniformLocation( program, "u_ModelMatrix");
	u.Sampler = glGetUniformLocation( program, "u_Sampler");

    //Storage locations for Attributes
    glBindAttribLocation( program, a_Position, "a_Position" );
    glBindAttribLocation( program, a_Color, "a_Color" );
    glBindAttribLocation( program, a_TexCoord, "a_TexCoord" );

    //Buffers (a_ attributes)
    initPlane(onePlane);
    initCube(oneCube);

    glutTimerFunc(1000.0/60.0, display, 1);
    glutMainLoop();

    return 0;
}


