#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

// Begin of shader setup
#include "Shaders/LoadShaders.h"
GLuint h_ShaderProgram; // handle to shader program
GLint loc_ModelViewProjectionMatrix, loc_primitive_color; // indices of uniform variables

void prepare_shader_program(void) {
	ShaderInfo shader_info[3] = {
		{ GL_VERTEX_SHADER, "Shaders/simple.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/simple.frag" },
		{ GL_NONE, NULL }
	};

	h_ShaderProgram = LoadShaders(shader_info);
	glUseProgram(h_ShaderProgram);

	loc_ModelViewProjectionMatrix = glGetUniformLocation(h_ShaderProgram, "u_ModelViewProjectionMatrix");
	loc_primitive_color = glGetUniformLocation(h_ShaderProgram, "u_primitive_color");
}
// End of shader setup

// Begin of geometry setup
#define BUFFER_OFFSET(offset) ((GLvoid *) (offset))
#define INDEX_VERTEX_POSITION 0

GLuint axes_VBO, axes_VAO;
GLfloat axes_vertices[6][3] = {
	{ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }
};
GLfloat axes_color[3][3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };

void prepare_axes(void) { 
	// Initialize vertex buffer object.
	glGenBuffers(1, &axes_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, axes_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(axes_vertices), &axes_vertices[0][0], GL_STATIC_DRAW);

	// Initialize vertex array object.
	glGenVertexArrays(1, &axes_VAO);
	glBindVertexArray(axes_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, axes_VBO);
	glVertexAttribPointer(INDEX_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(INDEX_VERTEX_POSITION);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_axes(void) {
	glBindVertexArray(axes_VAO);
	glUniform3fv(loc_primitive_color, 1, axes_color[0]);
	glDrawArrays(GL_LINES, 0, 2);
	glUniform3fv(loc_primitive_color, 1, axes_color[1]);
	glDrawArrays(GL_LINES, 2, 2);
	glUniform3fv(loc_primitive_color, 1, axes_color[2]);
	glDrawArrays(GL_LINES, 4, 2);
	glBindVertexArray(0);
}

#define N_OBJECTS 3
#define OBJECT_SQUARE16 0
#define OBJECT_TIGER 1
#define OBJECT_COW 2
GLuint object_VBO[N_OBJECTS], object_VAO[N_OBJECTS];
int object_n_triangles[N_OBJECTS];
GLfloat *object_vertices[N_OBJECTS];

int read_triangular_mesh(GLfloat **object, int bytes_per_primitive, char *filename) {
	int n_triangles;
	FILE *fp;

	fprintf(stdout, "Reading geometry from the geometry file %s...\n", filename);
	fp = fopen(filename, "rb");
	if (fp == NULL){
		fprintf(stderr, "Cannot open the object file %s ...", filename);
		return -1;
	}
	fread(&n_triangles, sizeof(int), 1, fp);
	*object = (float *)malloc(n_triangles*bytes_per_primitive);
	if (*object == NULL){
		fprintf(stderr, "Cannot allocate memory for the geometry file %s ...", filename);
		return -1;
	}

	fread(*object, bytes_per_primitive, n_triangles, fp);
	fprintf(stdout, "Read %d primitives successfully.\n\n", n_triangles);
	fclose(fp);

	return n_triangles;
}

void set_up_object(int object_ID, char *filename, int n_bytes_per_vertex) {
	object_n_triangles[object_ID] = read_triangular_mesh(&object_vertices[object_ID], 
		3*n_bytes_per_vertex, filename);
	// Error checking is needed here.

	// Initialize vertex buffer object.
	glGenBuffers(1, &object_VBO[object_ID]);

	glBindBuffer(GL_ARRAY_BUFFER, object_VBO[object_ID]);
	glBufferData(GL_ARRAY_BUFFER, object_n_triangles[object_ID]*3*n_bytes_per_vertex,
		object_vertices[object_ID], GL_STATIC_DRAW);

	// As the geometry data exists now in graphics memory, ...
	free(object_vertices[object_ID]);

	// Initialize vertex array object.
	glGenVertexArrays(1, &object_VAO[object_ID]);
	glBindVertexArray(object_VAO[object_ID]);

	glBindBuffer(GL_ARRAY_BUFFER, object_VBO[object_ID]);
	glVertexAttribPointer(INDEX_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(INDEX_VERTEX_POSITION);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void prepare_square(void) {
	// 8 = 3 for vertex, 3 for normal, and 2 for texcoord
	set_up_object(OBJECT_SQUARE16, "Data/Square16_triangles_vnt.geom", 8 * sizeof(float));
}

void prepare_tiger(void) {
	// 8 = 3 for vertex, 3 for normal, and 2 for texcoord
	set_up_object(OBJECT_TIGER, "Data/Tiger_00_triangles_vnt.geom", 8 * sizeof(float));
}

void prepare_cow(void) {
	// 3 = 3 for vertex
	set_up_object(OBJECT_COW, "Data/Cow_triangles_v.geom", 3 * sizeof(float));
}

void draw_object(int object_ID, float r, float g, float b) {
	glUniform3f(loc_primitive_color, r, g, b); 
	glBindVertexArray(object_VAO[object_ID]);
	glDrawArrays(GL_TRIANGLES, 0, 3 * object_n_triangles[object_ID]);
	glBindVertexArray(0);
}
// End of geometry setup

// Begin of callback function definitions
#define TO_RADIAN 0.01745329252f  
#define TO_DEGREE 57.295779513f

// include glm/*.hpp only if necessary
// #include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, lookAt, perspective, etc.
// ViewProjectionMatrix = ProjectionMatrix * ViewMatrix
glm::mat4 ViewProjectionMatrix, ViewMatrix, ProjectionMatrix;
// ModelViewProjectionMatrix = ProjectionMatrix * ViewMatrix * ModelMatrix
glm::mat4 ModelViewProjectionMatrix; // This one is sent to vertex shader when ready.

float rotation_angle_cow;

struct {
	glm::vec3 prp, vrp, vup;
	float aspect_ratio, zoom_factor;
} camera;

enum { 
	VIEW_WORLD, VIEW_SQUARE, VIEW_TIGER, VIEW_COW 
} view_mode;

void display(void) { 
	// should optimize this callback function further to reduce the amount of floating-point operations.
	glm::mat4 Matrix_TIGER_tmp;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// At this point, the matrix ViewProjectionMatrix has been properly set up.
	switch (view_mode) {
	case VIEW_WORLD:
		ModelViewProjectionMatrix = glm::scale(ViewProjectionMatrix, glm::vec3(5.0f, 5.0f, 5.0f));
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		glLineWidth(5.0f);
		draw_axes(); // draw the WC axes.
		glLineWidth(1.0f);

		ModelViewProjectionMatrix = glm::translate(ViewProjectionMatrix, glm::vec3(-6.0f, -0.01f, -6.0f));
		ModelViewProjectionMatrix = glm::scale(ModelViewProjectionMatrix, glm::vec3(12.0f, 12.0f, 12.0f));
		ModelViewProjectionMatrix = glm::rotate(ModelViewProjectionMatrix,
			90.0f*TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		draw_object(OBJECT_SQUARE16, 72 / 255.0f, 201 / 255.0f, 176 / 255.0f); //  draw the floor.
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		// set up this matrix only once outside this callback function if it is static.
		Matrix_TIGER_tmp = glm::rotate(glm::mat4(1.0f), -90.0f*TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
		Matrix_TIGER_tmp = glm::scale(Matrix_TIGER_tmp, glm::vec3(0.01f, 0.01f, 0.01f));

		ModelViewProjectionMatrix = glm::translate(ViewProjectionMatrix, glm::vec3(-2.0f, 0.0f, -3.5f));
		ModelViewProjectionMatrix = glm::rotate(ModelViewProjectionMatrix, 30.0f*TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
		ModelViewProjectionMatrix *= Matrix_TIGER_tmp;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_object(OBJECT_TIGER, 255 / 255.0f, 255 / 255.0f, 255 / 255.0f); // White

		ModelViewProjectionMatrix = glm::translate(ViewProjectionMatrix, glm::vec3(0.0f, 0.0f, -3.5f));
		ModelViewProjectionMatrix *= Matrix_TIGER_tmp;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_object(OBJECT_TIGER, 0 / 255.0f, 0 / 255.0f, 255 / 255.0f); // Blue

		ModelViewProjectionMatrix = glm::translate(ViewProjectionMatrix, glm::vec3(0.0f, 0.0f, 4.25f));
		ModelViewProjectionMatrix = glm::rotate(ModelViewProjectionMatrix, 180.0f*TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
		ModelViewProjectionMatrix *= Matrix_TIGER_tmp;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_object(OBJECT_TIGER, 0 / 255.0f, 255 / 255.0f, 255 / 255.0f); // Cyan

		ModelViewProjectionMatrix = glm::translate(ViewProjectionMatrix, glm::vec3(-3.0f, 0.0f, 2.0f));
		ModelViewProjectionMatrix = glm::rotate(ModelViewProjectionMatrix, 90.0f*TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
		ModelViewProjectionMatrix *= Matrix_TIGER_tmp;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_object(OBJECT_TIGER, 255 / 255.0f, 0 / 255.0f, 255 / 255.0f); // Magenta

		ModelViewProjectionMatrix = glm::translate(ViewProjectionMatrix, glm::vec3(2.0f, 0.0f, -3.0f));
		ModelViewProjectionMatrix = glm::rotate(ModelViewProjectionMatrix, -45.0f*TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
		ModelViewProjectionMatrix *= Matrix_TIGER_tmp;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_object(OBJECT_TIGER, 0 / 255.0f, 255 / 255.0f, 0 / 255.0f); // Green

		ModelViewProjectionMatrix = glm::translate(ViewProjectionMatrix, glm::vec3(3.0f, 0.0f, 2.0f));
		ModelViewProjectionMatrix = glm::rotate(ModelViewProjectionMatrix, -135.0f*TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
		ModelViewProjectionMatrix *= Matrix_TIGER_tmp;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_object(OBJECT_TIGER, 255 / 255.0f, 0 / 255.0f, 0 / 255.0f); // Red

		ModelViewProjectionMatrix = glm::rotate(ViewProjectionMatrix,
			2 * rotation_angle_cow*TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
		ModelViewProjectionMatrix = glm::translate(ModelViewProjectionMatrix, glm::vec3(1.25f, 0.645f, 0.0f));
		ModelViewProjectionMatrix = glm::rotate(ModelViewProjectionMatrix, 
			20 * rotation_angle_cow*TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
		ModelViewProjectionMatrix = glm::scale(ModelViewProjectionMatrix, glm::vec3(2.0f, 2.0f, 2.0f));
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_object(OBJECT_COW, 255 / 255.0f, 255 / 255.0f, 0 / 255.0f); //  Yellow
		break;
	case VIEW_SQUARE:
		ModelViewProjectionMatrix = ViewProjectionMatrix; // ModelMatrix = I
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_object(OBJECT_SQUARE16, 255 / 255.0f, 255 / 255.0f, 255 / 255.0f);    
		glLineWidth(3.0f);
		draw_axes();
		glLineWidth(1.0f);
		break;
	case VIEW_TIGER:
		ModelViewProjectionMatrix = ViewProjectionMatrix; // ModelMatrix = I
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		glLineWidth(3.0f);
		draw_axes();
		glLineWidth(1.0f);
		// the TIGER model is too big ...
		ModelViewProjectionMatrix = glm::scale(ViewProjectionMatrix, glm::vec3(0.01f, 0.01f, 0.01f));
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_object(OBJECT_TIGER, 255 / 255.0f, 255 / 255.0f, 255 / 255.0f);  
		break;
	case VIEW_COW:
		ModelViewProjectionMatrix = ViewProjectionMatrix; // ModelMatrix = I
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_object(OBJECT_COW, 255 / 255.0f, 255 / 255.0f, 255 / 255.0f);  
		glLineWidth(3.0f);
		draw_axes();
		glLineWidth(1.0f);
		break;
	}
	glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
	if (key == 27) { // ESC key
		glutLeaveMainLoop(); // incur destuction callback for cleanups.
	}
	switch (key) {
	case 'w':
		view_mode = VIEW_WORLD;
		break;
	case 's':
		view_mode = VIEW_SQUARE;
		break;
	case 't':
		view_mode = VIEW_TIGER;
		break;
	case 'c':
		view_mode = VIEW_COW;
		break;
	case 'o': // initialize the camera position and orientation
		camera.prp = glm::vec3(25.0f, 25.0f, 25.0f);
		camera.vrp = glm::vec3(0.0f, 0.0f, 0.0f);
		camera.vup = glm::vec3(0.0f, 1.0f, 0.0f);
		ViewMatrix = glm::lookAt(camera.prp, camera.vrp, camera.vup);
		ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		break;
	case 'x':
		camera.vrp = glm::vec3(5.0f, 0.0f, 0.0f);
		ViewMatrix = glm::lookAt(camera.prp, camera.vrp, camera.vup);
		ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		break;
	case 'y':
		camera.vrp = glm::vec3(0.0f, 5.0f, 0.0f);
		ViewMatrix = glm::lookAt(camera.prp, camera.vrp, camera.vup);
		ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		break;
	case 'z':
		camera.vrp = glm::vec3(0.0f, 0.0f, 5.0f);
		ViewMatrix = glm::lookAt(camera.prp, camera.vrp, camera.vup);
		ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		break;
	}
	glutPostRedisplay();
}

void  special(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_UP:
		camera.prp.y += 1.0f;
		if (camera.prp.y > 50.0f) camera.prp.y = 50.0f;
		ViewMatrix = glm::lookAt(camera.prp, camera.vrp, camera.vup);
		ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		break;
	case GLUT_KEY_DOWN:
		camera.prp.y -= 1.0f;
		if (camera.prp.y < 0.0f) camera.prp.y = 0.0f;
		ViewMatrix = glm::lookAt(camera.prp, camera.vrp, camera.vup);
		ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		break;
	}
	glutPostRedisplay();
}

void mousepress(int button, int state, int x, int y)  {
	if (state == GLUT_DOWN) { 
		if (button == GLUT_LEFT_BUTTON) {
			camera.zoom_factor *= 1.05f;
			if (camera.zoom_factor > 6.0f)
				camera.zoom_factor = 6.0f;
		}
		else if (button == GLUT_RIGHT_BUTTON) {
			camera.zoom_factor *= 0.95f;
			if (camera.zoom_factor < 0.1f)
				camera.zoom_factor = 0.1f;
		}
		ProjectionMatrix = glm::perspective(camera.zoom_factor*15.0f*TO_RADIAN, camera.aspect_ratio, 1.0f, 1000.0f);
		ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		glutPostRedisplay();
	}
}

void reshape(int width, int height) {
	glViewport(0, 0, width, height);
	
	camera.aspect_ratio = (float)width / height;
	ProjectionMatrix = glm::perspective(camera.zoom_factor*15.0f*TO_RADIAN, camera.aspect_ratio, 1.0f, 1000.0f);
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
	glutPostRedisplay();
}

void timer_scene(int timestamp_scene) {
	rotation_angle_cow = (float) (timestamp_scene % 360);
	glutPostRedisplay();
	glutTimerFunc(100, timer_scene, (timestamp_scene + 1) % INT_MAX);
}

void cleanup(void) {
	glDeleteVertexArrays(1, &axes_VAO);
	glDeleteBuffers(1, &axes_VBO);

	glDeleteVertexArrays(N_OBJECTS, object_VAO);
	glDeleteBuffers(N_OBJECTS, object_VBO);
}
// End of callback function definitions

void register_callbacks(void) {
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutMouseFunc(mousepress);
	glutReshapeFunc(reshape);
	glutTimerFunc(100, timer_scene, 0);
	glutCloseFunc(cleanup);
}

void initialize_OpenGL(void) {
	camera.prp = glm::vec3(25.0f, 25.0f, 25.0f);
	camera.vrp = glm::vec3(0.0f, 0.0f, 0.0f);
	camera.vup = glm::vec3(0.0f, 1.0f, 0.0f);
	ViewMatrix = glm::lookAt(camera.prp, camera.vrp, camera.vup);

	camera.zoom_factor = 1.0f; 
	camera.aspect_ratio = 1.0f; // will be set up properly when the window is popped up.

	rotation_angle_cow = 0.0f;
	view_mode = VIEW_WORLD;

	glClearColor(40 / 255.0f, 116 / 255.0f, 166 / 255.0f, 1.0f);  
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glEnable(GL_DEPTH_TEST);
}

void prepare_scene(void) {
	prepare_axes();
	prepare_square();
	prepare_tiger();
	prepare_cow();
}

void initialize_renderer(void) {
	register_callbacks();
	prepare_shader_program();
	initialize_OpenGL();
	prepare_scene();
}

void initialize_glew(void) {
	GLenum error;

	glewExperimental = GL_TRUE;
	error = glewInit();
	if (error != GLEW_OK) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(error));
		exit(-1);
	}
	fprintf(stdout, "*********************************************************\n");
	fprintf(stdout, " - GLEW version supported: %s\n", glewGetString(GLEW_VERSION));
	fprintf(stdout, " - OpenGL renderer: %s\n", glGetString(GL_RENDERER));
	fprintf(stdout, " - OpenGL version supported: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "*********************************************************\n\n");
}

void print_message(const char * m) {
	fprintf(stdout, "%s\n\n", m);
}

void greetings(char *program_name, char messages[][256], int n_message_lines) {
	fprintf(stdout, "**************************************************************\n\n");
	fprintf(stdout, "  PROGRAM NAME: %s\n\n", program_name);
	fprintf(stdout, "    This program was coded for CSE4170 students\n");
	fprintf(stdout, "      of Dept. of Comp. Sci. & Eng., Sogang University.\n\n");

	for (int i = 0; i < n_message_lines; i++)
		fprintf(stdout, "%s\n", messages[i]);
	fprintf(stdout, "\n**************************************************************\n\n");

	initialize_glew();
}

#define N_MESSAGE_LINES 2
void main(int argc, char *argv[]) {
	char program_name[64] = "Sogang CSE4170 (4.5.1) Simple Camera Transformation";
	char messages[N_MESSAGE_LINES][256] = { "    - Keys used: 'w', 's', 't', 'c', 'o', 'x', 'y', 'z', 'ESC', KEY-UP, KEY-DOWN",
		"    - Mouse used: Left/Right Butten Click"
	};

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(1200, 800);
	glutInitContextVersion(4, 0);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
}
