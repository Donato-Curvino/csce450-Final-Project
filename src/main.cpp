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
#include "spline.h"

#ifndef NDEBUG
    #define GL_ERR GLSL::checkError(GET_FILE_LINE);
#else
    #define GL_ERR ;
#endif


using namespace std;

GLFWwindow* window;
GLuint gridID;
GLuint gridVAO;
string RESOURCE_DIR = "";
bool keyToggles[256] = {false};

shared_ptr<Camera> camera = NULL;
shared_ptr<Program> prog;
shared_ptr<Program> prog_simple;
ShapeSkin cylinder;
double t;
double t0;

Spline spline;

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
    // keyToggles[(unsigned)'c'] = true;

    camera = make_shared<Camera>();

    glClearColor(1.f, 1.f, 1.f, 1.f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // For drawing the grid, etc.
    prog_simple = make_shared<Program>();
    prog_simple->setShaderNames(RESOURCE_DIR + "simple_vert.glsl", RESOURCE_DIR + "simple_frag.glsl");
    prog_simple->setVerbose(true);
    prog_simple->init();
    prog_simple->addAttribute("vertex");
    prog_simple->addUniform("MV");
    prog_simple->addUniform("P");

    GLint vtx = prog_simple->getAttribute("vertex");
    // Generate Vertex Array Object
    glGenVertexArrays(1, &gridVAO);                                 GL_ERR;
    glBindVertexArray(gridVAO);                                     GL_ERR;
    // Data Allocation
    glGenBuffers(1, &gridID);                                       GL_ERR;
    glBindBuffer(GL_ARRAY_BUFFER, gridID);                          GL_ERR;
    glVertexAttribPointer(vtx, 3, GL_FLOAT, GL_FALSE, 0, nullptr);  GL_ERR;
    glEnableVertexAttribArray(vtx);                                 GL_ERR;
    glBindVertexArray(0);                                           GL_ERR;

    prog = make_shared<Program>();
    prog->setVerbose(true);
    prog->setShaderNames(RESOURCE_DIR + "vert.glsl", RESOURCE_DIR + "frag.glsl");
    prog->init();

    prog->addAttribute("aPos");
    prog->addAttribute("aNor");
    // prog->addAttribute("aTex");
    prog->addAttribute("u");
    // prog->addAttribute("bw");
    // prog->addAttribute("bi");
    prog->addUniform("P");
    prog->addUniform("MV");
    prog->addUniform("G");
    prog->addUniform("B");
    prog->addUniform("bone");

    prog->addUniform("lightPos");
    prog->addUniform("ka");
    prog->addUniform("ks");
    prog->addUniform("s");
    // prog->addUniform("kdTex");
    prog->addUniform("kd");

    // Bind the texture to unit 1.
    // int unit = 1;
    // prog->bind();
    // glUniform1i(prog->getUniform("kdTex"), unit);
    // prog->unbind();

    // Generate cylinder
    cylinder.makeCylinder(20, 50, 10);
    cylinder.bindSkin();
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
    static vector<glm::vec3> grid;
    prog_simple->bind();
    glUniformMatrix4fv(prog_simple->getUniform("P"),  1, GL_FALSE, (float*)&P->topMatrix());
    glUniformMatrix4fv(prog_simple->getUniform("MV"), 1, GL_FALSE, (float*)&MV->topMatrix());
    float gridSizeHalf = 200.0f;
    int gridNx = 11;
    int gridNz = 11;
    grid.clear();
    grid.reserve(gridNx * gridNz);
    for(int i = 0; i < gridNx; ++i) {
        float alpha = i / (gridNx - 1.0f);
        float x = (1.0f - alpha) * (-gridSizeHalf) + alpha * gridSizeHalf;
        grid.emplace_back(x, 0, -gridSizeHalf);
        grid.emplace_back(x, 0,  gridSizeHalf);
    }
    for(int i = 0; i < gridNz; ++i) {
        float alpha = i / (gridNz - 1.0f);
        float z = (1.0f - alpha) * (-gridSizeHalf) + alpha * gridSizeHalf;
        grid.emplace_back(-gridSizeHalf, 0, z);
        grid.emplace_back( gridSizeHalf, 0, z);
    }
    int vtx = prog_simple->getAttribute("vertex");
    glEnableVertexAttribArray(vtx);                                     GL_ERR;
    glBindBuffer(GL_ARRAY_BUFFER, gridID);                              GL_ERR;
    glBufferData(GL_ARRAY_BUFFER, grid.size() * 3 * sizeof(float), grid.data(), GL_STATIC_DRAW);    GL_ERR;
    glBindVertexArray(gridVAO);                                         GL_ERR;
    glVertexAttribPointer(vtx, 3, GL_FLOAT, GL_FALSE, 0, nullptr);      GL_ERR;
    glDrawArrays(GL_LINES, 0, grid.size());                             GL_ERR;
    glBindBuffer(GL_ARRAY_BUFFER, 0);                                   GL_ERR;
    glDisableVertexAttribArray(vtx);                                    GL_ERR;
    prog_simple->unbind();

    prog->bind();
    // glLineWidth(1.0f); // for wireframe
    glUniformMatrix4fv(prog->getUniform("P"),  1, GL_FALSE, (float*)&P->topMatrix());   GL_ERR;
    glUniformMatrix4fv(prog->getUniform("MV"), 1, GL_FALSE, (float*)&MV->topMatrix());  GL_ERR;
    glUniform3f(prog->getUniform("ka"), 0.1f, 0.1f, 0.1f);                              GL_ERR;
    glUniform3f(prog->getUniform("ks"), 0.1f, 0.1f, 0.1f);                              GL_ERR;
    glUniform4f(prog->getUniform("kd"), 0.0f, 0.0f, 0.3f, 1.0f);                        GL_ERR;
    glUniform1f(prog->getUniform("s"), 200.0f);                                         GL_ERR;
    glUniform3f(prog->getUniform("lightPos"), 1, 1, -1);                                GL_ERR;
    cylinder.setProgram(prog);
    cylinder.draw(0);
    prog->unbind();

    prog_simple->bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    // cylinder.getSpline(0).draw();
    prog_simple->unbind();
    GLSL::checkError(GET_FILE_LINE);
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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLSL::checkError(GET_FILE_LINE);
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
