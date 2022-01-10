#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <FreeImage/FreeImage.h>

#include "Shaders/LoadShaders.h"
#include "My_Shading.h"
GLuint h_ShaderProgram_simple, h_ShaderProgram_TXPS; // handles to shader programs

// for simple shaders
GLint loc_ModelViewProjectionMatrix_simple, loc_primitive_color;

// for Phong Shading (Textured) shaders
#define NUMBER_OF_LIGHT_SUPPORTED 4 
GLint loc_global_ambient_color;
loc_light_Parameters loc_light[NUMBER_OF_LIGHT_SUPPORTED];
loc_Material_Parameters loc_material;
GLint loc_ModelViewProjectionMatrix_TXPS, loc_ModelViewMatrix_TXPS, loc_ModelViewMatrixInvTrans_TXPS;
GLint loc_texture, loc_flag_texture_mapping, loc_flag_fog;

// include glm/*.hpp only if necessary
//#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, lookAt, perspective, etc.
#include <glm/gtc/matrix_inverse.hpp> // inverseTranspose, etc.
glm::mat4 ModelViewProjectionMatrix, ModelViewMatrix;
glm::mat3 ModelViewMatrixInvTrans;
glm::mat4 tmpMatrix, rotateTmpMatrix;
glm::mat4 ViewMatrix, ProjectionMatrix, ViewProjectionMatrix;

#define TO_RADIAN 0.01745329252f  
#define TO_DEGREE 57.295779513f
#define BUFFER_OFFSET(offset) ((GLvoid *) (offset))

#define LOC_VERTEX 0
#define LOC_NORMAL 1
#define LOC_TEXCOORD 2

// lights in scene
Light_Parameters light[NUMBER_OF_LIGHT_SUPPORTED];

// texture stuffs
#define N_TEXTURES_USED 10
#define TEXTURE_ID_FLOOR 0
#define TEXTURE_ID_TIGER 1
#define TEXTURE_ID_GODZILLA 2
#define TEXTURE_ID_TANK 3
GLuint texture_names[N_TEXTURES_USED];
int flag_texture_mapping;
static int cameraNumber = 4;
int count = 0;

int centerX = 0, centerY = 0;

typedef struct _Camera {
	float pos[3];
	float uaxis[3], vaxis[3], naxis[3];
	float fovy, aspect_ratio, near_c, far_c;
	int move;
} Camera;

float point5_x = 0.0f, point5_y = 0.0f, point5_z = 0.0f;

Camera camera1;
Camera camera5;

//camera 1
void set_ViewMatrix_from_camera_frame1(void) {
	ViewMatrix = glm::mat4(camera1.uaxis[0], camera1.vaxis[0], camera1.naxis[0], 0.0f,
		camera1.uaxis[1], camera1.vaxis[1], camera1.naxis[1], 0.0f,
		camera1.uaxis[2], camera1.vaxis[2], camera1.naxis[2], 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	ViewMatrix = glm::translate(ViewMatrix, glm::vec3(-camera1.pos[0], -camera1.pos[1], -camera1.pos[2]));
}

void initialize_camera1(void) {
	camera1.pos[0] = 0.0f; camera1.pos[1] = 1500.0f;  camera1.pos[2] = 0.0f;
	camera1.uaxis[2] = camera1.uaxis[1] = 0.0f; camera1.uaxis[0] = 1.0f;
	camera1.vaxis[0] = camera1.vaxis[1] = 0.0f; camera1.vaxis[2] = 1.0f;
	camera1.naxis[0] = camera1.naxis[2] = 0.0f; camera1.naxis[1] = 1.0f;

	camera1.move = 0;
	camera1.fovy = 30.0f, camera1.aspect_ratio = 1.0f; camera1.near_c = 0.1f; camera1.far_c = 1000.0f;

	set_ViewMatrix_from_camera_frame1();
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
}

//camera 5
void set_ViewMatrix_from_camera_frame5(void) {
	ViewMatrix = glm::mat4(camera5.uaxis[0], camera5.vaxis[0], camera5.naxis[0], 0.0f,
		camera5.uaxis[1], camera5.vaxis[1], camera5.naxis[1], 0.0f,
		camera5.uaxis[2], camera5.vaxis[2], camera5.naxis[2], 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	ViewMatrix = glm::translate(ViewMatrix, glm::vec3(-camera5.pos[0], -camera5.pos[1], -camera5.pos[2]));
}


void initialize_camera5(void) {
	camera5.pos[0] = 1000.0f; camera5.pos[1] = 300.0f;  camera5.pos[2] = 100.0f;
	camera5.uaxis[0] = camera5.uaxis[1] = 0.0f; camera5.uaxis[2] = -1.0f;
	camera5.vaxis[0] = camera5.vaxis[2] = 0.0f; camera5.vaxis[1] = 1.0f;
	camera5.naxis[1] = camera5.naxis[2] = 0.0f; camera5.naxis[0] = 1.0f;

	camera5.move = 0;
	camera5.fovy = 30.0f, camera5.aspect_ratio = 1.0f; camera5.near_c = 0.1f; camera5.far_c = 1000.0f;

	set_ViewMatrix_from_camera_frame5();
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
}

void My_glTexImage2D_from_file(char *filename) {
	FREE_IMAGE_FORMAT tx_file_format;
	int tx_bits_per_pixel;
	FIBITMAP *tx_pixmap, *tx_pixmap_32;

	int width, height;
	GLvoid *data;
	
	tx_file_format = FreeImage_GetFileType(filename, 0);
	// assume everything is fine with reading texture from file: no error checking
	tx_pixmap = FreeImage_Load(tx_file_format, filename);
	tx_bits_per_pixel = FreeImage_GetBPP(tx_pixmap);

	fprintf(stdout, " * A %d-bit texture was read from %s.\n", tx_bits_per_pixel, filename);
	if (tx_bits_per_pixel == 32)
		tx_pixmap_32 = tx_pixmap;
	else {
		fprintf(stdout, " * Converting texture from %d bits to 32 bits...\n", tx_bits_per_pixel);
		tx_pixmap_32 = FreeImage_ConvertTo32Bits(tx_pixmap);
	}

	width = FreeImage_GetWidth(tx_pixmap_32);
	height = FreeImage_GetHeight(tx_pixmap_32);
	data = FreeImage_GetBits(tx_pixmap_32);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
	fprintf(stdout, " * Loaded %dx%d RGBA texture into graphics memory.\n\n", width, height);

	FreeImage_Unload(tx_pixmap_32);
	if (tx_bits_per_pixel != 32)
		FreeImage_Unload(tx_pixmap);
}

// fog stuffs
// you could control the fog parameters interactively: FOG_COLOR, FOG_NEAR_DISTANCE, FOG_FAR_DISTANCE   
int flag_fog;
//flags
int tiger_flag = 1;
int spider_flag = 1;
int ben_flag = 1;
int wolf_flag = 1;
int camera5_flag = 1;
int axis = 1;

// for tiger animation
unsigned int timestamp_scene = 0; // the global clock in the scene
unsigned int timestamp_tiger = 0;
unsigned int timestamp_spider = 0;
unsigned int timestamp_ben = 0;
unsigned int timestamp_wolf = 0;

int flag_tiger_animation, flag_polygon_fill;
int cur_frame_tiger = 0, cur_frame_ben = 0, cur_frame_wolf, cur_frame_spider = 0;
float rotation_angle_tiger = 0.0f;
float rotation_angle_wolf = 0.0f;

float location_spider = 0.0f;

float clock_spider = 0.0f;
float clock_ben = 0.0f;
float clock_tiger = 0.0f;
float clock_wolf = 0.0f;
float spider_size = 0.0f;

float camera5_x = 1300.0f;
float camera5_y = 1300.0f;
float camera5_z = 1300.0f;

// axes object
GLuint axes_VBO, axes_VAO;
GLfloat axes_vertices[6][3] = {
	{ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }
};
GLfloat axes_color[3][3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };

void prepare_axes(void) { // draw coordinate axes
	// initialize vertex buffer object
	glGenBuffers(1, &axes_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, axes_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(axes_vertices), &axes_vertices[0][0], GL_STATIC_DRAW);

	// initialize vertex array object
	glGenVertexArrays(1, &axes_VAO);
	glBindVertexArray(axes_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, axes_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

 void draw_axes(void) {
	 // assume ShaderProgram_simple is used
	 glBindVertexArray(axes_VAO);
	 glUniform3fv(loc_primitive_color, 1, axes_color[0]);
	 glDrawArrays(GL_LINES, 0, 2);
	 glUniform3fv(loc_primitive_color, 1, axes_color[1]);
	 glDrawArrays(GL_LINES, 2, 2);
	 glUniform3fv(loc_primitive_color, 1, axes_color[2]);
	 glDrawArrays(GL_LINES, 4, 2);
	 glBindVertexArray(0);
 }

 // floor object
#define TEX_COORD_EXTENT 1.0f
 GLuint rectangle_VBO, rectangle_VAO;
 GLfloat rectangle_vertices[6][8] = {  // vertices enumerated counterclockwise
	 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
	 { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, TEX_COORD_EXTENT, 0.0f },
	 { 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, TEX_COORD_EXTENT, TEX_COORD_EXTENT },
	 { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
	 { 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, TEX_COORD_EXTENT, TEX_COORD_EXTENT },
	 { 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, TEX_COORD_EXTENT }
 };

 Material_Parameters material_floor;


 void prepare_floor(void) { // Draw coordinate axes.
	 // Initialize vertex buffer object.
	 glGenBuffers(1, &rectangle_VBO);

	 glBindBuffer(GL_ARRAY_BUFFER, rectangle_VBO);
	 glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_vertices), &rectangle_vertices[0][0], GL_STATIC_DRAW);

	 // Initialize vertex array object.
	 glGenVertexArrays(1, &rectangle_VAO);
	 glBindVertexArray(rectangle_VAO);

	 glBindBuffer(GL_ARRAY_BUFFER, rectangle_VBO);
	 glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(0));
	 glEnableVertexAttribArray(0);
	 glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(3 * sizeof(float)));
 	 glEnableVertexAttribArray(1);
	 glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(6 * sizeof(float)));
	 glEnableVertexAttribArray(2);

	 glBindBuffer(GL_ARRAY_BUFFER, 0);
	 glBindVertexArray(0);

	 material_floor.ambient_color[0] = 0.0f;
	 material_floor.ambient_color[1] = 0.05f;
	 material_floor.ambient_color[2] = 0.0f;
	 material_floor.ambient_color[3] = 1.0f;

	 material_floor.diffuse_color[0] = 0.2f;
	 material_floor.diffuse_color[1] = 0.5f;
	 material_floor.diffuse_color[2] = 0.2f;
	 material_floor.diffuse_color[3] = 1.0f;

	 material_floor.specular_color[0] = 0.24f;
	 material_floor.specular_color[1] = 0.5f;
	 material_floor.specular_color[2] = 0.24f;
	 material_floor.specular_color[3] = 1.0f;

	 material_floor.specular_exponent = 2.5f;

	 material_floor.emissive_color[0] = 0.0f;
	 material_floor.emissive_color[1] = 0.0f;
	 material_floor.emissive_color[2] = 0.0f;
	 material_floor.emissive_color[3] = 1.0f;

 	 glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	 glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_FLOOR);
	 glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_FLOOR]);

