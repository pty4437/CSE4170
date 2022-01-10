#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include "Shaders/LoadShaders.h"
GLuint h_ShaderProgram; // handle to shader program
GLint loc_ModelViewProjectionMatrix, loc_primitive_color; // indices of uniform variables

														  // include glm/*.hpp only if necessary
														  //#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, ortho, etc.
glm::mat4 ModelViewProjectionMatrix;
glm::mat4 ViewMatrix, ProjectionMatrix, ViewProjectionMatrix;

#define TO_RADIAN 0.01745329252f  
#define TO_DEGREE 57.295779513f
#define BUFFER_OFFSET(offset) ((GLvoid *) (offset))

#define LOC_VERTEX 0

int win_width = 0, win_height = 0;
float centerx = 0.0f, centery = 0.0f, rotate_angle = 0.0f, scale_black_hole = 1.0f;

GLfloat axes[4][2];
GLfloat axes_color[3] = { 0.0f, 0.0f, 0.0f };
GLuint VBO_axes, VAO_axes;

unsigned int timestamp = 0;

void special(int key, int x, int y) {
#define ANGLE 10.0
#define SIZE 0.1
	switch (key) {
	case GLUT_KEY_LEFT:
		rotate_angle -= ANGLE;
		glutPostRedisplay();
		break;
	case GLUT_KEY_RIGHT:
		rotate_angle += ANGLE;
		glutPostRedisplay();
		break;
	case GLUT_KEY_DOWN:
		scale_black_hole -= SIZE;
		glutPostRedisplay();
		break;
	case GLUT_KEY_UP:
		scale_black_hole += SIZE;
		glutPostRedisplay();
		break;
	}
}

void timer(int value) {
	timestamp = (timestamp + 1) % UINT_MAX;
	glutPostRedisplay();
	glutTimerFunc(10, timer, 0);
}

int leftbuttonpressed = 0;

void mouse(int button, int state, int x, int y) {
	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN))
		leftbuttonpressed = 1;
	else if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_UP))
		leftbuttonpressed = 0;

	printf("%f %f\n", centerx, centery);
}

void motion(int x, int y) {
	static int delay = 0;
	static float tmpx = 0.0, tmpy = 0.0;
	float dx, dy;
	if (leftbuttonpressed) {
		centerx = x - win_width / 2.0f, centery = (win_height - y) - win_height / 2.0f;
		if (delay == 8) {
			dx = centerx - tmpx;
			dy = centery - tmpy;

			tmpx = centerx, tmpy = centery;
			delay = 0;
		}
		glutPostRedisplay();
		delay++;
	}
}

GLfloat line[2][2];
GLfloat line_color[3] = { 1.0f, 0.0f, 0.0f };
GLuint VBO_line, VAO_line;

#define AIRPLANE_BIG_WING 0
#define AIRPLANE_SMALL_WING 1
#define AIRPLANE_BODY 2
#define AIRPLANE_BACK 3
#define AIRPLANE_SIDEWINDER1 4
#define AIRPLANE_SIDEWINDER2 5
#define AIRPLANE_CENTER 6
GLfloat big_wing[6][2] = { { 0.0, 0.0 },{ -20.0, 15.0 },{ -20.0, 20.0 },{ 0.0, 23.0 },{ 20.0, 20.0 },{ 20.0, 15.0 } };
GLfloat small_wing[6][2] = { { 0.0, -18.0 },{ -11.0, -12.0 },{ -12.0, -7.0 },{ 0.0, -10.0 },{ 12.0, -7.0 },{ 11.0, -12.0 } };
GLfloat body[5][2] = { { 0.0, -25.0 },{ -6.0, 0.0 },{ -6.0, 22.0 },{ 6.0, 22.0 },{ 6.0, 0.0 } };
GLfloat back[5][2] = { { 0.0, 25.0 },{ -7.0, 24.0 },{ -7.0, 21.0 },{ 7.0, 21.0 },{ 7.0, 24.0 } };
GLfloat sidewinder1[5][2] = { { -20.0, 10.0 },{ -18.0, 3.0 },{ -16.0, 10.0 },{ -18.0, 20.0 },{ -20.0, 20.0 } };
GLfloat sidewinder2[5][2] = { { 20.0, 10.0 },{ 18.0, 3.0 },{ 16.0, 10.0 },{ 18.0, 20.0 },{ 20.0, 20.0 } };
GLfloat center[1][2] = { { 0.0, 0.0 } };
GLfloat airplane_color[7][3] = {
	{ 150 / 255.0f, 129 / 255.0f, 183 / 255.0f },  // big_wing
	{ 245 / 255.0f, 211 / 255.0f,   0 / 255.0f },  // small_wing
	{ 111 / 255.0f,  85 / 255.0f, 157 / 255.0f },  // body
	{ 150 / 255.0f, 129 / 255.0f, 183 / 255.0f },  // back
	{ 245 / 255.0f, 211 / 255.0f,   0 / 255.0f },  // sidewinder1
	{ 245 / 255.0f, 211 / 255.0f,   0 / 255.0f },  // sidewinder2
	{ 255 / 255.0f,   0 / 255.0f,   0 / 255.0f }   // center
};

GLuint VBO_airplane, VAO_airplane;

int airplane_clock = 0;
float airplane_s_factor = 1.0f;

void prepare_airplane() {
	GLsizeiptr buffer_size = sizeof(big_wing) + sizeof(small_wing) + sizeof(body) + sizeof(back)
		+ sizeof(sidewinder1) + sizeof(sidewinder2) + sizeof(center);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_airplane);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_airplane);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(big_wing), big_wing);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing), sizeof(small_wing), small_wing);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing), sizeof(body), body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing) + sizeof(body), sizeof(back), back);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing) + sizeof(body) + sizeof(back),
		sizeof(sidewinder1), sidewinder1);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing) + sizeof(body) + sizeof(back)
		+ sizeof(sidewinder1), sizeof(sidewinder2), sidewinder2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing) + sizeof(body) + sizeof(back)
		+ sizeof(sidewinder1) + sizeof(sidewinder2), sizeof(center), center);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_airplane);
	glBindVertexArray(VAO_airplane);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_airplane);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_airplane() { // Draw airplane in its MC.
	glBindVertexArray(VAO_airplane);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_BIG_WING]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_SMALL_WING]);
	glDrawArrays(GL_TRIANGLE_FAN, 6, 6);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 5);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_BACK]);
	glDrawArrays(GL_TRIANGLE_FAN, 17, 5);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_SIDEWINDER1]);
	glDrawArrays(GL_TRIANGLE_FAN, 22, 5);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_SIDEWINDER2]);
	glDrawArrays(GL_TRIANGLE_FAN, 27, 5);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_CENTER]);
	glPointSize(5.0);
	glDrawArrays(GL_POINTS, 32, 1);
	glPointSize(1.0);
	glBindVertexArray(0);
}

