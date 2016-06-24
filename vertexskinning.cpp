// Vertex Skinning 
// As seen on: https://www.youtube.com/watch?v=0K2W7I9c4ms
// Johannes Pagwiwoko
// http://yoarikso.tumblr.com


#include <GL/glut.h>
#include <GL/gl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define OGL_AXIS_DLIST	1
#define OGL_FLOORMESH_DLIST 2
#define UPPER_ARM_ID 4
#define LOWER_ARM_ID 5

#define MESH_HEIGHT 10;
#define STRIP_LENGTH 10;

#define M_PI        3.14159265358979323846
#define HALF_PI	    1.57079632679489661923

class Translation {
public:
	float matrix[16];
	float x, y, z;
};

class Rotation {
public:
	float matrix[16];
	float x, y, z;
};

class Scale {
public:
	float x, y, z;
	float matrix[16];
};

typedef struct tBone 
{
	int id;
	int childCount;
	
	Translation trans;
	Scale scale;
	Rotation rot;

	float matrix[16];
	struct tBone *child;
} Bone;

class Vertex {
public:
	float x, y, z, w; // w is ALWAYS 1
	float coordinates[4];
	int boneID1, boneID2; // the two bones this vertex is influenced by
	float weight1, weight2; // the corresponding weight for bone1 and bone2 respectively

	Vertex() {
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
		w = 1.0f;
		
		coordinates[0] = z;
		coordinates[1] = x;
		coordinates[2] = y;
		coordinates[3] = w;
		
		boneID1 = 0;
		boneID2 = 0;
		weight1 = 1;
		weight2 = 1;
	}

	Vertex(float x, float y, float z) {
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = 1.0;
		
		coordinates[0] = this->x;
		coordinates[1] = this->y;
		coordinates[2] = this->z;
		coordinates[3] = this->w;
	}

	Vertex(float x, float y, float z, int boneID1, int boneID2, float weight1, float weight2) {
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = 1.0;
		this->boneID1 = boneID1;
		this->boneID2 = boneID2;
		this->weight1 = weight1;
		this->weight2 = weight2;
		
		coordinates[0] = this->x;
		coordinates[1] = this->y;
		coordinates[2] = this->z;
		coordinates[3] = this->w;
	}

	
	float* getVertex() {
		return coordinates;
	}
};

float* multMatrixByConstant(const float* aMatrix, float constant) {
	float* resultMatrix = (float *) malloc(16 * sizeof(float));

	for(int i = 0; i < 16; i++) {
		resultMatrix[i] = 0.0f;
	}

	for(int i = 0; i < 16; i++) {
		resultMatrix[i] = aMatrix[i] * constant;
	}
	return resultMatrix;
}

float* multVectorByConstant(const float* aVector, float constant) {
	float* resultVector = (float *) malloc(4 * sizeof(float));

	for(int i = 0; i < 4; i++) {
		resultVector[i] = aVector[i] * constant;
	}
	return resultVector;
}

float* addVectors(const float* aVector, const float *bVector) {
	float* resultVector = (float *) malloc(4 * sizeof(float));

	for(int i = 0; i < 4; i++) {
		resultVector[i] = aVector[i] * bVector[i];
	}
	return resultVector;
}

float* addMatrix(const float* aMatrix, const float* bMatrix) {
	float* resultMatrix = (float *) malloc(16 * sizeof(float));
	
	for(int i = 0; i < 16; i++) {
		resultMatrix[i] = 0.0f;
	}

	for(int i = 0; i < 16; i++) {
		resultMatrix[i] = aMatrix[i] + bMatrix[i];
	}
	return resultMatrix;
}

float* multMatrixByMatrix(const float* aMatrix, const float* bMatrix){
	float* resultMatrix = (float *) malloc(16 * sizeof(float));
	double sum;

	for(int i = 0; i < 16; i++) {
		resultMatrix[i] = 0.0f;
	}

	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			sum = 0.0f;
			for(int k = 0; k < 4; k++) {
				sum += aMatrix[i + k*4] * bMatrix[j*4 + k];
			}
			resultMatrix[i + j*4] = sum;
		}
	}
	return resultMatrix;
}

