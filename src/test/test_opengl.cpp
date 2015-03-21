/*
 
  BASIC GLFW + GLXW WINDOW AND OPENGL SETUP 
  ------------------------------------------
  See https://gist.github.com/roxlu/6698180 for the latest version of the example.
   
  We make use of the GLAD library for GL loading, see: https://github.com/Dav1dde/glad/
 
*/
#include <stdlib.h>
#include <stdio.h>

#define SCREEN_CAPTURE_IMPLEMENTATION
#include <glad/glad.h>
#include <screencapture/ScreenCaptureGL.h>
#include <GLFW/glfw3.h>

/* @todo remove, just for debugging. */
#define ROXLU_USE_PNG
#define ROXLU_IMPLEMENTATION
#include <tinylib.h>

void button_callback(GLFWwindow* win, int bt, int action, int mods);
void cursor_callback(GLFWwindow* win, double x, double y);
void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods);
void char_callback(GLFWwindow* win, unsigned int key);
void error_callback(int err, const char* desc);
void resize_callback(GLFWwindow* window, int width, int height);

int main() {

  glfwSetErrorCallback(error_callback);

  if(!glfwInit()) {
    printf("Error: cannot setup glfw.\n");
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* win = NULL;
  int w = 1280;
  int h = 720;

  win = glfwCreateWindow(w, h, "GLFW", NULL, NULL);
  if(!win) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetFramebufferSizeCallback(win, resize_callback);
  glfwSetKeyCallback(win, key_callback);
  glfwSetCharCallback(win, char_callback);
  glfwSetCursorPosCallback(win, cursor_callback);
  glfwSetMouseButtonCallback(win, button_callback);
  glfwMakeContextCurrent(win);
  glfwSwapInterval(1);

  if (!gladLoadGL()) {
    printf("Cannot load GL.\n");
    exit(1);
  }

  // ----------------------------------------------------------------
  // THIS IS WHERE YOU START CALLING OPENGL FUNCTIONS, NOT EARLIER!!
  // ----------------------------------------------------------------

  sc::Settings cfg;
  sc::ScreenCaptureGL cap;
  
  cfg.display = 0;
  cfg.pixel_format = SC_BGRA;
  cfg.output_width = 1280;
  cfg.output_height = 720;

  if (0 != cap.init()) {
    exit(EXIT_FAILURE);
  }

  if (0 != cap.configure(cfg)) {
    exit(EXIT_FAILURE);
  }

  if (0 != cap.start()) {
    exit(EXIT_FAILURE);
  }

  while(!glfwWindowShouldClose(win)) {
    
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    cap.update();
    cap.draw();
    //cap.draw(0,0,320,240); /* On Mac this results in only part of the screen being drawn. */
    
    glfwSwapBuffers(win);
    glfwPollEvents();
  }

  glfwTerminate();

  return EXIT_SUCCESS;
}

void char_callback(GLFWwindow* win, unsigned int key) {
}

void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods) {

  if(action != GLFW_PRESS) {
    return;
  }

  switch(key) {
    case GLFW_KEY_ESCAPE: {
      glfwSetWindowShouldClose(win, GL_TRUE);
      break;
    }
  };
}

void resize_callback(GLFWwindow* window, int width, int height) {
}

void cursor_callback(GLFWwindow* win, double x, double y) {
}

void button_callback(GLFWwindow* win, int bt, int action, int mods) {
  /*
    if(action == GLFW_PRESS) { 
    }
  */
}

void error_callback(int err, const char* desc) {
  printf("GLFW error: %s (%d)\n", desc, err);
}