//shirt
#define SHIRT_LEFT_BODY 0
#define SHIRT_RIGHT_BODY 1
#define SHIRT_LEFT_COLLAR 2
#define SHIRT_RIGHT_COLLAR 3
#define SHIRT_FRONT_POCKET 4
#define SHIRT_BUTTON1 5
#define SHIRT_BUTTON2 6
#define SHIRT_BUTTON3 7
#define SHIRT_BUTTON4 8
GLfloat left_body[6][2] = { { 0.0, -9.0 },{ -8.0, -9.0 },{ -11.0, 8.0 },{ -6.0, 10.0 },{ -3.0, 7.0 },{ 0.0, 9.0 } };
GLfloat right_body[6][2] = { { 0.0, -9.0 },{ 0.0, 9.0 },{ 3.0, 7.0 },{ 6.0, 10.0 },{ 11.0, 8.0 },{ 8.0, -9.0 } };
GLfloat left_collar[4][2] = { { 0.0, 9.0 },{ -3.0, 7.0 },{ -6.0, 10.0 },{ -4.0, 11.0 } };
GLfloat right_collar[4][2] = { { 0.0, 9.0 },{ 4.0, 11.0 },{ 6.0, 10.0 },{ 3.0, 7.0 } };
GLfloat front_pocket[6][2] = { { 5.0, 0.0 },{ 4.0, 1.0 },{ 4.0, 3.0 },{ 7.0, 3.0 },{ 7.0, 1.0 },{ 6.0, 0.0 } };
GLfloat button1[3][2] = { { -1.0, 6.0 },{ 1.0, 6.0 },{ 0.0, 5.0 } };
GLfloat button2[3][2] = { { -1.0, 3.0 },{ 1.0, 3.0 },{ 0.0, 2.0 } };
GLfloat button3[3][2] = { { -1.0, 0.0 },{ 1.0, 0.0 },{ 0.0, -1.0 } };
GLfloat button4[3][2] = { { -1.0, -3.0 },{ 1.0, -3.0 },{ 0.0, -4.0 } };

GLfloat shirt_color[9][3] = {
	{ 255 / 255.0f, 255 / 255.0f, 255 / 255.0f },
	{ 255 / 255.0f, 255 / 255.0f, 255 / 255.0f },
	{ 206 / 255.0f, 173 / 255.0f, 184 / 255.0f },
	{ 206 / 255.0f, 173 / 255.0f, 184 / 255.0f },
	{ 206 / 255.0f, 173 / 255.0f, 184 / 255.0f },
	{ 206 / 255.0f, 173 / 255.0f, 184 / 255.0f },
	{ 206 / 255.0f, 173 / 255.0f, 184 / 255.0f },
	{ 206 / 255.0f, 173 / 255.0f, 184 / 255.0f },
	{ 206 / 255.0f, 173 / 255.0f, 184 / 255.0f }
};

GLuint VBO_shirt, VAO_shirt;
void prepare_shirt() {
	GLsizeiptr buffer_size = sizeof(left_body) + sizeof(right_body) + sizeof(left_collar) + sizeof(right_collar)
		+ sizeof(front_pocket) + sizeof(button1) + sizeof(button2) + sizeof(button3) + sizeof(button4);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_shirt);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_shirt);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(left_body), left_body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(left_body), sizeof(right_body), right_body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(left_body) + sizeof(right_body), sizeof(left_collar), left_collar);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(left_body) + sizeof(right_body) + sizeof(left_collar), sizeof(right_collar), right_collar);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(left_body) + sizeof(right_body) + sizeof(left_collar) + sizeof(right_collar),
		sizeof(front_pocket), front_pocket);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(left_body) + sizeof(right_body) + sizeof(left_collar) + sizeof(right_collar)
		+ sizeof(front_pocket), sizeof(button1), button1);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(left_body) + sizeof(right_body) + sizeof(left_collar) + sizeof(right_collar)
		+ sizeof(front_pocket) + sizeof(button1), sizeof(button2), button2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(left_body) + sizeof(right_body) + sizeof(left_collar) + sizeof(right_collar)
		+ sizeof(front_pocket) + sizeof(button1) + sizeof(button2), sizeof(button3), button3);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(left_body) + sizeof(right_body) + sizeof(left_collar) + sizeof(right_collar)
		+ sizeof(front_pocket) + sizeof(button1) + sizeof(button2) + sizeof(button3), sizeof(button4), button4);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_shirt);
	glBindVertexArray(VAO_shirt);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_shirt);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_shirt() {
	glBindVertexArray(VAO_shirt);

	glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_LEFT_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

	glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_RIGHT_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 6, 6);

	glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_LEFT_COLLAR]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);

	glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_RIGHT_COLLAR]);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4);

	glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_FRONT_POCKET]);
	glDrawArrays(GL_TRIANGLE_FAN, 20, 6);

	glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_BUTTON1]);
	glDrawArrays(GL_TRIANGLE_FAN, 26, 3);

	glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_BUTTON2]);
	glDrawArrays(GL_TRIANGLE_FAN, 29, 3);

	glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_BUTTON3]);
	glDrawArrays(GL_TRIANGLE_FAN, 32, 3);

	glUniform3fv(loc_primitive_color, 1, shirt_color[SHIRT_BUTTON4]);
	glDrawArrays(GL_TRIANGLE_FAN, 35, 3);
	glBindVertexArray(0);
}

//car
#define CAR_BODY 0
#define CAR_FRAME 1
#define CAR_WINDOW 2
#define CAR_LEFT_LIGHT 3
#define CAR_RIGHT_LIGHT 4
#define CAR_LEFT_WHEEL 5
#define CAR_RIGHT_WHEEL 6

GLfloat car_body[4][2] = { { -16.0, -8.0 },{ -16.0, 0.0 },{ 16.0, 0.0 },{ 16.0, -8.0 } };
GLfloat car_frame[4][2] = { { -10.0, 0.0 },{ -10.0, 10.0 },{ 10.0, 10.0 },{ 10.0, 0.0 } };
GLfloat car_window[4][2] = { { -8.0, 0.0 },{ -8.0, 8.0 },{ 8.0, 8.0 },{ 8.0, 0.0 } };
GLfloat car_left_light[4][2] = { { -9.0, -6.0 },{ -10.0, -5.0 },{ -9.0, -4.0 },{ -8.0, -5.0 } };
GLfloat car_right_light[4][2] = { { 9.0, -6.0 },{ 8.0, -5.0 },{ 9.0, -4.0 },{ 10.0, -5.0 } };
GLfloat car_left_wheel[4][2] = { { -10.0, -12.0 },{ -10.0, -8.0 },{ -6.0, -8.0 },{ -6.0, -12.0 } };
GLfloat car_right_wheel[4][2] = { { 6.0, -12.0 },{ 6.0, -8.0 },{ 10.0, -8.0 },{ 10.0, -12.0 } };