//COLUMN MAJOR
float* multMatrixByVector(const float A[16], const float b[4]) {
	float* resultVector = (float *) malloc(4 * sizeof(float));
	resultVector[0] = A[0] * b[0] + A[4] * b[1] + A[8] * b[2] + A[12] * b[3];
	resultVector[1] = A[1] * b[0] + A[5] * b[1] + A[9] * b[2] + A[13] * b[3];
	resultVector[2] = A[2] * b[0] + A[6] * b[1] + A[10] * b[2] + A[14] * b[3];
	resultVector[3] = A[3] * b[0] + A[7] * b[1] + A[11] * b[2] + A[15] * b[3];
	return resultVector;
}

Bone *upperArm;
Bone *lowerArm;

float cameraAngle = 0.0f;
float cameraRadius = 80.0f;
float xeye = 0, yeye = 0, zeye = cameraRadius;

int weightCaseNumber = 1;
char *weightCaseStr = "Weighting Case 1";

Vertex originalMesh [22][37];
Vertex weightedMesh [22][37];

void normal(double x1, double y1, double z1, 
			double x2, double y2, double z2, 
			double x3, double y3, double z3) 
{
	double nx, ny, nz;
	nx = y1 * (z2-z3) + y2 * (z3-z1) + y3 * (z1-z2);
	ny = z1 * (x2-x3) + z2 * (x3-x1) + z3 * (x1-x2);
	nz = x1 * (y2-y3) + x2 * (y3-y1) + x3 * (y1-y2);
	glNormal3f(nx, ny, nz);
}

void renderText(GLfloat x, GLfloat y, char* s, GLint red, GLint green, GLint blue)
{
	int lines;
    char* p;
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    	glLoadIdentity();
     	glOrtho(0.0, glutGet(GLUT_WINDOW_WIDTH), 0.0, glutGet(GLUT_WINDOW_HEIGHT), -1.0, 1.0);
     	glMatrixMode(GL_MODELVIEW);
      	glPushMatrix();
      		glLoadIdentity();
      		glColor3ub(red, green, blue);
      		glRasterPos2f(x, y);
      		for(p = s, lines = 0; *p; p++) {
	  			if (*p == '\n') {
	      			lines++;
	      			glRasterPos2f(x, y-(lines*18));
	  			}
	  			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
      		}
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
     glPopMatrix();
     glMatrixMode(GL_MODELVIEW);
}

void createBoneAxisDList() 
{
		glNewList(OGL_AXIS_DLIST,GL_COMPILE);
		glBegin(GL_LINES);
			glColor3f(1.0f, 0.0f, 0.0f);	// X AXIS STARTS - COLOR RED
			glVertex3f(-1.0f,  0.0f, 0.0f);
			glVertex3f( 1.0f,  0.0f, 0.0f);
			glVertex3f( 1.0f,  0.0f, 0.0f);	// TOP PIECE OF ARROWHEAD
			glVertex3f( 0.9f,  0.1f, 0.0f);
			glVertex3f( 1.0f,  0.0f, 0.0f);	// BOTTOM PIECE OF ARROWHEAD
			glVertex3f( 0.9f, -0.1f, 0.0f);

			glColor3f(0.0f, 1.0f, 0.0f);	// Y AXIS STARTS - COLOR GREEN
			glVertex3f( 0.0f,  1.0f, 0.0f);
			glVertex3f( 0.0f, -1.0f, 0.0f);			
			glVertex3f( 0.0f,  1.0f, 0.0f);	// TOP PIECE OF ARROWHEAD
			glVertex3f( 0.1f,  0.9f, 0.0f);
			glVertex3f( 0.0f,  1.0f, 0.0f);	// BOTTOM PIECE OF ARROWHEAD
			glVertex3f( -0.1f,  0.9f, 0.0f);

			glColor3ub(51, 51, 255); 	    // Z AXIS STARTS - COLOR BLUE
			glVertex3f( 0.0f,  0.0f,  1.0f);
			glVertex3f( 0.0f,  0.0f, -1.0f);
			glVertex3f( 0.0f,  0.0f, 1.0f);	// TOP PIECE OF ARROWHEAD
			glVertex3f( 0.0f,  0.1f, 0.9f);
			glVertex3f( 0.0f, 0.0f, 1.0f);	// BOTTOM PIECE OF ARROWHEAD
			glVertex3f( 0.0f, -0.1f, 0.9f);
		glEnd();
	glEndList();
}