//	 My_glTexImage2D_from_file("Data/static_objects/grass_tex.jpg");
 	 My_glTexImage2D_from_file("Data/static_objects/beach.jpg");

	 glGenerateMipmap(GL_TEXTURE_2D);

 	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

//	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//	 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

//   float border_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
//	 glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
 }

 void set_material_floor(void) {
	 // assume ShaderProgram_TXPS is used
	 glUniform4fv(loc_material.ambient_color, 1, material_floor.ambient_color);
	 glUniform4fv(loc_material.diffuse_color, 1, material_floor.diffuse_color);
	 glUniform4fv(loc_material.specular_color, 1, material_floor.specular_color);
	 glUniform1f(loc_material.specular_exponent, material_floor.specular_exponent);
	 glUniform4fv(loc_material.emissive_color, 1, material_floor.emissive_color);
 }

 void draw_floor(void) {
	 glFrontFace(GL_CCW);

	 glBindVertexArray(rectangle_VAO);
	 glDrawArrays(GL_TRIANGLES, 0, 6);
	 glBindVertexArray(0);
 }

 // tiger object
#define N_TIGER_FRAMES 12
GLuint tiger_VBO, tiger_VAO;
int tiger_n_triangles[N_TIGER_FRAMES];
int tiger_vertex_offset[N_TIGER_FRAMES];
GLfloat *tiger_vertices[N_TIGER_FRAMES];

Material_Parameters material_tiger;

// ben object
#define N_BEN_FRAMES 30
GLuint ben_VBO, ben_VAO;
int ben_n_triangles[N_BEN_FRAMES];
int ben_vertex_offset[N_BEN_FRAMES];
GLfloat *ben_vertices[N_BEN_FRAMES];

Material_Parameters material_ben;

// wolf object
#define N_WOLF_FRAMES 17
GLuint wolf_VBO, wolf_VAO;
int wolf_n_triangles[N_WOLF_FRAMES];
int wolf_vertex_offset[N_WOLF_FRAMES];
GLfloat *wolf_vertices[N_WOLF_FRAMES];

Material_Parameters material_wolf;

// spider object
#define N_SPIDER_FRAMES 16
GLuint spider_VBO, spider_VAO;
int spider_n_triangles[N_SPIDER_FRAMES];
int spider_vertex_offset[N_SPIDER_FRAMES];
GLfloat *spider_vertices[N_SPIDER_FRAMES];

Material_Parameters material_spider;

// dragon object
GLuint dragon_VBO, dragon_VAO;
int dragon_n_triangles;
GLfloat *dragon_vertices;

Material_Parameters material_dragon;

// optimus object
GLuint optimus_VBO, optimus_VAO;
int optimus_n_triangles;
GLfloat *optimus_vertices;

Material_Parameters material_optimus;

// cow object
GLuint cow_VBO, cow_VAO;
int cow_n_triangles;
GLfloat *cow_vertices;

Material_Parameters material_cow;

// bike object
GLuint bike_VBO, bike_VAO;
int bike_n_triangles;
GLfloat *bike_vertices;

Material_Parameters material_bike;

// bus object
GLuint bus_VBO, bus_VAO;
int bus_n_triangles;
GLfloat *bus_vertices;

Material_Parameters material_bus;

// godzilla object
GLuint godzilla_VBO, godzilla_VAO;
int godzilla_n_triangles;
GLfloat *godzilla_vertices;

Material_Parameters material_godzilla;

// ironman object
GLuint ironman_VBO, ironman_VAO;
int ironman_n_triangles;
GLfloat *ironman_vertices;

Material_Parameters material_ironman;

// tank object
GLuint tank_VBO, tank_VAO;
int tank_n_triangles;
GLfloat *tank_vertices;

Material_Parameters material_tank;



int read_geometry(GLfloat **object, int bytes_per_primitive, char *filename) {
	int n_triangles;
	FILE *fp;

	// fprintf(stdout, "Reading geometry from the geometry file %s...\n", filename);
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
	// fprintf(stdout, "Read %d primitives successfully.\n\n", n_triangles);
	fclose(fp);

	return n_triangles;
}