GLfloat car_color[7][3] = {
	{ 0 / 255.0f, 149 / 255.0f, 159 / 255.0f },
	{ 0 / 255.0f, 149 / 255.0f, 159 / 255.0f },
	{ 216 / 255.0f, 208 / 255.0f, 174 / 255.0f },
	{ 249 / 255.0f, 244 / 255.0f, 0 / 255.0f },
	{ 249 / 255.0f, 244 / 255.0f, 0 / 255.0f },
	{ 21 / 255.0f, 30 / 255.0f, 26 / 255.0f },
	{ 21 / 255.0f, 30 / 255.0f, 26 / 255.0f }
};

GLuint VBO_car, VAO_car;
void prepare_car() {
	GLsizeiptr buffer_size = sizeof(car_body) + sizeof(car_frame) + sizeof(car_window) + sizeof(car_left_light)
		+ sizeof(car_right_light) + sizeof(car_left_wheel) + sizeof(car_right_wheel);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_car);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_car);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(car_body), car_body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body), sizeof(car_frame), car_frame);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body) + sizeof(car_frame), sizeof(car_window), car_window);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body) + sizeof(car_frame) + sizeof(car_window), sizeof(car_left_light), car_left_light);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body) + sizeof(car_frame) + sizeof(car_window) + sizeof(car_left_light),
		sizeof(car_right_light), car_right_light);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body) + sizeof(car_frame) + sizeof(car_window) + sizeof(car_left_light)
		+ sizeof(car_right_light), sizeof(car_left_wheel), car_left_wheel);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body) + sizeof(car_frame) + sizeof(car_window) + sizeof(car_left_light)
		+ sizeof(car_right_light) + sizeof(car_left_wheel), sizeof(car_right_wheel), car_right_wheel);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_car);
	glBindVertexArray(VAO_car);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_car);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_car() {
	glBindVertexArray(VAO_car);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_FRAME]);
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_WINDOW]);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_LEFT_LIGHT]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_RIGHT_LIGHT]);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_LEFT_WHEEL]);
	glDrawArrays(GL_TRIANGLE_FAN, 20, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_RIGHT_WHEEL]);
	glDrawArrays(GL_TRIANGLE_FAN, 24, 4);

	glBindVertexArray(0);
}

// hat
#define HAT_LEAF 0
#define HAT_BODY 1
#define HAT_STRIP 2
#define HAT_BOTTOM 3

GLfloat hat_leaf[4][2] = { { 3.0, 20.0 },{ 3.0, 28.0 },{ 9.0, 32.0 },{ 9.0, 24.0 } };
GLfloat hat_body[4][2] = { { -19.5, 2.0 },{ 19.5, 2.0 },{ 15.0, 20.0 },{ -15.0, 20.0 } };
GLfloat hat_strip[4][2] = { { -20.0, 0.0 },{ 20.0, 0.0 },{ 19.5, 2.0 },{ -19.5, 2.0 } };
GLfloat hat_bottom[4][2] = { { 25.0, 0.0 },{ -25.0, 0.0 },{ -25.0, -4.0 },{ 25.0, -4.0 } };

GLfloat hat_color[4][3] = {
	{ 167 / 255.0f, 255 / 255.0f, 55 / 255.0f },
{ 255 / 255.0f, 144 / 255.0f, 32 / 255.0f },
{ 255 / 255.0f, 40 / 255.0f, 33 / 255.0f },
{ 255 / 255.0f, 144 / 255.0f, 32 / 255.0f }
};

GLuint VBO_hat, VAO_hat;

void prepare_hat() {
	GLsizeiptr buffer_size = sizeof(hat_leaf) + sizeof(hat_body) + sizeof(hat_strip) + sizeof(hat_bottom);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_hat);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_hat);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(hat_leaf), hat_leaf);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(hat_leaf), sizeof(hat_body), hat_body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(hat_leaf) + sizeof(hat_body), sizeof(hat_strip), hat_strip);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(hat_leaf) + sizeof(hat_body) + sizeof(hat_strip), sizeof(hat_bottom), hat_bottom);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_hat);
	glBindVertexArray(VAO_hat);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_hat);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_hat() {
	glBindVertexArray(VAO_hat);

	glUniform3fv(loc_primitive_color, 1, hat_color[HAT_LEAF]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glUniform3fv(loc_primitive_color, 1, hat_color[HAT_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

	glUniform3fv(loc_primitive_color, 1, hat_color[HAT_STRIP]);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);

	glUniform3fv(loc_primitive_color, 1, hat_color[HAT_BOTTOM]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);

	glBindVertexArray(0);
}

// sword

#define SWORD_BODY 0
#define SWORD_BODY2 1
#define SWORD_HEAD 2
#define SWORD_HEAD2 3
#define SWORD_IN 4
#define SWORD_DOWN 5
#define SWORD_BODY_IN 6

GLfloat sword_body[4][2] = { { -6.0, 0.0 },{ -6.0, -4.0 },{ 6.0, -4.0 },{ 6.0, 0.0 } };
GLfloat sword_body2[4][2] = { { -2.0, -4.0 },{ -2.0, -6.0 } ,{ 2.0, -6.0 },{ 2.0, -4.0 } };
GLfloat sword_head[4][2] = { { -2.0, 0.0 },{ -2.0, 16.0 } ,{ 2.0, 16.0 },{ 2.0, 0.0 } };
GLfloat sword_head2[3][2] = { { -2.0, 16.0 },{ 0.0, 19.46 } ,{ 2.0, 16.0 } };
GLfloat sword_in[4][2] = { { -0.3, 0.7 },{ -0.3, 15.3 } ,{ 0.3, 15.3 },{ 0.3, 0.7 } };
GLfloat sword_down[4][2] = { { -2.0, -6.0 } ,{ 2.0, -6.0 },{ 4.0, -8.0 },{ -4.0, -8.0 } };
GLfloat sword_body_in[4][2] = { { 0.0, -1.0 } ,{ 1.0, -2.732 },{ 0.0, -4.464 },{ -1.0, -2.732 } };

GLfloat sword_color[7][3] = {
	{ 139 / 255.0f, 69 / 255.0f, 19 / 255.0f },
{ 139 / 255.0f, 69 / 255.0f, 19 / 255.0f },
{ 155 / 255.0f, 155 / 255.0f, 155 / 255.0f },
{ 155 / 255.0f, 155 / 255.0f, 155 / 255.0f },
{ 0 / 255.0f, 0 / 255.0f, 0 / 255.0f },
{ 139 / 255.0f, 69 / 255.0f, 19 / 255.0f },
{ 255 / 255.0f, 0 / 255.0f, 0 / 255.0f }
};

GLuint VBO_sword, VAO_sword;

void prepare_sword() {
	GLsizeiptr buffer_size = sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head) + sizeof(sword_head2) + sizeof(sword_in) + sizeof(sword_down) + sizeof(sword_body_in);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_sword);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_sword);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sword_body), sword_body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body), sizeof(sword_body2), sword_body2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2), sizeof(sword_head), sword_head);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head), sizeof(sword_head2), sword_head2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head) + sizeof(sword_head2), sizeof(sword_in), sword_in);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head) + sizeof(sword_head2) + sizeof(sword_in), sizeof(sword_down), sword_down);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head) + sizeof(sword_head2) + sizeof(sword_in) + sizeof(sword_down), sizeof(sword_body_in), sword_body_in);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_sword);
	glBindVertexArray(VAO_sword);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_sword);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_sword() {
	glBindVertexArray(VAO_sword);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_BODY2]);
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_HEAD]);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_HEAD2]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 3);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_IN]);
	glDrawArrays(GL_TRIANGLE_FAN, 15, 4);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_DOWN]);
	glDrawArrays(GL_TRIANGLE_FAN, 19, 4);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_BODY_IN]);
	glDrawArrays(GL_TRIANGLE_FAN, 23, 4);

	glBindVertexArray(0);
}

