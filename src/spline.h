#ifndef SPLINE_H
#define SPLINE_H

#include <vector>
#include <glm/glm.hpp>

struct Spline {
    glm::mat4 B;
    std::vector<glm::vec3> cps;

  public:
    Spline();

    void draw() const;
};

#endif // SPLINE_H