void createBoneDLists(Bone *bone) {	
	if(bone->childCount > 0)
	{
		glNewList(bone->id, GL_COMPILE);
			glBegin(GL_LINE_STRIP);
				glVertex3f( 0.0f, 0.4f, 0.0f); // 0
				glVertex3f(-0.4f, 0.0f,-0.4f); // 1
				glVertex3f( 0.4f, 0.0f,-0.4f); // 2
				glVertex3f( 0.0f, bone->child->trans.y, 0.0f); // Base

				glVertex3f(-0.4f, 0.0f,-0.4f); // 1
				glVertex3f(-0.4f, 0.0f, 0.4f); // 4
				glVertex3f( 0.0f, 0.4f, 0.0f); // 0

				glVertex3f( 0.4f, 0.0f,-0.4f); // 2
				glVertex3f( 0.4f, 0.0f, 0.4f); // 3

				glVertex3f( 0.0f, 0.4f, 0.0f); // 0
				glVertex3f(-0.4f, 0.0f, 0.4f); // 4
				glVertex3f( 0.0f, bone->child->trans.y, 0.0f); // Base
				glVertex3f( 0.4f, 0.0f, 0.4f); // 3
				glVertex3f(-0.4f, 0.0f, 0.4f); // 4
			glEnd();
		glEndList();

		if(bone->childCount > 0) {
			createBoneDLists(bone->child);
		}
	}
}

void printBoneMatrix(Bone* bone) 
{
	printf("boneID: %d\n", bone->id);
	printf("BoneMatrix:\n");
	for(int i = 0; i < 16; i++) {
		printf("%f ", bone->matrix[i]);
	}
	printf("\n");
}