// white_star

#define WHITE_STAR_BODY1 0
#define WHITE_STAR_BODY2 1
#define WHITE_STAR_BODY3 2
#define WHITE_STAR_BODY4 3
#define WHITE_STAR_BODY5 4
#define WHITE_STAR_BODY6 5

GLfloat white_star_body1[3][2] = { { -6.0, 0.0 },{ 6.0, 0.0 },{ 0.0, 10.0 } };
GLfloat white_star_body2[3][2] = { { -8.0, 5.0 },{ -2.0, 5.0 } ,{ -6.0, 0.0 } };
GLfloat white_star_body3[3][2] = { { 8.0, 5.0 },{ 2.0, 5.0 } ,{ 6.0, 0.0 } };
GLfloat white_star_body4[3][2] = { { -6.0, 0.0 },{ 6.0, 0.0 },{ 0.0, -10.0 } };
GLfloat white_star_body5[3][2] = { { -8.0, -5.0 },{ -2.0, -5.0 } ,{ -6.0, 0.0 } };
GLfloat white_star_body6[3][2] = { { 8.0, -5.0 },{ 2.0, -5.0 } ,{ 6.0, 0.0 } };

GLfloat white_star_color[6][3] = {
	{ 255 / 255.0f, 255 / 255.0f, 255 / 255.0f },
{ 255 / 255.0f, 255 / 255.0f, 255 / 255.0f },
{ 255 / 255.0f, 255 / 255.0f, 255 / 255.0f },
{ 255 / 255.0f, 255 / 255.0f, 255 / 255.0f },
{ 255 / 255.0f, 255 / 255.0f, 255 / 255.0f },
{ 255 / 255.0f, 255 / 255.0f, 255 / 255.0f }
};

GLuint VBO_white_star, VAO_white_star;

void prepare_white_star() {
	GLsizeiptr buffer_size = sizeof(white_star_body1) + sizeof(white_star_body2) + sizeof(white_star_body3) + sizeof(white_star_body4) + sizeof(white_star_body5) + sizeof(white_star_body6);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_white_star);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_white_star);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(white_star_body1), white_star_body1);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(white_star_body1), sizeof(white_star_body2), white_star_body2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(white_star_body1) + sizeof(white_star_body2), sizeof(white_star_body3), white_star_body3);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(white_star_body1) + sizeof(white_star_body2) + sizeof(white_star_body3), sizeof(white_star_body4), white_star_body4);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(white_star_body1) + sizeof(white_star_body2) + sizeof(white_star_body3) + sizeof(white_star_body4), sizeof(white_star_body5), white_star_body5);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(white_star_body1) + sizeof(white_star_body2) + sizeof(white_star_body3) + sizeof(white_star_body4) + sizeof(white_star_body5), sizeof(white_star_body6), white_star_body6);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_white_star);
	glBindVertexArray(VAO_white_star);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_white_star);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_white_star() {
	glBindVertexArray(VAO_white_star);

	glUniform3fv(loc_primitive_color, 1, white_star_color[WHITE_STAR_BODY1]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 3);

	glUniform3fv(loc_primitive_color, 1, white_star_color[WHITE_STAR_BODY2]);
	glDrawArrays(GL_TRIANGLE_FAN, 3, 3);

	glUniform3fv(loc_primitive_color, 1, white_star_color[WHITE_STAR_BODY3]);
	glDrawArrays(GL_TRIANGLE_FAN, 6, 3);

	glUniform3fv(loc_primitive_color, 1, white_star_color[WHITE_STAR_BODY4]);
	glDrawArrays(GL_TRIANGLE_FAN, 9, 3);

	glUniform3fv(loc_primitive_color, 1, white_star_color[WHITE_STAR_BODY5]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 3);

	glUniform3fv(loc_primitive_color, 1, white_star_color[WHITE_STAR_BODY6]);
	glDrawArrays(GL_TRIANGLE_FAN, 15, 3);

	glBindVertexArray(0);
}

// blue_star

#define BLUE_STAR_BODY1 0
#define BLUE_STAR_BODY2 1
#define BLUE_STAR_BODY3 2
#define BLUE_STAR_BODY4 3
#define BLUE_STAR_BODY5 4
#define BLUE_STAR_BODY6 5

GLfloat blue_star_body1[3][2] = { { -6.0, 0.0 },{ 6.0, 0.0 },{ 0.0, 10.0 } };
GLfloat blue_star_body2[3][2] = { { -8.0, 5.0 },{ -2.0, 5.0 } ,{ -6.0, 0.0 } };
GLfloat blue_star_body3[3][2] = { { 8.0, 5.0 },{ 2.0, 5.0 } ,{ 6.0, 0.0 } };
GLfloat blue_star_body4[3][2] = { { -6.0, 0.0 },{ 6.0, 0.0 },{ 0.0, -10.0 } };
GLfloat blue_star_body5[3][2] = { { -8.0, -5.0 },{ -2.0, -5.0 } ,{ -6.0, 0.0 } };
GLfloat blue_star_body6[3][2] = { { 8.0, -5.0 },{ 2.0, -5.0 } ,{ 6.0, 0.0 } };

