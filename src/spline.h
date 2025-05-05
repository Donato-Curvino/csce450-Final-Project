#ifndef SPLINE_H
#define SPLINE_H

#include <vector>
#include <glm/glm.hpp>

struct Spline {
    glm::mat4 B;
    glm::mat4x3 G;

    static constexpr glm::mat4 BEZIER = {
         1.f,  0.f,  0.f,  0.f,
        -3.f,  3.f,  0.f,  0.f,
         3.f, -6.f,  3.f,  0.f,
        -1.f,  3.f, -3.f,  1.f
    };

  public:
    Spline() : B{BEZIER} {}
    Spline(glm::mat4x3 g) : B{BEZIER}, G{g} {}

    void draw() const;
};

#endif // SPLINE_H