void drawSkeleton(Bone *rootBone) 
{
	int loop;
	Bone *currentBone;

	currentBone = rootBone;
	for(loop = 0; loop < rootBone->childCount; loop++) {
		glPushMatrix();

			// Set base orientation and position
			glTranslatef(currentBone->trans.x, currentBone->trans.y, currentBone->trans.z);

			glRotatef(currentBone->rot.z, 0.0f, 0.0f, 1.0f);
			glRotatef(currentBone->rot.y, 0.0f, 1.0f, 0.0f);
			glRotatef(currentBone->rot.x, 1.0f, 0.0f, 0.0f);

			// The scale is local so push and pop
			glPushMatrix();
				glScalef(currentBone->scale.x, currentBone->scale.y, currentBone->scale.z);
				
				// draw the openGL axis object
				glCallList(OGL_AXIS_DLIST);

				// draw the actual bone structure, only make a bone if there is a child
				if(currentBone->childCount > 0) {
					glColor3ub(224, 224, 0);
					glCallList(currentBone->id);
				}

				// grab the matrix at this point so that we can use it for the deformation
				//glGetFloatv(GL_MODELVIEW_MATRIX, currentBone->matrix);
				float scaleMatrix[16] = {currentBone->scale.x, 0, 0, 0, 
									  0, currentBone->scale.y, 0, 0,
									  0, 0, currentBone->scale.z, 0,
									  0, 0, 0, 1};

				float transMatrix[16] = {1, 0, 0, 0, 
									  0, 1, 0, 0,
									  0, 0, 1, 0,
									  currentBone->trans.x, currentBone->trans.y, currentBone->trans.z, 1};

				float rotXMatrix[16] = {1, 0, 0, 0,
										0, cos(currentBone->rot.x * M_PI /180), sin(currentBone->rot.x * M_PI / 180), 0,
										0, -sin(currentBone->rot.x * M_PI / 180), cos(currentBone->rot.x * M_PI /180), 0,
										0, 0, 0, 1};
				float rotYMatrix[16] = {cos(currentBone->rot.y * M_PI /180), 0, -sin(currentBone->rot.y * M_PI / 180), 0,
										0, 1, 0, 0,
										sin(currentBone->rot.y * M_PI / 180), 0, cos(currentBone->rot.y * M_PI /180), 0,
										0, 0, 0, 1};
				float rotZMatrix[16] = {cos(currentBone->rot.z * M_PI /180), sin(currentBone->rot.z * M_PI / 180), 0, 0,
										-sin(currentBone->rot.z * M_PI / 180), cos(currentBone->rot.z * M_PI /180), 0, 0,
										0, 0, 1, 0,
										0, 0, 0, 1};

				float identity[16] = {1, 0, 0, 0,
									  0, 1, 0, 0,
									  0, 0, 1, 0,
									  0, 0, 0, 1};

				float* afterScaling = multMatrixByMatrix(scaleMatrix, identity);
				float* afterRotX = multMatrixByMatrix(rotXMatrix, afterScaling);
				float* afterRotY = multMatrixByMatrix(rotYMatrix, afterRotX);
				float* afterRotZ = multMatrixByMatrix(rotZMatrix, afterRotY);

				free(afterScaling);
				free(afterRotX);
				free(afterRotY);

				for(int i = 0; i < 16; i++) {
					currentBone->matrix[i] = afterRotZ[i];
				}
				free(afterRotZ);

			glPopMatrix();

			// check if this bone has children, do recursive call if true
			if(currentBone->childCount > 0) {
				drawSkeleton(currentBone->child);
			}
		glPopMatrix();
	}
}

void createFloorMeshDisplayList() 
{
	printf("floor DList id: %d\n", OGL_FLOORMESH_DLIST);
	glNewList(OGL_FLOORMESH_DLIST, GL_COMPILE);
		glBegin(GL_LINES);
		glNormal3f(0.0, 1.0, 0.0);
		for(int i= -50; i <=50; i++) {
			// "horizontal lines", drawing by varying the z-values
			glVertex3i(-50, 0, i);
			glVertex3i(50, 0, i);
			// "vertical lines", drawn by varying the x-values
			glVertex3i(i, 0, -50);
			glVertex3i(i, 0, 50);
		}
		glEnd();
	glEndList();
}

void createArmPointMesh()
{	
	glPointSize(2.5);
	glBegin(GL_POINTS);
	for(int i = 0; i <= 20; i++) {
		for(int j = 0; j < 36; j += 1) {
			glVertex3f(weightedMesh[i][j].x, weightedMesh[i][j].y, weightedMesh[i][j].z);
		}
	}
	glEnd();
}