GLfloat blue_star_color[6][3] = {
	{ 120 / 255.0f, 120 / 255.0f, 255 / 255.0f },
{ 120 / 255.0f, 120 / 255.0f, 255 / 255.0f },
{ 120 / 255.0f, 120 / 255.0f, 255 / 255.0f },
{ 120 / 255.0f, 120 / 255.0f, 255 / 255.0f },
{ 120 / 255.0f, 120 / 255.0f, 255 / 255.0f },
{ 120 / 255.0f, 120 / 255.0f, 255 / 255.0f }
};

GLuint VBO_blue_star, VAO_blue_star;

void prepare_blue_star() {
	GLsizeiptr buffer_size = sizeof(blue_star_body1) + sizeof(blue_star_body2) + sizeof(blue_star_body3) + sizeof(blue_star_body4) + sizeof(blue_star_body5) + sizeof(blue_star_body6);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_blue_star);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_blue_star);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(blue_star_body1), blue_star_body1);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(blue_star_body1), sizeof(blue_star_body2), blue_star_body2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(blue_star_body1) + sizeof(blue_star_body2), sizeof(blue_star_body3), blue_star_body3);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(blue_star_body1) + sizeof(blue_star_body2) + sizeof(blue_star_body3), sizeof(blue_star_body4), blue_star_body4);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(blue_star_body1) + sizeof(blue_star_body2) + sizeof(blue_star_body3) + sizeof(blue_star_body4), sizeof(blue_star_body5), blue_star_body5);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(blue_star_body1) + sizeof(blue_star_body2) + sizeof(blue_star_body3) + sizeof(blue_star_body4) + sizeof(blue_star_body5), sizeof(blue_star_body6), blue_star_body6);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_blue_star);
	glBindVertexArray(VAO_blue_star);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_blue_star);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_blue_star() {
	glBindVertexArray(VAO_blue_star);

	glUniform3fv(loc_primitive_color, 1, blue_star_color[BLUE_STAR_BODY1]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 3);

	glUniform3fv(loc_primitive_color, 1, blue_star_color[BLUE_STAR_BODY2]);
	glDrawArrays(GL_TRIANGLE_FAN, 3, 3);

	glUniform3fv(loc_primitive_color, 1, blue_star_color[BLUE_STAR_BODY3]);
	glDrawArrays(GL_TRIANGLE_FAN, 6, 3);

	glUniform3fv(loc_primitive_color, 1, blue_star_color[BLUE_STAR_BODY4]);
	glDrawArrays(GL_TRIANGLE_FAN, 9, 3);

	glUniform3fv(loc_primitive_color, 1, blue_star_color[BLUE_STAR_BODY5]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 3);

	glUniform3fv(loc_primitive_color, 1, blue_star_color[BLUE_STAR_BODY6]);
	glDrawArrays(GL_TRIANGLE_FAN, 15, 3);

	glBindVertexArray(0);
}

// red_star

#define RED_STAR_BODY1 0
#define RED_STAR_BODY2 1
#define RED_STAR_BODY3 2
#define RED_STAR_BODY4 3
#define RED_STAR_BODY5 4
#define RED_STAR_BODY6 5

GLfloat red_star_body1[3][2] = { { -6.0, 0.0 },{ 6.0, 0.0 },{ 0.0, 10.0 } };
GLfloat red_star_body2[3][2] = { { -8.0, 5.0 },{ -2.0, 5.0 } ,{ -6.0, 0.0 } };
GLfloat red_star_body3[3][2] = { { 8.0, 5.0 },{ 2.0, 5.0 } ,{ 6.0, 0.0 } };
GLfloat red_star_body4[3][2] = { { -6.0, 0.0 },{ 6.0, 0.0 },{ 0.0, -10.0 } };
GLfloat red_star_body5[3][2] = { { -8.0, -5.0 },{ -2.0, -5.0 } ,{ -6.0, 0.0 } };
GLfloat red_star_body6[3][2] = { { 8.0, -5.0 },{ 2.0, -5.0 } ,{ 6.0, 0.0 } };

GLfloat red_star_color[6][3] = {
	{ 255 / 255.0f, 120 / 255.0f, 120 / 255.0f },
{ 255 / 255.0f, 120 / 255.0f, 120 / 255.0f },
{ 255 / 255.0f, 120 / 255.0f, 120 / 255.0f },
{ 255 / 255.0f, 120 / 255.0f, 120 / 255.0f },
{ 255 / 255.0f, 120 / 255.0f, 120 / 255.0f },
{ 255 / 255.0f, 120 / 255.0f, 120 / 255.0f }
};

GLuint VBO_red_star, VAO_red_star;

void prepare_red_star() {
	GLsizeiptr buffer_size = sizeof(red_star_body1) + sizeof(red_star_body2) + sizeof(red_star_body3) + sizeof(red_star_body4) + sizeof(red_star_body5) + sizeof(red_star_body6);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_red_star);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_red_star);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(red_star_body1), red_star_body1);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(red_star_body1), sizeof(red_star_body2), red_star_body2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(red_star_body1) + sizeof(red_star_body2), sizeof(red_star_body3), red_star_body3);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(red_star_body1) + sizeof(red_star_body2) + sizeof(red_star_body3), sizeof(red_star_body4), red_star_body4);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(red_star_body1) + sizeof(red_star_body2) + sizeof(red_star_body3) + sizeof(red_star_body4), sizeof(red_star_body5), red_star_body5);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(red_star_body1) + sizeof(red_star_body2) + sizeof(red_star_body3) + sizeof(red_star_body4) + sizeof(red_star_body5), sizeof(red_star_body6), red_star_body6);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_red_star);
	glBindVertexArray(VAO_red_star);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_red_star);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_red_star() {
	glBindVertexArray(VAO_red_star);

	glUniform3fv(loc_primitive_color, 1, red_star_color[RED_STAR_BODY1]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 3);

	glUniform3fv(loc_primitive_color, 1, red_star_color[RED_STAR_BODY2]);
	glDrawArrays(GL_TRIANGLE_FAN, 3, 3);

	glUniform3fv(loc_primitive_color, 1, red_star_color[RED_STAR_BODY3]);
	glDrawArrays(GL_TRIANGLE_FAN, 6, 3);

	glUniform3fv(loc_primitive_color, 1, red_star_color[RED_STAR_BODY4]);
	glDrawArrays(GL_TRIANGLE_FAN, 9, 3);

	glUniform3fv(loc_primitive_color, 1, red_star_color[RED_STAR_BODY5]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 3);

	glUniform3fv(loc_primitive_color, 1, red_star_color[RED_STAR_BODY6]);
	glDrawArrays(GL_TRIANGLE_FAN, 15, 3);

	glBindVertexArray(0);
}

