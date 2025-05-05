#include "spline.h"

#include <GL/glew.h>

void Spline::draw() const {
    // glColor3f(0.f, 0.f, 0.f);
    glBegin(GL_LINE_STRIP);

    unsigned int ncps = 4;
    glm::mat4 G;
    glm::vec4 uVec;
    unsigned int p0 = 0;
    do {
        G = {glm::vec4(this->G[p0 + 0], 0.f), glm::vec4(this->G[p0 + 1], 0.f), glm::vec4(this->G[p0 + 2], 0.f), glm::vec4(this->G[p0 + 3], 0.f)};
        for (float u = 0.f; u <= 1; u += 0.02) {
            uVec = {1.f, u, u*u, u*u*u};
            glm::vec4 p = G * (B * uVec);
            glVertex3f(p.x, p.y, p.z);
        }

    } while ((++p0 + 4) <= ncps);
    glEnd();
}
