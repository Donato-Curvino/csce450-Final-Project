#include <iostream>
#include <string>
#include <memory>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GLSL.h"
#include "Camera.h"
#include "Program.h"
#include "MatrixStack.h"
#include "ShapeSkin.h"

using namespace std;

GLFWwindow* window;
string RESOURCE_DIR = "";
bool keyToggles[256] = {false};

shared_ptr<Camera> camera = NULL;
shared_ptr<Program> prog;
shared_ptr<Program> prog_simple;
ShapeSkin cylinder;
double t;
double t0;

static void error_callback(int error, const char *description) {
    std::cerr << description << std::endl;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

static void char_callback(GLFWwindow *window, unsigned int key) {
    keyToggles[key] = !keyToggles[key];
    switch(key) {
    }
}

static void cursor_position_callback(GLFWwindow* window, double xmouse, double ymouse) {
    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if(state == GLFW_PRESS) {
        camera->mouseMoved(xmouse, ymouse);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    // Get the current mouse position.
    double xmouse, ymouse;
    glfwGetCursorPos(window, &xmouse, &ymouse);
    // Get current window size.
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    if(action == GLFW_PRESS) {
        bool shift = mods & GLFW_MOD_SHIFT;
        bool ctrl  = mods & GLFW_MOD_CONTROL;
        bool alt   = mods & GLFW_MOD_ALT;
        camera->mouseClicked(xmouse, ymouse, shift, ctrl, alt);
    }
}

void init() {
    keyToggles[(unsigned)'c'] = true;

    camera = make_shared<Camera>();

    glClearColor(1.f, 1.f, 1.f, 1.f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // For drawing the grid, etc.
    prog_simple = make_shared<Program>();
    prog_simple->setShaderNames(RESOURCE_DIR + "simple_vert.glsl", RESOURCE_DIR + "simple_frag.glsl");
    prog_simple->setVerbose(true);

    prog_simple->addUniform("P");
    prog_simple->addUniform("MV");

    prog = make_shared<Program>();
    prog->setShaderNames(RESOURCE_DIR + "vert.glsl", RESOURCE_DIR + "frag.glsl");
    prog->setVerbose(true);
    prog->init();

    prog->addAttribute("aPos");
    prog->addAttribute("aNor");
    prog->addAttribute("aTex");
    prog->addAttribute("u");
    prog->addAttribute("bw");
    prog->addAttribute("bi");
    prog->addUniform("P");
    prog->addUniform("MV");
    prog->addUniform("bones");
    prog->addUniform("Gs");
    prog->addUniform("B");
    prog->addUniform("ligntPos");
    prog->addUniform("ka");
    prog->addUniform("ks");
    prog->addUniform("s");
    prog->addUniform("kdTex");

    // Bind the texture to unit 1.
    int unit = 1;
    prog->bind();
    glUniform1i(prog->getUniform("kdTex"), unit);
    prog->unbind();

    // Generate cylinder
    cylinder.makeCylinder(20);
    cylinder.init();

    glfwSetTime(0.);
    GLSL::checkError(GET_FILE_LINE);
}

void render() {
    // Update time.
    double t1 = glfwGetTime();
    float dt = (t1 - t0);
    if(keyToggles[(unsigned)' ']) {
        t += dt;
    }
    t0 = t1;

    // Get current frame buffer size.
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Use the window size for camera.
    glfwGetWindowSize(window, &width, &height);
    camera->setAspect((float)width/(float)height);

    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if(keyToggles[(unsigned)'c'])
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);

    if(keyToggles[(unsigned)'z'])
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    shared_ptr<MatrixStack> P = make_shared<MatrixStack>();
    shared_ptr<MatrixStack> MV = make_shared<MatrixStack>();

    // Apply camera transforms
    P->pushMatrix();
    camera->applyProjectionMatrix(P);
    MV->pushMatrix();
    camera->applyViewMatrix(MV);

    // Draw grid
    prog_simple->bind();
    glUniformMatrix4fv(prog_simple->getUniform("P"),  1, GL_FALSE, (float*)&P->topMatrix());
    glUniformMatrix4fv(prog_simple->getUniform("MV"), 1, GL_FALSE, (float*)&MV->topMatrix());
    float gridSizeHalf = 200.0f;
    int gridNx = 11;
    int gridNz = 11;
    glLineWidth(1);
    glColor3f(0.8f, 0.8f, 0.8f);
    glBegin(GL_LINES);
    for(int i = 0; i < gridNx; ++i) {
        float alpha = i / (gridNx - 1.0f);
        float x = (1.0f - alpha) * (-gridSizeHalf) + alpha * gridSizeHalf;
        glVertex3f(x, 0, -gridSizeHalf);
        glVertex3f(x, 0,  gridSizeHalf);
    }
    for(int i = 0; i < gridNz; ++i) {
        float alpha = i / (gridNz - 1.0f);
        float z = (1.0f - alpha) * (-gridSizeHalf) + alpha * gridSizeHalf;
        glVertex3f(-gridSizeHalf, 0, z);
        glVertex3f( gridSizeHalf, 0, z);
    }
    glEnd();
    prog_simple->unbind();

    prog->bind();
    glLineWidth(1.0f); // for wireframe
    glUniformMatrix4fv(prog->getUniform("P"),  1, GL_FALSE, (float*)&P->topMatrix());
    glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, (float*)&MV->topMatrix());
    glUniform3f(prog->getUniform("ka"), 0.1f, 0.1f, 0.1f);
    glUniform3f(prog->getUniform("ks"), 0.1f, 0.1f, 0.1f);
    glUniform1f(prog->getUniform("s"), 200.0f);
    cylinder.setProgram(prog);
    cylinder.draw(0);
    prog->unbind();


}

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Usage: FinalProject <RESOURCE DIR>" << endl;
        return 0;
    }
    RESOURCE_DIR = argv[1] + string("/");

    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        return -1;

    window = glfwCreateWindow(640, 480, "Donato Curvino", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = true;
    if(glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    glGetError(); // A bug in glewInit() causes an error that we can safely ignore.
    cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

    glfwSwapInterval(1);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCharCallback(window, char_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    init();
    while(!glfwWindowShouldClose(window)) {
        if (!glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
            render();
            glfwSwapBuffers(window);
        }
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}