// black_hole

#define BLACK_HOLE_BODY1 0

GLfloat black_hole_body1[11][2] = { { 0.0, 100.0 },{ 10.0, 99.0 },{ 20.0, 96.0 }, { 30.0, 90.0 },{ 40.0, 80.0 },{ 50.0, 50.0 },{ 40.0, 20.0 }, { 30.0, 10.0 }, { 20.0, 4.0 },{ 10.0, 1.0 },{ 0.0, 0.0 } };

GLfloat black_hole_color[1][3] = {
	{ 255 / 255.0f, 255 / 255.0f, 255 / 255.0f },
};

GLuint VBO_black_hole, VAO_black_hole;

void prepare_black_hole() {
	GLsizeiptr buffer_size = sizeof(black_hole_body1);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_black_hole);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_black_hole);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(black_hole_body1), black_hole_body1);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_black_hole);
	glBindVertexArray(VAO_black_hole);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_black_hole);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_black_hole() {
	glBindVertexArray(VAO_black_hole);

	glLineWidth(3.0f);
	
	glUniform3fv(loc_primitive_color, 1, black_hole_color[BLACK_HOLE_BODY1]);
	glDrawArrays(GL_LINE_STRIP, 0, 11);

	glBindVertexArray(0);
}

// UFO

#define UFO_BODY1 0
#define UFO_BODY2 1
#define UFO_BODY3 2
#define UFO_BODY4 3
#define UFO_BODY5 4
#define UFO_BODY6 5
#define UFO_BODY7 6
#define UFO_BODY8 7
#define UFO_BODY9 8
#define UFO_BODY10 9
#define UFO_BODY11 10
#define UFO_BODY12 11

GLfloat ufo_body1[3][2] = { { -10.0, 0.0 },{ 0.0, 0.0 },{ 0.0, 6.0 } };
GLfloat ufo_body2[3][2] = { { 0.0, 0.0 },{ 6.0, 0.0 } ,{ 0.0, 6.0 } };
GLfloat ufo_body3[3][2] = { { 6.0, 6.0 },{ 6.0, 0.0 } ,{ 0.0, 6.0 } };
GLfloat ufo_body4[3][2] = { { -18.0, 0.0 },{ -18.0, -5.0 },{ 24.0, 0.0 } };
GLfloat ufo_body5[3][2] = { { 24.0, 0.0 },{ 24.0, -5.0 } ,{ -18.0, -5.0 } };
GLfloat ufo_body6[3][2] = { { 16.0, 0.0 },{ 6.0, 6.0 } ,{ 6.0, 0.0 } };
GLfloat ufo_body7[3][2] = { { 1.0, 1.0 },{ 5.0, 1.0 } ,{ 1.0, 5.0 } };
GLfloat ufo_body8[3][2] = { { 5.0, 5.0 },{ 5.0, 1.0 } ,{ 1.0, 5.0 } };
GLfloat ufo_body9[2][2] = { { -16.0, -2.5 } ,{ 22.0, -2.5 } };

GLfloat ufo_body10[2][2] = { { 3.0, -5.0 } ,{ 3.0, -10.0 } };
GLfloat ufo_body11[2][2] = { { -3.0, -5.0 } ,{ -8.0, -10.0 } };
GLfloat ufo_body12[2][2] = { { 9.0, -5.0 } ,{ 14.0, -10.0 } };

GLfloat ufo_color[12][3] = {
{ 150 / 255.0f, 150 / 255.0f, 150 / 255.0f },
{ 150 / 255.0f, 150 / 255.0f, 150 / 255.0f },
{ 150 / 255.0f, 150 / 255.0f, 150 / 255.0f },
{ 200 / 255.0f, 200 / 255.0f, 200 / 255.0f },
{ 200 / 255.0f, 200 / 255.0f, 200 / 255.0f },
{ 150 / 255.0f, 150 / 255.0f, 150 / 255.0f },
{ 204 / 255.0f, 220 / 255.0f, 255 / 255.0f },
{ 204 / 255.0f, 220 / 255.0f, 255 / 255.0f },
{ 255 / 255.0f, 255 / 255.0f, 51 / 255.0f },
{ 0 / 255.0f, 51 / 255.0f, 0 / 255.0f },
{ 0 / 255.0f, 51 / 255.0f, 0 / 255.0f },
{ 0 / 255.0f, 51 / 255.0f, 0 / 255.0f }
};

GLuint VBO_ufo, VAO_ufo;