void prepare_wolf(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, wolf_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_WOLF_FRAMES; i++) {
		sprintf(filename, "Data/dynamic_objects/wolf/wolf_%02d_vnt.geom", i);
		wolf_n_triangles[i] = read_geometry(&wolf_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		wolf_n_total_triangles += wolf_n_triangles[i];

		if (i == 0)
			wolf_vertex_offset[i] = 0;
		else
			wolf_vertex_offset[i] = wolf_vertex_offset[i - 1] + 3 * wolf_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &wolf_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, wolf_VBO);
	glBufferData(GL_ARRAY_BUFFER, wolf_n_total_triangles*n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_WOLF_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, wolf_vertex_offset[i] * n_bytes_per_vertex,
			wolf_n_triangles[i] * n_bytes_per_triangle, wolf_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_WOLF_FRAMES; i++)
		free(wolf_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &wolf_VAO);
	glBindVertexArray(wolf_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, wolf_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//material_wolf.ambient_color[0] = 0.24725f;
	//material_wolf.ambient_color[1] = 0.1995f;
	//material_wolf.ambient_color[2] = 0.0745f;
	//material_wolf.ambient_color[3] = 1.0f;
	//
	//material_wolf.diffuse_color[0] = 0.75164f;
	//material_wolf.diffuse_color[1] = 0.60648f;
	//material_wolf.diffuse_color[2] = 0.22648f;
	//material_wolf.diffuse_color[3] = 1.0f;
	//
	//material_wolf.specular_color[0] = 0.728281f;
	//material_wolf.specular_color[1] = 0.655802f;
	//material_wolf.specular_color[2] = 0.466065f;
	//material_wolf.specular_color[3] = 1.0f;
	//
	//material_wolf.specular_exponent = 51.2f;
	//
	//material_wolf.emissive_color[0] = 0.1f;
	//material_wolf.emissive_color[1] = 0.1f;
	//material_wolf.emissive_color[2] = 0.0f;
	//material_wolf.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
void prepare_ben(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, ben_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_BEN_FRAMES; i++) {
		sprintf(filename, "Data/dynamic_objects/ben/ben_vn%d%d.geom", i / 10, i % 10);
		ben_n_triangles[i] = read_geometry(&ben_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		ben_n_total_triangles += ben_n_triangles[i];

		if (i == 0)
			ben_vertex_offset[i] = 0;
		else
			ben_vertex_offset[i] = ben_vertex_offset[i - 1] + 3 * ben_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &ben_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, ben_VBO);
	glBufferData(GL_ARRAY_BUFFER, ben_n_total_triangles*n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_BEN_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, ben_vertex_offset[i] * n_bytes_per_vertex,
			ben_n_triangles[i] * n_bytes_per_triangle, ben_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_BEN_FRAMES; i++)
		free(ben_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &ben_VAO);
	glBindVertexArray(ben_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, ben_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//material_ben.ambient_color[0] = 0.24725f;
	//material_ben.ambient_color[1] = 0.1995f;
	//material_ben.ambient_color[2] = 0.0745f;
	//material_ben.ambient_color[3] = 1.0f;
	//
	//material_ben.diffuse_color[0] = 0.75164f;
	//material_ben.diffuse_color[1] = 0.60648f;
	//material_ben.diffuse_color[2] = 0.22648f;
	//material_ben.diffuse_color[3] = 1.0f;
	//
	//material_ben.specular_color[0] = 0.728281f;
	//material_ben.specular_color[1] = 0.655802f;
	//material_ben.specular_color[2] = 0.466065f;
	//material_ben.specular_color[3] = 1.0f;
	//
	//material_ben.specular_exponent = 51.2f;
	//
	//material_ben.emissive_color[0] = 0.1f;
	//material_ben.emissive_color[1] = 0.1f;
	//material_ben.emissive_color[2] = 0.0f;
	//material_ben.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
void prepare_spider(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, spider_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_SPIDER_FRAMES; i++) {
		sprintf(filename, "Data/dynamic_objects/spider/spider_vnt_%d%d.geom", i / 10, i % 10);
		spider_n_triangles[i] = read_geometry(&spider_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		spider_n_total_triangles += spider_n_triangles[i];

		if (i == 0)
			spider_vertex_offset[i] = 0;
		else
			spider_vertex_offset[i] = spider_vertex_offset[i - 1] + 3 * spider_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &spider_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, spider_VBO);
	glBufferData(GL_ARRAY_BUFFER, spider_n_total_triangles*n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_SPIDER_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, spider_vertex_offset[i] * n_bytes_per_vertex,
			spider_n_triangles[i] * n_bytes_per_triangle, spider_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_SPIDER_FRAMES; i++)
		free(spider_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &spider_VAO);
	glBindVertexArray(spider_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, spider_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	material_spider.ambient_color[0] = 0.24725f;
	material_spider.ambient_color[1] = 0.1995f;
	material_spider.ambient_color[2] = 0.0745f;
	material_spider.ambient_color[3] = 1.0f;
	//
	//material_spider.diffuse_color[0] = 0.75164f;
	//material_spider.diffuse_color[1] = 0.60648f;
	//material_spider.diffuse_color[2] = 0.22648f;
	//material_spider.diffuse_color[3] = 1.0f;
	//
	//material_spider.specular_color[0] = 0.728281f;
	//material_spider.specular_color[1] = 0.655802f;
	//material_spider.specular_color[2] = 0.466065f;
	//material_spider.specular_color[3] = 1.0f;
	//
	//material_spider.specular_exponent = 51.2f;
	//
	//material_spider.emissive_color[0] = 0.1f;
	//material_spider.emissive_color[1] = 0.1f;
	//material_spider.emissive_color[2] = 0.0f;
	//material_spider.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
void prepare_tiger(void) { // vertices enumerated clockwise
	int i, n_bytes_per_vertex, n_bytes_per_triangle, tiger_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_TIGER_FRAMES; i++) {
		sprintf(filename, "Data/dynamic_objects/tiger/Tiger_%d%d_triangles_vnt.geom", i / 10, i % 10);
		tiger_n_triangles[i] = read_geometry(&tiger_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		tiger_n_total_triangles += tiger_n_triangles[i];

		if (i == 0)
			tiger_vertex_offset[i] = 0;
		else
			tiger_vertex_offset[i] = tiger_vertex_offset[i - 1] + 3 * tiger_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &tiger_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
	glBufferData(GL_ARRAY_BUFFER, tiger_n_total_triangles*n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_TIGER_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, tiger_vertex_offset[i] * n_bytes_per_vertex,
		tiger_n_triangles[i] * n_bytes_per_triangle, tiger_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_TIGER_FRAMES; i++)
		free(tiger_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &tiger_VAO);
	glBindVertexArray(tiger_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	material_tiger.ambient_color[0] = 0.24725f;
	material_tiger.ambient_color[1] = 0.1995f;
	material_tiger.ambient_color[2] = 0.0745f;
	material_tiger.ambient_color[3] = 1.0f;

	material_tiger.diffuse_color[0] = 0.75164f;
	material_tiger.diffuse_color[1] = 0.60648f;
	material_tiger.diffuse_color[2] = 0.22648f;
	material_tiger.diffuse_color[3] = 1.0f;

	material_tiger.specular_color[0] = 0.728281f;
	material_tiger.specular_color[1] = 0.655802f;
	material_tiger.specular_color[2] = 0.466065f;
	material_tiger.specular_color[3] = 1.0f;

	material_tiger.specular_exponent = 51.2f;

	material_tiger.emissive_color[0] = 0.1f;
	material_tiger.emissive_color[1] = 0.1f;
	material_tiger.emissive_color[2] = 0.0f;
	material_tiger.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
void prepare_dragon(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, dragon_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/dragon_vnt.geom");
	dragon_n_triangles = read_geometry(&dragon_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	dragon_n_total_triangles += dragon_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &dragon_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, dragon_VBO);
	glBufferData(GL_ARRAY_BUFFER, dragon_n_total_triangles * 3 * n_bytes_per_vertex, dragon_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(dragon_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &dragon_VAO);
	glBindVertexArray(dragon_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, dragon_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//material_dragon.ambient_color[0] = 0.24725f;
	//material_dragon.ambient_color[1] = 0.1995f;
	//material_dragon.ambient_color[2] = 0.0745f;
	//material_dragon.ambient_color[3] = 1.0f;
	//
	//material_dragon.diffuse_color[0] = 0.75164f;
	//material_dragon.diffuse_color[1] = 0.60648f;
	//material_dragon.diffuse_color[2] = 0.22648f;
	//material_dragon.diffuse_color[3] = 1.0f;
	//
	//material_dragon.specular_color[0] = 0.728281f;
	//material_dragon.specular_color[1] = 0.655802f;
	//material_dragon.specular_color[2] = 0.466065f;
	//material_dragon.specular_color[3] = 1.0f;
	//
	//material_dragon.specular_exponent = 51.2f;
	//
	//material_dragon.emissive_color[0] = 0.1f;
	//material_dragon.emissive_color[1] = 0.1f;
	//material_dragon.emissive_color[2] = 0.0f;
	//material_dragon.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
void prepare_optimus(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, optimus_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/optimus_vnt.geom");
	optimus_n_triangles = read_geometry(&optimus_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	optimus_n_total_triangles += optimus_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &optimus_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, optimus_VBO);
	glBufferData(GL_ARRAY_BUFFER, optimus_n_total_triangles * 3 * n_bytes_per_vertex, optimus_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(optimus_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &optimus_VAO);
	glBindVertexArray(optimus_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, optimus_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//material_optimus.ambient_color[0] = 0.24725f;
	//material_optimus.ambient_color[1] = 0.1995f;
	//material_optimus.ambient_color[2] = 0.0745f;
	//material_optimus.ambient_color[3] = 1.0f;
	//
	//material_optimus.diffuse_color[0] = 0.75164f;
	//material_optimus.diffuse_color[1] = 0.60648f;
	//material_optimus.diffuse_color[2] = 0.22648f;
	//material_optimus.diffuse_color[3] = 1.0f;
	//
	//material_optimus.specular_color[0] = 0.728281f;
	//material_optimus.specular_color[1] = 0.655802f;
	//material_optimus.specular_color[2] = 0.466065f;
	//material_optimus.specular_color[3] = 1.0f;
	//
	//material_optimus.specular_exponent = 51.2f;
	//
	//material_optimus.emissive_color[0] = 0.1f;
	//material_optimus.emissive_color[1] = 0.1f;
	//material_optimus.emissive_color[2] = 0.0f;
	//material_optimus.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
void prepare_cow(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, cow_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/cow_vn.geom");
	cow_n_triangles = read_geometry(&cow_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	cow_n_total_triangles += cow_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &cow_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, cow_VBO);
	glBufferData(GL_ARRAY_BUFFER, cow_n_total_triangles * 3 * n_bytes_per_vertex, cow_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(cow_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &cow_VAO);
	glBindVertexArray(cow_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, cow_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//material_cow.ambient_color[0] = 0.24725f;
	//material_cow.ambient_color[1] = 0.1995f;
	//material_cow.ambient_color[2] = 0.0745f;
	//material_cow.ambient_color[3] = 1.0f;
	//
	//material_cow.diffuse_color[0] = 0.75164f;
	//material_cow.diffuse_color[1] = 0.60648f;
	//material_cow.diffuse_color[2] = 0.22648f;
	//material_cow.diffuse_color[3] = 1.0f;
	//
	//material_cow.specular_color[0] = 0.728281f;
	//material_cow.specular_color[1] = 0.655802f;
	//material_cow.specular_color[2] = 0.466065f;
	//material_cow.specular_color[3] = 1.0f;
	//
	//material_cow.specular_exponent = 51.2f;
	//
	//material_cow.emissive_color[0] = 0.1f;
	//material_cow.emissive_color[1] = 0.1f;
	//material_cow.emissive_color[2] = 0.0f;
	//material_cow.emissive_color[3] = 1.0f;
}
void prepare_bike(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, bike_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/bike_vnt.geom");
	bike_n_triangles = read_geometry(&bike_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	bike_n_total_triangles += bike_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &bike_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, bike_VBO);
	glBufferData(GL_ARRAY_BUFFER, bike_n_total_triangles * 3 * n_bytes_per_vertex, bike_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(bike_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &bike_VAO);
	glBindVertexArray(bike_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, bike_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//material_bike.ambient_color[0] = 0.24725f;
	//material_bike.ambient_color[1] = 0.1995f;
	//material_bike.ambient_color[2] = 0.0745f;
	//material_bike.ambient_color[3] = 1.0f;
	//
	//material_bike.diffuse_color[0] = 0.75164f;
	//material_bike.diffuse_color[1] = 0.60648f;
	//material_bike.diffuse_color[2] = 0.22648f;
	//material_bike.diffuse_color[3] = 1.0f;
	//
	//material_bike.specular_color[0] = 0.728281f;
	//material_bike.specular_color[1] = 0.655802f;
	//material_bike.specular_color[2] = 0.466065f;
	//material_bike.specular_color[3] = 1.0f;
	//
	//material_bike.specular_exponent = 51.2f;
	//
	//material_bike.emissive_color[0] = 0.1f;
	//material_bike.emissive_color[1] = 0.1f;
	//material_bike.emissive_color[2] = 0.0f;
	//material_bike.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
void prepare_bus(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, bus_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/bus_vnt.geom");
	bus_n_triangles = read_geometry(&bus_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	bus_n_total_triangles += bus_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &bus_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, bus_VBO);
	glBufferData(GL_ARRAY_BUFFER, bus_n_total_triangles * 3 * n_bytes_per_vertex, bus_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(bus_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &bus_VAO);
	glBindVertexArray(bus_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, bus_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//material_bus.ambient_color[0] = 0.24725f;
	//material_bus.ambient_color[1] = 0.1995f;
	//material_bus.ambient_color[2] = 0.0745f;
	//material_bus.ambient_color[3] = 1.0f;
	//
	//material_bus.diffuse_color[0] = 0.75164f;
	//material_bus.diffuse_color[1] = 0.60648f;
	//material_bus.diffuse_color[2] = 0.22648f;
	//material_bus.diffuse_color[3] = 1.0f;
	//
	//material_bus.specular_color[0] = 0.728281f;
	//material_bus.specular_color[1] = 0.655802f;
	//material_bus.specular_color[2] = 0.466065f;
	//material_bus.specular_color[3] = 1.0f;
	//
	//material_bus.specular_exponent = 51.2f;
	//
	//material_bus.emissive_color[0] = 0.1f;
	//material_bus.emissive_color[1] = 0.1f;
	//material_bus.emissive_color[2] = 0.0f;
	//material_bus.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
void prepare_godzilla(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, godzilla_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/godzilla_vnt.geom");
	godzilla_n_triangles = read_geometry(&godzilla_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	godzilla_n_total_triangles += godzilla_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &godzilla_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, godzilla_VBO);
	glBufferData(GL_ARRAY_BUFFER, godzilla_n_total_triangles * 3 * n_bytes_per_vertex, godzilla_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(godzilla_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &godzilla_VAO);
	glBindVertexArray(godzilla_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, godzilla_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	material_godzilla.ambient_color[0] = 0.24725f;
	material_godzilla.ambient_color[1] = 0.1995f;
	material_godzilla.ambient_color[2] = 0.0745f;
	material_godzilla.ambient_color[3] = 1.0f;
	//
	material_godzilla.diffuse_color[0] = 0.75164f;
	material_godzilla.diffuse_color[1] = 0.60648f;
	material_godzilla.diffuse_color[2] = 0.22648f;
	material_godzilla.diffuse_color[3] = 1.0f;
	//
	material_godzilla.specular_color[0] = 0.728281f;
	material_godzilla.specular_color[1] = 0.655802f;
	material_godzilla.specular_color[2] = 0.466065f;
	material_godzilla.specular_color[3] = 1.0f;
	//
	material_godzilla.specular_exponent = 51.2f;
	//
	material_godzilla.emissive_color[0] = 0.1f;
	material_godzilla.emissive_color[1] = 0.1f;
	material_godzilla.emissive_color[2] = 0.0f;
	material_godzilla.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_GODZILLA);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_GODZILLA]);

	My_glTexImage2D_from_file("Data/static_objects/godzilla.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
void prepare_ironman(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, ironman_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/ironman_vnt.geom");
	ironman_n_triangles = read_geometry(&ironman_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	ironman_n_total_triangles += ironman_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &ironman_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, ironman_VBO);
	glBufferData(GL_ARRAY_BUFFER, ironman_n_total_triangles * 3 * n_bytes_per_vertex, ironman_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(ironman_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &ironman_VAO);
	glBindVertexArray(ironman_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, ironman_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//material_ironman.ambient_color[0] = 0.24725f;
	//material_ironman.ambient_color[1] = 0.1995f;
	//material_ironman.ambient_color[2] = 0.0745f;
	//material_ironman.ambient_color[3] = 1.0f;
	//
	//material_ironman.diffuse_color[0] = 0.75164f;
	//material_ironman.diffuse_color[1] = 0.60648f;
	//material_ironman.diffuse_color[2] = 0.22648f;
	//material_ironman.diffuse_color[3] = 1.0f;
	//
	//material_ironman.specular_color[0] = 0.728281f;
	//material_ironman.specular_color[1] = 0.655802f;
	//material_ironman.specular_color[2] = 0.466065f;
	//material_ironman.specular_color[3] = 1.0f;
	//
	//material_ironman.specular_exponent = 51.2f;
	//
	//material_ironman.emissive_color[0] = 0.1f;
	//material_ironman.emissive_color[1] = 0.1f;
	//material_ironman.emissive_color[2] = 0.0f;
	//material_ironman.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TIGER);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TIGER]);

	My_glTexImage2D_from_file("Data/dynamic_objects/tiger/tiger_tex2.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
void prepare_tank(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, tank_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/tank_vnt.geom");
	tank_n_triangles = read_geometry(&tank_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	tank_n_total_triangles += tank_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &tank_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, tank_VBO);
	glBufferData(GL_ARRAY_BUFFER, tank_n_total_triangles * 3 * n_bytes_per_vertex, tank_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(tank_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &tank_VAO);
	glBindVertexArray(tank_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, tank_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	material_tank.ambient_color[0] = 0.24725f;
	material_tank.ambient_color[1] = 0.1995f;
	material_tank.ambient_color[2] = 0.0745f;
	material_tank.ambient_color[3] = 1.0f;
	//
	material_tank.diffuse_color[0] = 0.75164f;
	material_tank.diffuse_color[1] = 0.60648f;
	material_tank.diffuse_color[2] = 0.22648f;
	material_tank.diffuse_color[3] = 1.0f;
	//
	material_tank.specular_color[0] = 0.728281f;
	material_tank.specular_color[1] = 0.655802f;
	material_tank.specular_color[2] = 0.466065f;
	material_tank.specular_color[3] = 1.0f;
	//
	material_tank.specular_exponent = 51.2f;
	//
	material_tank.emissive_color[0] = 0.1f;
	material_tank.emissive_color[1] = 0.1f;
	material_tank.emissive_color[2] = 0.0f;
	material_tank.emissive_color[3] = 1.0f;

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

	glActiveTexture(GL_TEXTURE0 + TEXTURE_ID_TANK);
	glBindTexture(GL_TEXTURE_2D, texture_names[TEXTURE_ID_TANK]);

	My_glTexImage2D_from_file("Data/static_objects/tank.jpg");

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void set_material_tiger(void) {
	// assume ShaderProgram_TXPS is used
	glUniform4fv(loc_material.ambient_color, 1, material_tiger.ambient_color);
	glUniform4fv(loc_material.diffuse_color, 1, material_tiger.diffuse_color);
	glUniform4fv(loc_material.specular_color, 1, material_tiger.specular_color);
	glUniform1f(loc_material.specular_exponent, material_tiger.specular_exponent);
	glUniform4fv(loc_material.emissive_color, 1, material_tiger.emissive_color);
}


void draw_tiger(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(tiger_VAO);
	glDrawArrays(GL_TRIANGLES, tiger_vertex_offset[cur_frame_tiger], 3 * tiger_n_triangles[cur_frame_tiger]);
	glBindVertexArray(0);
}
void draw_ben(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(ben_VAO);
	glDrawArrays(GL_TRIANGLES, ben_vertex_offset[cur_frame_ben], 3 * ben_n_triangles[cur_frame_ben]);
	glBindVertexArray(0);
}
void draw_wolf(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(wolf_VAO);
	glDrawArrays(GL_TRIANGLES, wolf_vertex_offset[cur_frame_wolf], 3 * wolf_n_triangles[cur_frame_wolf]);
	glBindVertexArray(0);
}
void draw_spider(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(spider_VAO);
	glDrawArrays(GL_TRIANGLES, spider_vertex_offset[cur_frame_spider], 3 * spider_n_triangles[cur_frame_spider]);
	glBindVertexArray(0);
}
void draw_dragon(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(dragon_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * dragon_n_triangles);
	glBindVertexArray(0);
}
void draw_optimus(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(optimus_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * optimus_n_triangles);
	glBindVertexArray(0);
}
void draw_cow(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(cow_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * cow_n_triangles);
	glBindVertexArray(0);
}
void draw_bike(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(bike_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * bike_n_triangles);
	glBindVertexArray(0);
}
void draw_bus(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(bus_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * bus_n_triangles);
	glBindVertexArray(0);
}

void draw_godzilla(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(godzilla_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * godzilla_n_triangles);
	glBindVertexArray(0);
}

void draw_ironman(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(ironman_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * ironman_n_triangles);
	glBindVertexArray(0);
}

void draw_tank(void) {
	glFrontFace(GL_CW);

	glBindVertexArray(tank_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * tank_n_triangles);
	glBindVertexArray(0);
}
// callbacks
float PRP_distance_scale[6] = { 0.5f, 1.0f, 2.5f, 5.0f, 10.0f, 20.0f };


void set_material_godzilla(void) {
	// assume ShaderProgram_TXPS is used
	glUniform4fv(loc_material.ambient_color, 1, material_godzilla.ambient_color);
	glUniform4fv(loc_material.diffuse_color, 1, material_godzilla.diffuse_color);
	glUniform4fv(loc_material.specular_color, 1, material_godzilla.specular_color);
	glUniform1f(loc_material.specular_exponent, material_godzilla.specular_exponent);
	glUniform4fv(loc_material.emissive_color, 1, material_godzilla.emissive_color);
}


void set_material_tank(void) {
	// assume ShaderProgram_TXPS is used
	glUniform4fv(loc_material.ambient_color, 1, material_tank.ambient_color);
	glUniform4fv(loc_material.diffuse_color, 1, material_tank.diffuse_color);
	glUniform4fv(loc_material.specular_color, 1, material_tank.specular_color);
	glUniform1f(loc_material.specular_exponent, material_tank.specular_exponent);
	glUniform4fv(loc_material.emissive_color, 1, material_tank.emissive_color);
}


void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(h_ShaderProgram_simple);
	ModelViewMatrix = glm::scale(ViewMatrix, glm::vec3(200.0f, 200.0f, 200.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_simple, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glLineWidth(2.0f);
	draw_axes();
	glLineWidth(1.0f);

	glUseProgram(h_ShaderProgram_TXPS);
  	set_material_floor();
	glUniform1i(loc_texture, TEXTURE_ID_FLOOR);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-1500.0f, 0.0f, 1500.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(3000.0f, 3000.0f, 3000.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f*TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_floor();
	
	//동적인 tiger그리기
 	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);

	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-1000.0f - (clock_tiger * tanf(clock_tiger * 0.04f * TO_RADIAN)), -30.0f, 250.0f * sinf(clock_tiger * TO_RADIAN)));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, (60.0f * sinf(clock_tiger * TO_RADIAN)) * TO_RADIAN - 90.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, (240.0f) * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_tiger();
	
	glUseProgram(h_ShaderProgram_simple);
	ModelViewProjectionMatrix = glm::scale(ModelViewProjectionMatrix, glm::vec3(20.0f, 20.0f, 20.0f));
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_simple, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_axes();

	//동적인 ben그리기
	glUseProgram(h_ShaderProgram_TXPS);
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);
	ModelViewMatrix = glm::rotate(ViewMatrix, (-clock_ben-45.0f) * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(200.0f+clock_ben, clock_ben, 200.0f+clock_ben));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(100.0f, -100.0f, -100.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_ben();
	
	//동적인 wolf그리기
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);
	if (rotation_angle_wolf < 360) {
		//ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(360.0f-rotation_angle_wolf, 500.0f * sinf(rotation_angle_wolf * TO_RADIAN), 0.0f));
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(1000.0f, 0.0f, 0.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, -10.0f * rotation_angle_wolf * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
		//ModelViewMatrix = glm::rotate(ModelViewMatrix, -1.0f * rotation_angle_wolf * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	else {
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(rotation_angle_wolf, 500.0f * sinf(rotation_angle_wolf * TO_RADIAN), 0.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 1.0f * rotation_angle_wolf * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 1.0f * rotation_angle_wolf * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(500.0f, -250.0f, 0.0f));
	}

	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3((100.0f, 100.0f, 100.0f)));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_wolf();

	//동적인 spider그리기
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-300.0f + clock_spider, 0.0f, /*300.0f - spider_size - clock_spider*/0.0f));
	if (clock_spider > 180 && clock_spider < 360) {
		if (clock_spider < 270) {
			ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(1.0f, (clock_spider - 180.0f) * 5.0f, 1.0f));
			ModelViewMatrix = glm::rotate(ModelViewMatrix, 30.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		}
		else {
			ModelViewMatrix = glm::translate(ModelViewMatrix, glm::vec3(1.0f, (360.0f - clock_spider) * 5.0f, 1.0f));
			ModelViewMatrix = glm::rotate(ModelViewMatrix, -30.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		}
	}
	ModelViewMatrix = glm::rotate(ModelViewMatrix, 120.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(50.0f + spider_size, -50.0f - spider_size, 50.0f + spider_size));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_spider();

	//정적인 bike 그리기
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);
	
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-200.0f, -30.0f, 200.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 1.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(60.0f, 60.0f, 60.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_bike();

	//정적인 bus 그리기
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(100.0f, 300.0f, -100.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -30.0f * TO_RADIAN, glm::vec3(-1.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(3.0f, 3.0f, 10.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_bus();

	//정적인 godzilla 그리기
	set_material_godzilla();
	glUniform1i(loc_texture, TEXTURE_ID_GODZILLA);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(0.0f, -300.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(3.0f, 3.0f, 3.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_godzilla();


	//정적인 ironman 그리기
	set_material_tiger();
	glUniform1i(loc_texture, TEXTURE_ID_TIGER);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-400.0f, 200.0f, -400.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(-1.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(30.0f, 30.0f, 30.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_ironman();

	//정적인 tank 그리기
	set_material_tank();
	glUniform1i(loc_texture, TEXTURE_ID_TANK);
	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(200.0f, 50.0f, 200.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -150.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(10.0f, 10.0f, 10.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::inverseTranspose(glm::mat3(ModelViewMatrix));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPS, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPS, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPS, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	draw_tank();


	glUseProgram(0);

	glutSwapBuffers();
}

void timer_scene(int value) {
	timestamp_scene = (timestamp_scene + 1) % UINT_MAX;

	rotation_angle_tiger = -720 + timestamp_tiger % 360;
	rotation_angle_wolf = timestamp_wolf % 720;
	location_spider = timestamp_spider % 1000;

	clock_spider = (timestamp_spider % 540);
	clock_ben = timestamp_ben % 540;
	clock_tiger = -500 + timestamp_tiger % 1000;
	clock_wolf = timestamp_wolf % 1000;


	glutPostRedisplay();

	
	if (spider_flag == 1) {
		timestamp_spider = (timestamp_spider + 1) % UINT_MAX;
		cur_frame_spider = timestamp_scene % N_SPIDER_FRAMES;
	}
	else if (spider_flag == 2) {
		cur_frame_spider = 0;
		timestamp_spider = (timestamp_spider + 1) % UINT_MAX;
	}
	else if (spider_flag == 3) {
		cur_frame_spider = timestamp_scene % N_SPIDER_FRAMES;
	}
	else if (spider_flag == 4) {
		cur_frame_spider = 0;
	}

	if (tiger_flag == 1) {
		timestamp_tiger = (timestamp_tiger + 1) % UINT_MAX;
		cur_frame_tiger = timestamp_scene % N_TIGER_FRAMES;
	}
	else if (tiger_flag == 2) {
		cur_frame_tiger = 0;
		timestamp_tiger = (timestamp_tiger + 1) % UINT_MAX;
	}
	else if (tiger_flag == 3) {
		cur_frame_tiger = timestamp_scene % N_TIGER_FRAMES;
	}
	else if (tiger_flag == 4) {
		cur_frame_tiger = 0;
	}

	if (ben_flag == 1) {
		timestamp_ben = (timestamp_ben + 1) % UINT_MAX;
		cur_frame_ben = timestamp_scene % N_BEN_FRAMES;
	}
	else if (ben_flag == 2) {
		cur_frame_ben = 0;
		timestamp_ben = (timestamp_ben + 1) % UINT_MAX;
	}
	else if (ben_flag == 3) {
		cur_frame_ben = timestamp_scene % N_BEN_FRAMES;
	}
	else if (ben_flag == 4) {
		cur_frame_ben = 0;
	}

	if (wolf_flag == 1) {
		timestamp_wolf = (timestamp_wolf + 1) % UINT_MAX;
		cur_frame_wolf = timestamp_scene % N_WOLF_FRAMES;
	}
	else if (wolf_flag == 2) {
		cur_frame_wolf = 0;
		timestamp_wolf = (timestamp_wolf + 1) % UINT_MAX;
	}
	else if (wolf_flag == 3) {
		cur_frame_wolf = timestamp_scene % N_WOLF_FRAMES;
	}
	else if (wolf_flag == 4) {
		cur_frame_wolf = 0;
	}

	glutTimerFunc(10, timer_scene, 0);


}

void print_flag(int flag) {

	if (flag == 1)
		printf("It is moving and walking\n");
	else if(flag == 2)
		printf("It is moving and not walking\n");
	else if (flag == 3)
		printf("It is not moving and walking\n");
	else if (flag == 4)
		printf("It is not moving and not walking\n");


}

void setflag(int num) {
	if (num == 1) {
		fprintf(stdout, "^^^ Change spider Flag.\n");
		spider_flag += 1;

		if (spider_flag >= 5)
			spider_flag = 1;

		print_flag(spider_flag);
	}

	else if (num == 2) {
		fprintf(stdout, "^^^ Change tiger Flag.\n");
		tiger_flag += 1;

		if (tiger_flag >= 5)
			tiger_flag = 1;

		print_flag(tiger_flag);
	}

	else if (num == 3) {
		fprintf(stdout, "^^^ Change ben Flag.\n");
		ben_flag += 1;

		if (ben_flag >= 5)
			ben_flag = 1;

		print_flag(ben_flag);
	}

	else if (num == 4) {
		fprintf(stdout, "^^^ Change wolf Flag.\n");
		wolf_flag += 1;

		if (wolf_flag >= 5)
			wolf_flag = 1;

		print_flag(wolf_flag);
	}

	else {
		fprintf(stdout, "^^^ Set All Flags OFF.\n");
		spider_flag = 1;
		tiger_flag = 1;
		ben_flag = 1;
		wolf_flag = 1;
	}

}

void set_projection(int wheel) {
	if (wheel == 1) {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				if(ProjectionMatrix[i][j] != 0)
					ProjectionMatrix[i][j] += 0.1;
			}
		}
	}

	else {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				if (ProjectionMatrix[i][j] != 0)
					ProjectionMatrix[i][j] -= 0.1;
			}
		}
	}
}

void mouseWheel(int button, int dir, int x, int y) {
	glm::vec4 position_EC;
	glm::vec3 direction_EC;
	if (camera5_flag == 1) {
		if (dir > 0 && count < 10) {

			printf("%f %f %f %f\n", ProjectionMatrix[0][0], ProjectionMatrix[0][1], ProjectionMatrix[0][2], ProjectionMatrix[0][3]);
			printf("%f %f %f %f\n", ProjectionMatrix[1][0], ProjectionMatrix[1][1], ProjectionMatrix[1][2], ProjectionMatrix[1][3]);
			printf("%f %f %f %f\n", ProjectionMatrix[2][0], ProjectionMatrix[2][1], ProjectionMatrix[2][2], ProjectionMatrix[2][3]);
			printf("%f %f %f %f\n\n", ProjectionMatrix[3][0], ProjectionMatrix[3][1], ProjectionMatrix[3][2], ProjectionMatrix[3][3]);


			set_projection(1);

			//camera5.pos[0] = camera5.pos[0] - camera5.naxis[0]*10.0f;
			//camera5.pos[1] = camera5.pos[1] - camera5.naxis[1]*10.0f;
			//camera5.pos[2] = camera5.pos[2] - camera5.naxis[2]*10.0f;

			set_ViewMatrix_from_camera_frame5();
			ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
			count++;
		}
		else if (dir < 0 && count > -10) {
			//camera5.pos[0] = camera5.pos[0] + camera5.naxis[0] * 10.0f;
			//camera5.pos[1] = camera5.pos[1] + camera5.naxis[1] * 10.0f;
			//camera5.pos[2] = camera5.pos[2] + camera5.naxis[2] * 10.0f;

			set_projection(-1);

			set_ViewMatrix_from_camera_frame5();
			ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
			count--;
		}
	}
	glUseProgram(h_ShaderProgram_TXPS);
	// Must update the light 1's geometry in EC.
	position_EC = ViewMatrix * glm::vec4(light[1].position[0], light[1].position[1],
		light[1].position[2], light[1].position[3]);
	glUniform4fv(loc_light[1].position, 1, &position_EC[0]);
	direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[1].spot_direction[0],
		light[1].spot_direction[1], light[1].spot_direction[2]);
	glUniform3fv(loc_light[1].spot_direction, 1, &direction_EC[0]);
	glUseProgram(0);
}

int motion_flag = -1;

void renew_cam_orientation_rotation_around1(int angle, float x, float y, float z) {
	// let's get a help from glm
	glm::mat3 RotationMatrix;
	glm::vec3 direction;

	RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0), TO_RADIAN * angle,
		glm::vec3(x, y, z)));


	direction = RotationMatrix * glm::vec3(camera1.uaxis[0], camera1.uaxis[1], camera1.uaxis[2]);
	direction = normalize(direction);
	camera1.uaxis[0] = direction.x; camera1.uaxis[1] = direction.y; camera1.uaxis[2] = direction.z;

	direction = RotationMatrix * glm::vec3(camera1.naxis[0], camera1.naxis[1], camera1.naxis[2]);
	direction = normalize(direction);
	camera1.naxis[0] = direction.x; camera1.naxis[1] = direction.y; camera1.naxis[2] = direction.z;

	direction = RotationMatrix * glm::vec3(camera1.vaxis[0], camera1.vaxis[1], camera1.vaxis[2]);
	direction = normalize(direction);
	camera1.vaxis[0] = direction.x; camera1.vaxis[1] = direction.y; camera1.vaxis[2] = direction.z;

}

void renew_cam_orientation_rotation_around5(int angle, float x, float y, float z) {
	// let's get a help from glm
	glm::mat3 RotationMatrix;
	glm::vec3 direction;

	RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0), TO_RADIAN * angle,
		glm::vec3(x, y, z)));


	direction = RotationMatrix * glm::vec3(camera5.uaxis[0], camera5.uaxis[1], camera5.uaxis[2]);
	direction = normalize(direction);
	camera5.uaxis[0] = direction.x; camera5.uaxis[1] = direction.y; camera5.uaxis[2] = direction.z;

	direction = RotationMatrix * glm::vec3(camera5.naxis[0], camera5.naxis[1], camera5.naxis[2]);
	direction = normalize(direction);
	camera5.naxis[0] = direction.x; camera5.naxis[1] = direction.y; camera5.naxis[2] = direction.z;

	direction = RotationMatrix * glm::vec3(camera5.vaxis[0], camera5.vaxis[1], camera5.vaxis[2]);
	direction = normalize(direction);
	camera5.vaxis[0] = direction.x; camera5.vaxis[1] = direction.y; camera5.vaxis[2] = direction.z;

}

void motion(int x, int y) {
	int disX = centerX - x;
	int disY = centerY - y;

	if (disX < 0)
		disX *= -1;
	if (disY < 0)
		disY *= -1;

	glm::vec4 position_EC;
	glm::vec3 direction_EC;

	if (motion_flag == -1) return;

	else {
		if(y > centerY)
			renew_cam_orientation_rotation_around1(disY * 0.01, camera1.uaxis[0], camera1.uaxis[1], camera1.uaxis[2]);
		else if(y < centerY)
			renew_cam_orientation_rotation_around1(-(disY * 0.01), camera1.uaxis[0], camera1.uaxis[1], camera1.uaxis[2]);
		if (x > centerX)
			renew_cam_orientation_rotation_around1(disX * 0.01, camera1.vaxis[0], camera1.vaxis[1], camera1.vaxis[2]);
		else if (x < centerX)
			renew_cam_orientation_rotation_around1(-(disX * 0.01), camera1.vaxis[0], camera1.vaxis[1], camera1.vaxis[2]);
		set_ViewMatrix_from_camera_frame1();
		glUseProgram(h_ShaderProgram_TXPS);
		// Must update the light 1's geometry in EC.
		position_EC = ViewMatrix * glm::vec4(light[1].position[0], light[1].position[1],
			light[1].position[2], light[1].position[3]);
		glUniform4fv(loc_light[1].position, 1, &position_EC[0]);
		direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[1].spot_direction[0],
			light[1].spot_direction[1], light[1].spot_direction[2]);
		glUniform3fv(loc_light[1].spot_direction, 1, &direction_EC[0]);
		glUseProgram(0);
	}


}

void mouse(int button, int state, int x, int y) {
	if ((button == GLUT_LEFT_BUTTON)) {
		if (cameraNumber == 0) {		
			if (state == GLUT_DOWN) {
				motion_flag = 1;
			}
			else if (state == GLUT_UP) {
				motion_flag = -1;
			}
		}
		
	}
}

void initialize_lights_and_material(void) { // follow OpenGL conventions for initialization
	int i;

	glUseProgram(h_ShaderProgram_TXPS);

	glUniform4f(loc_global_ambient_color, 0.115f, 0.115f, 0.115f, 1.0f);
	for (i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; i++) {
		glUniform1i(loc_light[i].light_on, 0); // turn off all lights initially
		glUniform4f(loc_light[i].position, 0.0f, 0.0f, 1.0f, 0.0f);
		glUniform4f(loc_light[i].ambient_color, 0.0f, 0.0f, 0.0f, 1.0f);
		if (i == 0) {
			glUniform4f(loc_light[i].diffuse_color, 1.0f, 1.0f, 1.0f, 1.0f);
			glUniform4f(loc_light[i].specular_color, 1.0f, 1.0f, 1.0f, 1.0f);
		}
		else {
			glUniform4f(loc_light[i].diffuse_color, 0.0f, 0.0f, 0.0f, 1.0f);
			glUniform4f(loc_light[i].specular_color, 0.0f, 0.0f, 0.0f, 1.0f);
		}
		glUniform3f(loc_light[i].spot_direction, 0.0f, 0.0f, -1.0f);
		glUniform1f(loc_light[i].spot_exponent, 0.0f); // [0.0, 128.0]
		glUniform1f(loc_light[i].spot_cutoff_angle, 180.0f); // [0.0, 90.0] or 180.0 (180.0 for no spot light effect)
		glUniform4f(loc_light[i].light_attenuation_factors, 1.0f, 0.0f, 0.0f, 0.0f); // .w != 0.0f for no ligth attenuation
	}

	glUniform4f(loc_material.ambient_color, 0.2f, 0.2f, 0.2f, 1.0f);
	glUniform4f(loc_material.diffuse_color, 0.8f, 0.8f, 0.8f, 1.0f);
	glUniform4f(loc_material.specular_color, 0.0f, 0.0f, 0.0f, 1.0f);
	glUniform4f(loc_material.emissive_color, 0.0f, 0.0f, 0.0f, 1.0f);
	glUniform1f(loc_material.specular_exponent, 0.0f); // [0.0, 128.0]

	glUseProgram(0);
}

void initialize_flags(void) {
	flag_tiger_animation = 1;
	flag_polygon_fill = 1;
	flag_texture_mapping = 1;
	flag_fog = 0;

	glUseProgram(h_ShaderProgram_TXPS);
	glUniform1i(loc_flag_fog, flag_fog);
	glUniform1i(loc_flag_texture_mapping, flag_texture_mapping);
	glUseProgram(0);
}

void initialize_OpenGL(void) {

	glEnable(GL_MULTISAMPLE);


	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	/*
	ViewMatrix = glm::lookAt(glm::vec3(-400.0f, 1500.0f, 200.0f),
		glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));*/

	ViewMatrix = glm::mat4(1.0f);
	ProjectionMatrix = glm::mat4(1.0f);
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;

	initialize_lights_and_material();
	initialize_flags();

	glGenTextures(N_TEXTURES_USED, texture_names);

	//fprintf(stdout, "init : %f\n", ProjectionMatrix[0][0]);
}

void special(int key, int x, int y) {

	static int flag_cull_face = 0;

	glm::vec4 position_EC;
	glm::vec3 direction_EC;

	#define ANGLE 10.0
	#define SIZE 0.1
		//카메라 변경(1~4번)

		if (camera5_flag == -1) {
			switch (key) {
			case GLUT_KEY_LEFT:
				cameraNumber = (cameraNumber + 1) % 4;
				fprintf(stdout, "^^^ Camera Number = %d.\n", cameraNumber+1);

				ProjectionMatrix = tmpMatrix;

				if (cameraNumber == 0) {
					initialize_camera1();
					/*
					ViewMatrix = glm::lookAt(glm::vec3(-400.0f, 1500.0f, 200.0f),
						glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));*/
					
				}

				else if (cameraNumber == 1) {
					ViewMatrix = glm::lookAt(glm::vec3(1500.0f, 300.0f, -600.0f),
						glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				}

				else if (cameraNumber == 2) {
					ViewMatrix = glm::lookAt(glm::vec3(100.0f, 100.0f, 1500.0f),
						glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				}

				else if (cameraNumber == 3) {
					ViewMatrix = glm::lookAt(glm::vec3(1000.0f, 1000.0f, 1000.0f),
						glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				}

				glUseProgram(h_ShaderProgram_TXPS);
				// Must update the light 1's geometry in EC.
				position_EC = ViewMatrix * glm::vec4(light[1].position[0], light[1].position[1],
					light[1].position[2], light[1].position[3]);
				glUniform4fv(loc_light[1].position, 1, &position_EC[0]);
				direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[1].spot_direction[0],
					light[1].spot_direction[1], light[1].spot_direction[2]);
				glUniform3fv(loc_light[1].spot_direction, 1, &direction_EC[0]);
				glUseProgram(0);

				glutPostRedisplay();
				break;
				//카메라 변경(4~1번)
			case GLUT_KEY_RIGHT:
				cameraNumber = (cameraNumber - 1) % 4;
				if (cameraNumber < 0)
					cameraNumber = 3;
				fprintf(stdout, "^^^ Camera Number = %d.\n", cameraNumber + 1);

				ProjectionMatrix = tmpMatrix;

				if (cameraNumber == 0) {
					initialize_camera1();
					/*
					ViewMatrix = glm::lookAt(glm::vec3(-400.0f, 1500.0f, 200.0f),
						glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));*/
				}

				else if (cameraNumber == 1) {
					ViewMatrix = glm::lookAt(glm::vec3(1500.0f, 300.0f, -600.0f),
						glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				}

				else if (cameraNumber == 2) {
					ViewMatrix = glm::lookAt(glm::vec3(100.0f, 100.0f, 1500.0f),
						glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				}

				else if (cameraNumber == 3) {
					ViewMatrix = glm::lookAt(glm::vec3(1000.0f, 1000.0f, 1000.0f),
						glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				}

				glUseProgram(h_ShaderProgram_TXPS);
				// Must update the light 1's geometry in EC.
				position_EC = ViewMatrix * glm::vec4(light[1].position[0], light[1].position[1],
					light[1].position[2], light[1].position[3]);
				glUniform4fv(loc_light[1].position, 1, &position_EC[0]);
				direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[1].spot_direction[0],
					light[1].spot_direction[1], light[1].spot_direction[2]);
				glUniform3fv(loc_light[1].spot_direction, 1, &direction_EC[0]);
				glUseProgram(0);

				glutPostRedisplay();
				break;
			}
		}

		//5번 카메라인 경우
		else if (camera5_flag == 1) {
			switch (key) {
				
				//해당 축 방향 기준으로 translation(-)
				case GLUT_KEY_LEFT:
					if (axis == 1) {				
						camera5.pos[0] -= 15.0f *(camera5.uaxis[0]);
						camera5.pos[1] -= 15.0f *(camera5.uaxis[1]);
						camera5.pos[2] -= 15.0f *(camera5.uaxis[2]);

						set_ViewMatrix_from_camera_frame5();
							
						ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;						
					}

					else if (axis == 2) {
						camera5.pos[0] -= 15.0f * (camera5.vaxis[0]);
						camera5.pos[1] -= 15.0f * (camera5.vaxis[1]);
						camera5.pos[2] -= 15.0f * (camera5.vaxis[2]);

						set_ViewMatrix_from_camera_frame5();
						ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
					}

					else if (axis == 3) {
						camera5.pos[0] -= 15.0f * (camera5.naxis[0]);
						camera5.pos[1] -= 15.0f * (camera5.naxis[1]);
						camera5.pos[2] -= 15.0f * (camera5.naxis[2]);

						set_ViewMatrix_from_camera_frame5();
						ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
					}

					
					glUseProgram(h_ShaderProgram_TXPS);
					// Must update the light 1's geometry in EC.
					position_EC = ViewMatrix * glm::vec4(light[1].position[0], light[1].position[1],
						light[1].position[2], light[1].position[3]);
					glUniform4fv(loc_light[1].position, 1, &position_EC[0]);
					direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[1].spot_direction[0],
						light[1].spot_direction[1], light[1].spot_direction[2]);
					glUniform3fv(loc_light[1].spot_direction, 1, &direction_EC[0]);
					glUseProgram(0);
					glutPostRedisplay();
				break;

				//해당 축 방향 기준으로 translation(+)
				case GLUT_KEY_RIGHT:
					if (axis == 1) {
						camera5.pos[0] += 15.0f * (camera5.uaxis[0]);
						camera5.pos[1] += 15.0f * (camera5.uaxis[1]);
						camera5.pos[2] += 15.0f * (camera5.uaxis[2]);

						set_ViewMatrix_from_camera_frame5();
						ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
					}

					else if (axis == 2) {
						camera5.pos[0] += 15.0f * (camera5.vaxis[0]);
						camera5.pos[1] += 15.0f * (camera5.vaxis[1]);
						camera5.pos[2] += 15.0f * (camera5.vaxis[2]);

						set_ViewMatrix_from_camera_frame5();
						ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
					}

					else if (axis == 3) {
						camera5.pos[0] += 15.0f * (camera5.naxis[0]);
						camera5.pos[1] += 15.0f * (camera5.naxis[1]);
						camera5.pos[2] += 15.0f * (camera5.naxis[2]);

						set_ViewMatrix_from_camera_frame5();
						ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
					}

					
					glUseProgram(h_ShaderProgram_TXPS);
					// Must update the light 1's geometry in EC.
					position_EC = ViewMatrix * glm::vec4(light[1].position[0], light[1].position[1],
						light[1].position[2], light[1].position[3]);
					glUniform4fv(loc_light[1].position, 1, &position_EC[0]);
					direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[1].spot_direction[0],
						light[1].spot_direction[1], light[1].spot_direction[2]);
					glUniform3fv(loc_light[1].spot_direction, 1, &direction_EC[0]);
					glUseProgram(0);
					glutPostRedisplay();
				break;

				//해당 축 방향 기준으로 rotation(-)
				case GLUT_KEY_UP:
					if (axis == 1) {				
						renew_cam_orientation_rotation_around5(15, camera5.uaxis[0], camera5.uaxis[1], camera5.uaxis[2]);
						set_ViewMatrix_from_camera_frame5();
					}

					else if (axis == 2) {
						renew_cam_orientation_rotation_around5(15, camera5.vaxis[0], camera5.vaxis[1], camera5.vaxis[2]);
						set_ViewMatrix_from_camera_frame5();
					}

					else if (axis == 3) {
						renew_cam_orientation_rotation_around5(15, camera5.naxis[0], camera5.naxis[1], camera5.naxis[2]);
						set_ViewMatrix_from_camera_frame5();
					}

					glUseProgram(h_ShaderProgram_TXPS);
					// Must update the light 1's geometry in EC.
					position_EC = ViewMatrix * glm::vec4(light[1].position[0], light[1].position[1],
						light[1].position[2], light[1].position[3]);
					glUniform4fv(loc_light[1].position, 1, &position_EC[0]);
					direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[1].spot_direction[0],
						light[1].spot_direction[1], light[1].spot_direction[2]);
					glUniform3fv(loc_light[1].spot_direction, 1, &direction_EC[0]);
					glUseProgram(0);
					glutPostRedisplay();
				break;

				//해당 축 방향 기준으로 rotation(+)
				case GLUT_KEY_DOWN:
					if (axis == 1) {
						renew_cam_orientation_rotation_around5(-15, camera5.uaxis[0], camera5.uaxis[1], camera5.uaxis[2]);
						set_ViewMatrix_from_camera_frame5();
					}

					else if (axis == 2) {
						renew_cam_orientation_rotation_around5(-15, camera5.vaxis[0], camera5.vaxis[1], camera5.vaxis[2]);
						set_ViewMatrix_from_camera_frame5();
					}

					else if (axis == 3) {
						renew_cam_orientation_rotation_around5(-15, camera5.naxis[0], camera5.naxis[1], camera5.naxis[2]);
						set_ViewMatrix_from_camera_frame5();
					}

					glUseProgram(h_ShaderProgram_TXPS);
					// Must update the light 1's geometry in EC.
					position_EC = ViewMatrix * glm::vec4(light[1].position[0], light[1].position[1],
						light[1].position[2], light[1].position[3]);
					glUniform4fv(loc_light[1].position, 1, &position_EC[0]);
					direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[1].spot_direction[0],
						light[1].spot_direction[1], light[1].spot_direction[2]);
					glUniform3fv(loc_light[1].spot_direction, 1, &direction_EC[0]);
					glUseProgram(0);
					glutPostRedisplay();
				break;
			}

		}
		/*
		case GLUT_KEY_DOWN:
			scale_black_hole -= SIZE;
			glutPostRedisplay();
			break;
		case GLUT_KEY_UP:
			scale_black_hole += SIZE;
			glutPostRedisplay();
			break;
		*/
}

void reshape(int width, int height) {

	float aspect_ratio;

	glViewport(0, 0, width, height);

	centerX = width / 2;
	centerY = height / 2;

	aspect_ratio = (float) width / height;

	ProjectionMatrix = glm::perspective(45.0f*TO_RADIAN, aspect_ratio, 100.0f, 20000.0f);

	tmpMatrix = ProjectionMatrix;

	glutPostRedisplay();
}

void cleanup(void) {
	glDeleteVertexArrays(1, &axes_VAO); 
	glDeleteBuffers(1, &axes_VBO);

	glDeleteVertexArrays(1, &rectangle_VAO);
	glDeleteBuffers(1, &rectangle_VBO);

	glDeleteVertexArrays(1, &tiger_VAO);
	glDeleteBuffers(1, &tiger_VBO);

	glDeleteTextures(N_TEXTURES_USED, texture_names);
}

void keyboard(unsigned char key, int x, int y) {
	static int flag_cull_face = 0;
	static int PRP_distance_level = 4;

	glm::vec4 position_EC;
	glm::vec3 direction_EC;

	if ((key >= '0') && (key <= '0' + NUMBER_OF_LIGHT_SUPPORTED - 1)) {
		int light_ID = (int)(key - '0');

		glUseProgram(h_ShaderProgram_TXPS);
		light[light_ID].light_on = 1 - light[light_ID].light_on;
		glUniform1i(loc_light[light_ID].light_on, light[light_ID].light_on);
		glUseProgram(0);

		glutPostRedisplay();
		return;
	}

	switch (key) {
		//pause모드 설정을 한다. 플래그가 세워져 있는 물체의 동작을 일시정지시킨다.
	/*
	case 'a': // toggle the animation effect.
		flag_tiger_animation = 1 - flag_tiger_animation;
		if (flag_tiger_animation) {
			//glutTimerFunc(100, timer_scene, 0);
			fprintf(stdout, "^^^ Pause mode ON.\n");
		}
		else {
			//glutTimerFunc(100, timer_scene, 0);
			fprintf(stdout, "^^^ Pause mode OFF.\n");
		}
		break;
		//안개??
	case 'f':
		flag_fog = 1 - flag_fog;
		glUseProgram(h_ShaderProgram_TXPS);
		glUniform1i(loc_flag_fog, flag_fog);
		glUseProgram(0);
		glutPostRedisplay();
		break;
		//바닥 바꾸기
			/*
		case 't':
			flag_texture_mapping = 1 - flag_texture_mapping;
			glUseProgram(h_ShaderProgram_TXPS);
			glUniform1i(loc_flag_texture_mapping, flag_texture_mapping);
			glUseProgram(0);
			glutPostRedisplay();
			break;
			*/
	/*
	case 'm':
		flag_cull_face = (flag_cull_face + 1) % 3;
		switch (flag_cull_face) {
		case 0:
			glDisable(GL_CULL_FACE);
			glutPostRedisplay();
			break;
		case 1: // cull back faces;
			glCullFace(GL_BACK);
			glEnable(GL_CULL_FACE);
			glutPostRedisplay();
			break;
		case 2: // cull front faces;
			glCullFace(GL_FRONT);
			glEnable(GL_CULL_FACE);
			glutPostRedisplay();
			break;
		}
		break;
		*/

		//이동가능한 5번 카메라를 나타낸다.
	case 'c':
		camera5_flag *= -1;
		if (camera5_flag == 1) {
			fprintf(stdout, "^^^ fifth camera!!User can do translation or rotation.\n");
			count = 0;
			ProjectionMatrix = tmpMatrix;

			//fprintf(stdout, "%f\n", ProjectionMatrix[0][0]);

			initialize_camera5();
			//set_ViewMatrix_from_camera_frame5();

			glUseProgram(h_ShaderProgram_TXPS);
			// Must update the light 1's geometry in EC.
			position_EC = ViewMatrix * glm::vec4(light[1].position[0], light[1].position[1],
				light[1].position[2], light[1].position[3]);
			glUniform4fv(loc_light[1].position, 1, &position_EC[0]);
			direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[1].spot_direction[0],
				light[1].spot_direction[1], light[1].spot_direction[2]);
			glUniform3fv(loc_light[1].spot_direction, 1, &direction_EC[0]);
			glUseProgram(0);
		}
		glutPostRedisplay();
		break;

		//5번 카메라의 기준을 x축으로 지정한다.
	case 'x':
		axis = 1;
		glutPostRedisplay();
		break;
		//5번 카메라의 기준을 y축으로 지정한다.
	case 'y':
		axis = 2;
		glutPostRedisplay();
		break;
		//5번 카메라의 기준을 z축으로 지정한다.
	case 'z':
		axis = 3;
		glutPostRedisplay();
		break;

	/*
		//바닥 바꾸기
	case 'p':
		flag_polygon_fill = 1 - flag_polygon_fill;
		if (flag_polygon_fill)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glutPostRedisplay();
		break;*/
		//거미에 대한 플래그 설정
	case 's':
		setflag(1);
		glutPostRedisplay();
		break;
		//호랑이에 대한 플래그 설정
	case 't':
		setflag(2);
		glutPostRedisplay();
		break;
		//ben에 대한 플래그 설정
	case 'b':
		setflag(3);
		glutPostRedisplay();
		break;
		//wolf에 대한 플래그 설정
	case 'w':
		setflag(4);
		glutPostRedisplay();
		break;
		//모든 flag해제
	case 'o':
		setflag(5);
		glutPostRedisplay();
		break;
	case 27: // ESC key
		glutLeaveMainLoop(); // Incur destuction callback for cleanups
		break;
	}
}

void register_callbacks(void) {
	glutSpecialFunc(special);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	glutTimerFunc(100, timer_scene, 0);
	glutCloseFunc(cleanup);
	glutMouseWheelFunc(mouseWheel);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
}


void prepare_shader_program(void) {
	int i;
	char string[256];
	ShaderInfo shader_info_simple[3] = {
		{ GL_VERTEX_SHADER, "Shaders/simple.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/simple.frag" },
		{ GL_NONE, NULL }
	};
	ShaderInfo shader_info_TXPS[3] = {
		{ GL_VERTEX_SHADER, "Shaders/Phong_Tx.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/Phong_Tx.frag" },
		{ GL_NONE, NULL }
	};

	h_ShaderProgram_simple = LoadShaders(shader_info_simple);
	loc_primitive_color = glGetUniformLocation(h_ShaderProgram_simple, "u_primitive_color");
	loc_ModelViewProjectionMatrix_simple = glGetUniformLocation(h_ShaderProgram_simple, "u_ModelViewProjectionMatrix");

	h_ShaderProgram_TXPS = LoadShaders(shader_info_TXPS);
	loc_ModelViewProjectionMatrix_TXPS = glGetUniformLocation(h_ShaderProgram_TXPS, "u_ModelViewProjectionMatrix");
	loc_ModelViewMatrix_TXPS = glGetUniformLocation(h_ShaderProgram_TXPS, "u_ModelViewMatrix");
	loc_ModelViewMatrixInvTrans_TXPS = glGetUniformLocation(h_ShaderProgram_TXPS, "u_ModelViewMatrixInvTrans");

	loc_global_ambient_color = glGetUniformLocation(h_ShaderProgram_TXPS, "u_global_ambient_color");
	for (i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; i++) {
		sprintf(string, "u_light[%d].light_on", i);
		loc_light[i].light_on = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		sprintf(string, "u_light[%d].position", i);
		loc_light[i].position = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		sprintf(string, "u_light[%d].ambient_color", i);
		loc_light[i].ambient_color = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		sprintf(string, "u_light[%d].diffuse_color", i);
		loc_light[i].diffuse_color = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		sprintf(string, "u_light[%d].specular_color", i);
		loc_light[i].specular_color = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		sprintf(string, "u_light[%d].spot_direction", i);
		loc_light[i].spot_direction = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		sprintf(string, "u_light[%d].spot_exponent", i);
		loc_light[i].spot_exponent = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		sprintf(string, "u_light[%d].spot_cutoff_angle", i);
		loc_light[i].spot_cutoff_angle = glGetUniformLocation(h_ShaderProgram_TXPS, string);
		sprintf(string, "u_light[%d].light_attenuation_factors", i);
		loc_light[i].light_attenuation_factors = glGetUniformLocation(h_ShaderProgram_TXPS, string);
	}

	loc_material.ambient_color = glGetUniformLocation(h_ShaderProgram_TXPS, "u_material.ambient_color");
	loc_material.diffuse_color = glGetUniformLocation(h_ShaderProgram_TXPS, "u_material.diffuse_color");
	loc_material.specular_color = glGetUniformLocation(h_ShaderProgram_TXPS, "u_material.specular_color");
	loc_material.emissive_color = glGetUniformLocation(h_ShaderProgram_TXPS, "u_material.emissive_color");
	loc_material.specular_exponent = glGetUniformLocation(h_ShaderProgram_TXPS, "u_material.specular_exponent");

	loc_texture = glGetUniformLocation(h_ShaderProgram_TXPS, "u_base_texture");

	loc_flag_texture_mapping = glGetUniformLocation(h_ShaderProgram_TXPS, "u_flag_texture_mapping");
	loc_flag_fog = glGetUniformLocation(h_ShaderProgram_TXPS, "u_flag_fog");
}



void set_up_scene_lights(void) {
	// point_light_EC: use light 0
	light[0].light_on = 1;
	light[0].position[0] = 0.0f; light[0].position[1] = 100.0f; 	// point light position in EC
	light[0].position[2] = 0.0f; light[0].position[3] = 1.0f;

	light[0].ambient_color[0] = 0.13f; light[0].ambient_color[1] = 0.13f;
	light[0].ambient_color[2] = 0.13f; light[0].ambient_color[3] = 1.0f;

	light[0].diffuse_color[0] = 0.5f; light[0].diffuse_color[1] = 0.5f;
	light[0].diffuse_color[2] = 0.5f; light[0].diffuse_color[3] = 1.5f;

	light[0].specular_color[0] = 0.8f; light[0].specular_color[1] = 0.8f;
	light[0].specular_color[2] = 0.8f; light[0].specular_color[3] = 1.0f;

	// spot_light_WC: use light 1
	light[1].light_on = 1;
	light[1].position[0] = -200.0f; light[1].position[1] = 500.0f; // spot light position in WC
	light[1].position[2] = -200.0f; light[1].position[3] = 1.0f;

	light[1].ambient_color[0] = 0.152f; light[1].ambient_color[1] = 0.152f;
	light[1].ambient_color[2] = 0.152f; light[1].ambient_color[3] = 1.0f;

	light[1].diffuse_color[0] = 0.572f; light[1].diffuse_color[1] = 0.572f;
	light[1].diffuse_color[2] = 0.572f; light[1].diffuse_color[3] = 1.0f;

	light[1].specular_color[0] = 0.772f; light[1].specular_color[1] = 0.772f;
	light[1].specular_color[2] = 0.772f; light[1].specular_color[3] = 1.0f;

	light[1].spot_direction[0] = 0.0f; light[1].spot_direction[1] = -1.0f; // spot light direction in WC
	light[1].spot_direction[2] = 0.0f;
	light[1].spot_cutoff_angle = 20.0f;
	light[1].spot_exponent = 8.0f;

	glUseProgram(h_ShaderProgram_TXPS);
	glUniform1i(loc_light[0].light_on, light[0].light_on);
	glUniform4fv(loc_light[0].position, 1, light[0].position);
	glUniform4fv(loc_light[0].ambient_color, 1, light[0].ambient_color);
	glUniform4fv(loc_light[0].diffuse_color, 1, light[0].diffuse_color);
	glUniform4fv(loc_light[0].specular_color, 1, light[0].specular_color);

	glUniform1i(loc_light[1].light_on, light[1].light_on);
	// need to supply position in EC for shading
	glm::vec4 position_EC = ViewMatrix * glm::vec4(light[1].position[0], light[1].position[1],
												light[1].position[2], light[1].position[3]);
	glUniform4fv(loc_light[1].position, 1, &position_EC[0]); 
	glUniform4fv(loc_light[1].ambient_color, 1, light[1].ambient_color);
	glUniform4fv(loc_light[1].diffuse_color, 1, light[1].diffuse_color);
	glUniform4fv(loc_light[1].specular_color, 1, light[1].specular_color);
	// need to supply direction in EC for shading in this example shader
	// note that the viewing transform is a rigid body transform
	// thus transpose(inverse(mat3(ViewMatrix)) = mat3(ViewMatrix)
	glm::vec3 direction_EC = glm::mat3(ViewMatrix) * glm::vec3(light[1].spot_direction[0], light[1].spot_direction[1], 
																light[1].spot_direction[2]);
	glUniform3fv(loc_light[1].spot_direction, 1, &direction_EC[0]); 
	glUniform1f(loc_light[1].spot_cutoff_angle, light[1].spot_cutoff_angle);
	glUniform1f(loc_light[1].spot_exponent, light[1].spot_exponent);
	glUseProgram(0);
}

void prepare_scene(void) {
	prepare_axes();
	prepare_floor();
	prepare_tiger();
	prepare_ben();
	prepare_wolf();
	prepare_spider();
	prepare_dragon();
	prepare_optimus();
	prepare_cow();
	prepare_bus();
	prepare_bike();
	prepare_godzilla();
	prepare_ironman();
	prepare_tank();
	set_up_scene_lights();
}

void initialize_renderer(void) {
	register_callbacks();
	prepare_shader_program();
	initialize_OpenGL();
	prepare_scene();
	initialize_camera5();
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

#define N_MESSAGE_LINES 1
void main(int argc, char *argv[]) {
	char program_name[64] = "HW3 20171646";
	char messages[N_MESSAGE_LINES][256] = { "    - Keys used: '0', '1', 'a', 't', 'f', 'c', 'd', 'y', 'u', 'i', 'o', 'ESC'"  };

	glutInit(&argc, argv);
  	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glutInitWindowSize(800, 800);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
}