void createArmMesh(int height, float radius) 
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	for(int i = 0; i < height * 2; i++) {
		glBegin(GL_TRIANGLE_STRIP);
		for(int alpha = 0; alpha <= 360; alpha += 10) {
			glVertex3f(radius * sin(alpha * M_PI / 180), 0.5 * i, radius * cos(alpha * M_PI / 180));
			glVertex3f(radius * sin(alpha * M_PI / 180), 0.5 * i + 0.5, radius * cos(alpha * M_PI / 180));
		}
		glEnd();
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// row in the 2D-array mesh
void setWeights(int row, float newWeight) {
	for(int i = 0; i < 37; i++) {
		originalMesh[row][i].weight1 = newWeight;
		originalMesh[row][i].weight2 = 1.0f - newWeight;
	}
}

void weightCase1() {	
	setWeights(0, 0.00f);
	setWeights(1, 0.05f);
	setWeights(2, 0.10f);
	setWeights(3, 0.15f);
	setWeights(4, 0.20f);
	setWeights(5, 0.25f);
	
	setWeights(6, 0.30f);
	setWeights(7, 0.35f);
	setWeights(8, 0.40f);
	setWeights(9, 0.45f);
	setWeights(10, 0.50f);
	setWeights(11, 0.55f);
	setWeights(12, 0.60f);
	setWeights(13, 0.65f);
	setWeights(14, 0.70f);
	
	setWeights(15, 0.75f);
	setWeights(16, 0.80f);
	setWeights(17, 0.85f);
	setWeights(18, 0.90f);
	setWeights(19, 0.95f);
	setWeights(20, 1.0f);
}

void weightCase2() {
	setWeights(0, 0.00f);
	setWeights(1, 0.00f);
	setWeights(2, 0.00f);
	setWeights(3, 0.00f);
	setWeights(4, 0.00f);
	setWeights(5, 0.00f);
	
	setWeights(6, 0.10f);
	setWeights(7, 0.25f);
	setWeights(8, 0.30f);
	setWeights(9, 0.45f);
	setWeights(10, 0.50f);
	setWeights(11, 0.65f);
	setWeights(12, 0.70f);
	setWeights(13, 0.85f);
	setWeights(14, 0.90f);
	
	setWeights(15, 1.0f);
	setWeights(16, 1.0f);
	setWeights(17, 1.0f);
	setWeights(18, 1.0f);
	setWeights(19, 1.0f);
	setWeights(20, 1.0f);
}

void weightCase3() {
	setWeights(0, 0.00f);
	setWeights(1, 0.00f);
	setWeights(2, 0.00f);
	setWeights(3, 0.00f);
	setWeights(4, 0.00f);
	setWeights(5, 0.00f);
	setWeights(6, 0.00f);
	setWeights(7, 0.00f);
	
	setWeights(8, 0.20f);
	setWeights(9, 0.30f);
	setWeights(10, 0.40f);
	setWeights(11, 0.50f);
	setWeights(12, 0.60f);
	
	setWeights(13, 1.00f);
	setWeights(14, 1.00f);
	setWeights(15, 1.00f);
	setWeights(16, 1.00f);
	setWeights(17, 1.00f);
	setWeights(18, 1.00f);
	setWeights(19, 1.00f);
	setWeights(20, 1.00f);
}

void weightCase4() {
	setWeights(0, 0.00f);
	setWeights(1, 0.00f);
	setWeights(2, 0.00f);
	setWeights(3, 0.00f);
	setWeights(4, 0.00f);
	setWeights(5, 0.00f);
	
	setWeights(6, 0.00f);
	setWeights(7, 0.00f);
	setWeights(8, 0.00f);
	setWeights(9, 0.00f);
	setWeights(10, 0.00f);
	setWeights(11, 0.00f);
	setWeights(12, 0.00f);
	setWeights(13, 0.00f);
	setWeights(14, 0.00f);
	
	setWeights(15, 0.00f);
	setWeights(16, 0.00f);
	setWeights(17, 0.00f);
	setWeights(18, 0.00f);
	setWeights(19, 0.00f);
	setWeights(20, 0.00f);
}

void weightCase5() {
	setWeights(0, 1.00f);
	setWeights(1, 1.00f);
	setWeights(2, 1.00f);
	setWeights(3, 1.00f);
	setWeights(4, 1.00f);
	setWeights(5, 1.00f);
	
	setWeights(6, 1.00f);
	setWeights(7, 1.00f);
	setWeights(8, 1.00f);
	setWeights(9, 1.00f);
	setWeights(10, 1.00f);
	setWeights(11, 1.00f);
	setWeights(12, 1.00f);
	setWeights(13, 1.00f);
	setWeights(14, 1.00f);
	
	setWeights(15, 1.00f);
	setWeights(16, 1.00f);
	setWeights(17, 1.00f);
	setWeights(18, 1.00f);
	setWeights(19, 1.00f);
	setWeights(20, 1.00f);
}

void setWeightCase(int number) {
	switch(number) {
		case 1: weightCase1(); break;
		case 2: weightCase2(); break;
		case 3: weightCase3(); break;
		case 4: weightCase4(); break;
		case 5: weightCase5(); break;
		default: weightCase1(); break;
	}
}

void createOriginalMeshMatrix(int height, float radius) 
{
	float weight1 = 0.0f;
	float weight2 = 0.0f;
	for(int i = 0; i <= height * 2; i++) {
		for(int alpha = 0; alpha < 370; alpha += 10) {		
			Vertex vertex(radius * sin(alpha * M_PI / 180), 0.5 * i, radius * cos(alpha * M_PI / 180), UPPER_ARM_ID, LOWER_ARM_ID, weight1, weight2);
			originalMesh[i][alpha/10] = vertex;
		}
	}
	setWeightCase(weightCaseNumber);
}

void drawOriginalArmMesh() {
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	for(int i = 0; i < 21; i++) {
		glBegin(GL_QUAD_STRIP);
		for(int j = 0; j < 37; j++) {
			glVertex3f(originalMesh[i][j].x, originalMesh[i][j].y, originalMesh[i][j].z);
			glVertex3f(originalMesh[i+1][j].x, originalMesh[i+1][j].y, originalMesh[i+1][j].z);
		}
		glEnd();
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void createWeightedMeshMatrix() {

	for(int i = 0; i < 22; i++) {
		for(int j = 0; j < 37; j++) {
			float* upperMatrix = multMatrixByConstant(upperArm->matrix, originalMesh[i][j].weight1); // w1M1
			float* lowerMatrix = multMatrixByConstant(lowerArm->matrix, originalMesh[i][j].weight2); // w2M2
			float* weightedMatrix = addMatrix(upperMatrix, lowerMatrix); // w1M1 + w2M2
			
			float origin[4] = { originalMesh[i][j].x, originalMesh[i][j].y - 5, originalMesh[i][j].z, 1}; // where the bone base lies (0, 5, 0);
			
			float* finalPosition = multMatrixByVector(weightedMatrix, origin); // (w1M1 + w2M2) * V
			weightedMesh[i][j] = Vertex(finalPosition[0], finalPosition[1], finalPosition[2], UPPER_ARM_ID, LOWER_ARM_ID, originalMesh[i][j].weight1, originalMesh[i][j].weight2);
			
			free(upperMatrix);
			free(lowerMatrix);
			free(weightedMatrix);
			free(finalPosition);
		}
	}
}

void drawWeightedArmMesh() 
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	for(int i = 0; i < 20; i++) {
		glBegin(GL_QUAD_STRIP);
		for(int j = 0; j < 37; j++) {
			glVertex3f(weightedMesh[i][j].x, weightedMesh[i][j].y, weightedMesh[i][j].z);
			glVertex3f(weightedMesh[i+1][j].x, weightedMesh[i+1][j].y, weightedMesh[i+1][j].z);
		}
		glEnd();
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void display()
{
	float lightPos[4] = {0.0, 10.0, 0.0, 1.0};

	zeye = cameraRadius * cos(cameraAngle / 180.0 * M_PI);
	xeye = cameraRadius * sin(cameraAngle / 180.0 * M_PI);
	
	glMatrixMode(GL_MODELVIEW);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	glLoadIdentity();
	
	renderText(10.0f, 10.0f, weightCaseStr, 215, 215, 215);
	gluLookAt(xeye, yeye, zeye, 0.0, yeye, 0.0, 0.0, 1.0, 0.0);

	// set light source parameter
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
	
   	// draw floor Mesh
   	float matDif1[4] = {0.0, 1.0, 0.0, 1.0};
	float matAmb1[4] = {0.0, 1.0, 0.0, 1.0};
	glMaterialfv(GL_FRONT, GL_DIFFUSE, matDif1);
   	glMaterialfv(GL_FRONT, GL_AMBIENT, matAmb1);
   	
	//glPushMatrix();
		//glColor3ub(0, 255, 0);
		//glCallList(OGL_FLOORMESH_DLIST);
	//glPopMatrix();
	
	glPushMatrix();
		drawSkeleton(upperArm);
	glPopMatrix();
	
	glPushMatrix();
		glColor3ub(102, 0, 51);
		createOriginalMeshMatrix(11, 1.75f);
		//drawOriginalArmMesh();
		createWeightedMeshMatrix();
		drawWeightedArmMesh();
		glColor3ub(255, 255, 255);
		createArmPointMesh();
	glPopMatrix();
	
	glFlush();
	glutSwapBuffers();
}

void animate()
{
	glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) 
{
	switch(key) {
		case 'd': cameraAngle += 10; break;
		case 'a': cameraAngle -= 10; break;
		case 'w': cameraRadius -= 1; break;
		case 's': cameraRadius += 1; break;
		case 'y': lowerArm->rot.y += 2; break;

		case '1': weightCaseNumber = 1; 
				  weightCaseStr = "Weighting Case 1"; break;
		case '2': weightCaseNumber = 2;
				  weightCaseStr = "Weighting Case 2"; break;
		case '3': weightCaseNumber = 3;
				  weightCaseStr = "Weighting Case 3"; break;
		case '4': weightCaseNumber = 4;
				  weightCaseStr = "Weighting Case 4"; break;
		case '5': weightCaseNumber = 5;
				  weightCaseStr = "Weighting Case 5"; break;

		case 27 : exit(EXIT_SUCCESS);
	}
	glutPostRedisplay();
}

void specialKeyboard(int key, int x, int y) {
	switch(key) {
		case GLUT_KEY_UP: yeye += 1; break;
		case GLUT_KEY_DOWN: yeye -= 1; break;
		case GLUT_KEY_LEFT: lowerArm->rot.z -= 2; break;
		case GLUT_KEY_RIGHT: lowerArm->rot.z += 2; break;
	}
	glutPostRedisplay();
}

void initializeGL()	
{
	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	glColor3f(1.0,1.0,0.0);
	glEnable(GL_DEPTH_TEST);
	
	//glEnable(GL_LIGHTING); // Enable lighting
	//glEnable(GL_LIGHT0); // enable light source 0
	
	glEnable(GL_NORMALIZE);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glFrustum(-1.0, 1.0, -1.0, 1.0, 10.0, 100.0);

	createBoneAxisDList();
	createFloorMeshDisplayList();
}

void initializeSkeleton() 
{
	upperArm = new Bone();
	lowerArm = new Bone();
	Bone *endBone = new Bone();

	upperArm->id = UPPER_ARM_ID;
	lowerArm->id = LOWER_ARM_ID;

	upperArm->scale.x = 1.0f;
	upperArm->scale.y = 1.0f;
	upperArm->scale.z = 1.0f;

	upperArm->trans.x = 0.0f;
	upperArm->trans.y = 5.0f;
	upperArm->trans.z = 0.0f;
	
	upperArm->child = lowerArm;
	upperArm->childCount = 1;

	lowerArm->scale.x = 1.0f;
	lowerArm->scale.y = 1.0f;
	lowerArm->scale.z = 1.0f;

	lowerArm->trans.x = 0.0f;
	lowerArm->trans.y = -5.0f; // -5 wrt upperArm's base
	lowerArm->trans.z = 0.0f;

	lowerArm->rot.z = 0.0f;
	lowerArm->child = endBone;
	lowerArm->childCount = 1;

	endBone->trans.y = -5;

	createBoneDLists(upperArm); // -5 wrt lowerArm's base
}

int main(int argc, char** argv)
{
	// initialize glut
    glutInit(&argc, argv);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize (600, 600); 
    glutInitWindowPosition (100, 100);
    glutCreateWindow (argv[0]);
   
	//initialize main stuff
    initializeGL();
    initializeSkeleton();

    glutDisplayFunc(display); 
    glutIdleFunc(animate);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(specialKeyboard);

    glutMainLoop();

    return 0;
}