void prepare_ufo() {
	GLsizeiptr buffer_size = sizeof(ufo_body1) + sizeof(ufo_body2) + sizeof(ufo_body3) + sizeof(ufo_body4) + sizeof(ufo_body5) + sizeof(ufo_body6) + sizeof(ufo_body7) + sizeof(ufo_body8) + sizeof(ufo_body9) + sizeof(ufo_body10) + sizeof(ufo_body11) + sizeof(ufo_body12);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_ufo);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_ufo);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(ufo_body1), ufo_body1);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ufo_body1), sizeof(ufo_body2), ufo_body2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ufo_body1) + sizeof(ufo_body2), sizeof(ufo_body3), ufo_body3);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ufo_body1) + sizeof(ufo_body2) + sizeof(ufo_body3), sizeof(ufo_body4), ufo_body4);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ufo_body1) + sizeof(ufo_body2) + sizeof(ufo_body3) + sizeof(ufo_body4), sizeof(ufo_body5), ufo_body5);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ufo_body1) + sizeof(ufo_body2) + sizeof(ufo_body3) + sizeof(ufo_body4) + sizeof(ufo_body5), sizeof(ufo_body6), ufo_body6);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ufo_body1) + sizeof(ufo_body2) + sizeof(ufo_body3) + sizeof(ufo_body4) + sizeof(ufo_body5) + sizeof(ufo_body6), sizeof(ufo_body7), ufo_body7);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ufo_body1) + sizeof(ufo_body2) + sizeof(ufo_body3) + sizeof(ufo_body4) + sizeof(ufo_body5) + sizeof(ufo_body6) + sizeof(ufo_body7), sizeof(ufo_body8), ufo_body8);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ufo_body1) + sizeof(ufo_body2) + sizeof(ufo_body3) + sizeof(ufo_body4) + sizeof(ufo_body5) + sizeof(ufo_body6) + sizeof(ufo_body7) + sizeof(ufo_body8), sizeof(ufo_body9), ufo_body9);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ufo_body1) + sizeof(ufo_body2) + sizeof(ufo_body3) + sizeof(ufo_body4) + sizeof(ufo_body5) + sizeof(ufo_body6) + sizeof(ufo_body7) + sizeof(ufo_body8) + sizeof(ufo_body9), sizeof(ufo_body10),ufo_body10);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ufo_body1) + sizeof(ufo_body2) + sizeof(ufo_body3) + sizeof(ufo_body4) + sizeof(ufo_body5) + sizeof(ufo_body6) + sizeof(ufo_body7) + sizeof(ufo_body8) + sizeof(ufo_body9) + sizeof(ufo_body10), sizeof(ufo_body11), ufo_body11);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(ufo_body1) + sizeof(ufo_body2) + sizeof(ufo_body3) + sizeof(ufo_body4) + sizeof(ufo_body5) + sizeof(ufo_body6) + sizeof(ufo_body7) + sizeof(ufo_body8) + sizeof(ufo_body9) + sizeof(ufo_body10) + sizeof(ufo_body11), sizeof(ufo_body12), ufo_body12);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_ufo);
	glBindVertexArray(VAO_ufo);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_ufo);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_ufo() {
	glBindVertexArray(VAO_ufo);

	glUniform3fv(loc_primitive_color, 1, ufo_color[UFO_BODY1]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 3);

	glUniform3fv(loc_primitive_color, 1, ufo_color[UFO_BODY2]);
	glDrawArrays(GL_TRIANGLE_FAN, 3, 3);

	glUniform3fv(loc_primitive_color, 1, ufo_color[UFO_BODY3]);
	glDrawArrays(GL_TRIANGLE_FAN, 6, 3);

	glUniform3fv(loc_primitive_color, 1, ufo_color[UFO_BODY4]);
	glDrawArrays(GL_TRIANGLE_FAN, 9, 3);

	glUniform3fv(loc_primitive_color, 1, ufo_color[UFO_BODY5]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 3);

	glUniform3fv(loc_primitive_color, 1, ufo_color[UFO_BODY6]);
	glDrawArrays(GL_TRIANGLE_FAN, 15, 3);

	glUniform3fv(loc_primitive_color, 1, ufo_color[UFO_BODY7]);
	glDrawArrays(GL_TRIANGLE_FAN, 18, 3);

	glUniform3fv(loc_primitive_color, 1, ufo_color[UFO_BODY8]);
	glDrawArrays(GL_TRIANGLE_FAN, 21, 3);

	glUniform3fv(loc_primitive_color, 1, ufo_color[UFO_BODY9]);
	glDrawArrays(GL_LINE_STRIP, 24, 2);

	glUniform3fv(loc_primitive_color, 1, ufo_color[UFO_BODY10]);
	glDrawArrays(GL_LINE_STRIP, 26, 2);

	glUniform3fv(loc_primitive_color, 1, ufo_color[UFO_BODY11]);
	glDrawArrays(GL_LINE_STRIP, 28, 2);

	glUniform3fv(loc_primitive_color, 1, ufo_color[UFO_BODY12]);
	glDrawArrays(GL_LINE_STRIP, 30, 2);

	glBindVertexArray(0);
}

void display(void) {
	float x, r, s, delx, delr, dels;
	glm::mat4 ModelMatrix;

	glClear(GL_COLOR_BUFFER_BIT);

	glPointSize(5.0f); glBegin(GL_POINTS); glVertex2f(-0.5f, 0.5f); glVertex2f(0.5f, 0.5f); glVertex2f(-0.5f, -0.5f); glVertex2f(0.5f, -0.5f); glEnd();

	ModelMatrix = glm::mat4(1.0f);
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	//draw_axes();

	//drawing white star
	for (int i = 0; i < 10; i++) {
		int sign1 = rand() % 2;
		int sign2 = rand() % 2;

		int num1 = rand() % 30;
		int num2 = rand() % 30;

		if (sign1 == 1)
			num1 *= -1;
		if (sign2 == 1)
			num2 *= -1;

		for (int j = 0; j < 3; j++) {
			ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3((num1*50)*1.0f, (num2*50)*1.0f, 0.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.5f, 1.5f, 1.0f));
			ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
			glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			draw_white_star();
		}
	}

	//drawing blue star
	for (int i = 0; i < 10; i++) {
		int sign1 = rand() % 2;
		int sign2 = rand() % 2;

		int num1 = rand() % 30;
		int num2 = rand() % 30;

		if (sign1 == 1)
			num1 *= -1;
		if (sign2 == 1)
			num2 *= -1;

		for (int j = 0; j < 3; j++) {
			ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3((num1 * 50) * 1.0f, (num2 * 50) * 1.0f, 0.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.5f, 1.5f, 1.0f));
			ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
			glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			draw_blue_star();
		}
	}

	//drawing red star
	for (int i = 0; i < 10; i++) {
		int sign1 = rand() % 2;
		int sign2 = rand() % 2;

		int num1 = rand() % 30;
		int num2 = rand() % 30;

		if (sign1 == 1)
			num1 *= -1;
		if (sign2 == 1)
			num2 *= -1;

		for (int j = 0; j < 3; j++) {
			ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3((num1 * 50) * 1.0f, (num2 * 50) * 1.0f, 0.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.5f, 1.5f, 1.0f));
			ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
			glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			draw_red_star();
		}
	}

	//drawing black hole
	int black_hole_clock = (timestamp % 2000) / 2 - 180;
	float rotation_angle_black_hole = 90 * 1.0f * TO_RADIAN * (black_hole_clock * TO_RADIAN);

	for (int i = 0; i < 160; i = i + 2) {
		int scale_num = rand() % 18+1;

		ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(centerx, centery, 0.0f));
		ModelMatrix = glm::rotate(ModelMatrix, rotation_angle_black_hole, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(scale_num * (0.1f) * scale_black_hole, scale_num * (0.1f) * scale_black_hole, 1.0f));
		ModelMatrix = glm::rotate(ModelMatrix, (i * 3)*1.0f*TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_black_hole();
	}

	//drawing sword
	for (int i = 0; i < 15; i++) {
		int sword_clock = timestamp % 1000 + i * 30;

		ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(centerx, centery, 0.0f));	
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3((100 - sword_clock % 100) * 0.01f, (100 - sword_clock % 100) * 0.01f, 1.0f));
		ModelMatrix = glm::rotate(ModelMatrix, -(sword_clock % 100) * 1.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3((-100*i-400)*1.0f + (sword_clock % 150) * 2.0f -400.0f, 0.0f, 0.0f));
		ModelMatrix = glm::rotate(ModelMatrix, -90.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(3.0f, 3.0f, 1.0f));


		ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_sword();
	}


	//drawing airplane
	int airplane_clock = (timestamp % 2000) / 2 - 180;

	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(centerx, centery, 0.0f));
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3((200 - (airplane_clock % 200)) * 0.01f, (200 - (airplane_clock % 200)) * 0.01f, 1.0f));
	ModelMatrix = glm::rotate(ModelMatrix, -airplane_clock * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3((200 - (airplane_clock % 200)) * 1.0f, 0.0f, 0.0f));
	
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_airplane();

	//drawing hat
	int hat_clock = timestamp % 500;
	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(centerx, centery, 0.0f));
	int rotate_num = rand() % 100;

	if (hat_clock <= 250) {
		ModelMatrix = glm::rotate(ModelMatrix, -hat_clock/5 * TO_RADIAN * rotate_num, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3((250 - (hat_clock % 250)) * 0.01f, (250 - (hat_clock % 250)) * 0.01f, 1.0f));
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, scale_black_hole * ((hat_clock % 250)) * 1.0f, 0.0f));
	}
	else {
		ModelMatrix = glm::rotate(ModelMatrix, hat_clock / 3 * TO_RADIAN * rotate_num, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3((250 - (hat_clock%250)) * 0.01f, (250 - (hat_clock%250)) * 0.01f, 1.0f));
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, scale_black_hole * (-(hat_clock%250)) * 1.0f, 0.0f));
	}

	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_hat();

	//drawing shirt
	for (int i = 0; i < 30; i++) {
		int shirt_clock = timestamp % 1000 + i*10;
		ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

		if ((shirt_clock / 50) % 2 == 0)
			ModelMatrix = glm::rotate(ModelMatrix, (rotate_angle + 50 - shirt_clock % 50) * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		else
			ModelMatrix = glm::rotate(ModelMatrix, (rotate_angle + shirt_clock % 50) * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));

		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(3.0f, 3.0f, 1.0f));
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, (-500+i*3 + shirt_clock) * 1.0f, 0.0f));

		ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_shirt();
	}

	//drawing car
	for (int i = -3; i < 4; i++) {
		int car_clock = timestamp % 1440 - 720 + 200*i;
		float rotation_angle_car = TO_RADIAN * (car_clock % 360);
		float scaling_factor_car = 5.0f * (1.0f - fabs(cosf(car_clock * TO_RADIAN)));

		ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(centerx, centery, 0.0f));
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3((float)car_clock * tanf(car_clock * TO_RADIAN), 200.0f * sinf(car_clock * TO_RADIAN)+100.0f * i, 0.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(scaling_factor_car, scaling_factor_car, 1.0f));
		ModelMatrix = glm::rotate(ModelMatrix, rotation_angle_car, glm::vec3(0.0f, 0.0f, 1.0f));

		ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_car();
	}

	//drawing ufo
	int ufo_clock = timestamp % 500 - 250;

	ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	
	ModelMatrix = glm::translate(ModelMatrix, glm::vec3(ufo_clock * 5 * 1.0f, ufo_clock * 5 * 1.0f, 0.0f));
	ModelMatrix = glm::rotate(ModelMatrix, (ufo_clock%10) * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelMatrix = glm::scale(ModelMatrix, glm::vec3(5.0f - ufo_clock * 0.1f, 5.0f, 1.0f));
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_ufo();

	glFlush();

}

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 27: // ESC key
		glutLeaveMainLoop(); // Incur destuction callback for cleanups.
		break;
	}
}


void reshape(int width, int height) {
	win_width = width, win_height = height;

	glViewport(0, 0, win_width, win_height);
	ProjectionMatrix = glm::ortho(-win_width / 2.0, win_width / 2.0,
		-win_height / 2.0, win_height / 2.0, -1000.0, 1000.0);
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;

	glutPostRedisplay();
}

void cleanup(void) {

	glDeleteVertexArrays(1, &VAO_airplane);
	glDeleteBuffers(1, &VBO_airplane);

	glDeleteVertexArrays(1, &VAO_hat);
	glDeleteBuffers(1, &VBO_hat);

	glDeleteVertexArrays(1, &VAO_sword);
	glDeleteBuffers(1, &VBO_sword);

	glDeleteVertexArrays(1, &VAO_car);
	glDeleteBuffers(1, &VBO_car);

	glDeleteVertexArrays(1, &VAO_shirt);
	glDeleteBuffers(1, &VBO_shirt);

	glDeleteVertexArrays(1, &VAO_black_hole);
	glDeleteBuffers(1, &VBO_black_hole);

	glDeleteVertexArrays(1, &VAO_red_star);
	glDeleteBuffers(1, &VBO_red_star);

	glDeleteVertexArrays(1, &VAO_blue_star);
	glDeleteBuffers(1, &VBO_blue_star);

	glDeleteVertexArrays(1, &VAO_white_star);
	glDeleteBuffers(1, &VBO_white_star);

	glDeleteVertexArrays(1, &VAO_ufo);
	glDeleteBuffers(1, &VBO_ufo);
}

void register_callbacks(void) {
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	glutCloseFunc(cleanup);
	glutTimerFunc(10, timer, 0);
	glutMotionFunc(motion);
	glutMouseFunc(mouse);
	glutSpecialFunc(special);
}

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

void initialize_OpenGL(void) {
	glEnable(GL_MULTISAMPLE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glClearColor(0 / 255.0f, 0 / 255.0f, 0 / 255.0f, 1.0f);
	ViewMatrix = glm::mat4(1.0f);
}

void prepare_scene(void) {
	prepare_airplane();
	prepare_shirt();
	prepare_car();
	prepare_hat();
	prepare_sword();
	prepare_white_star();
	prepare_blue_star();
	prepare_red_star();
	prepare_black_hole();
	prepare_ufo();
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
	char program_name[64] = "HW2 : 20171646 Tae Yoon Park";
	char messages[N_MESSAGE_LINES][256] = {
		"    - Keys used: 'ESC' \n"
		"    - Left Mouse: 'Can move black hole' \n"
		"    - Left Key, Right Key : 'Rotate Shirt Snake' \n"
		"    - Up Key, Down Key : 'Scale Black Hole'"
	};

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_MULTISAMPLE);
	glutInitWindowSize(1300, 700);
	glutInitWindowPosition(0, 0);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
}


