#include "Skeleton.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <GL/glew.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

void Skeleton::loadSkeleton(const std::string& skel_name) {
    std::ifstream fin(skel_name);
    if (!fin.good()) {
        std::cout << "Cannot read " << skel_name << std::endl;
        return;
    }

    int nframes = 0;
    int nbones = 0;
    int f = 0;
    std::string line;
    while (getline(fin, line)) {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream stin(line);
        if (nframes == 0 || nbones == 0) {
            stin >> nframes >> nbones;
            frames.resize(nframes + 1);
        } else {
            std::vector<glm::mat4>& frame = frames[f];
            frame.resize(nbones + 1);

            for (int b = 0; b < nbones; b++) {
                glm::quat q;
                glm::vec3 pos;
                stin >> q.x >> q.y >> q.z >> q.w
                    >> pos.x >> pos.y >> pos.z;
                if (!stin.good()) {
                    std::cout << "Error parsing " << skel_name << std::endl;
                    return;
                }
                frame[b] = glm::mat4_cast(q);
                frame[b][3] = glm::vec4(pos, 1.f);
            }

            f++;
        }
    }
}

void Skeleton::draw(int k) {
    // for (unsigned int j = 0; j < frames[k].size(); j++) {
    const glm::mat2x4 frame[3] = {{
        0, 0, 0, 1,
        5, 0, 0, 1
    }, {
        0, 0, 0, 1,
        0, 5, 0, 1
    }, {
        0, 0, 0, 1,
        0, 0, 5, 1
    }};
    const glm::vec3 colors[3] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };

    glLineWidth(2);
    glBegin(GL_LINES);
    for (glm::mat4& bone : frames[k]) {
        for (int i = 0; i < 3; i++) {
           glColor3fv(&colors[i].r);
           glm::vec4 p0 = bone * frame[i][0];
           glm::vec4 p1 = bone * frame[i][1];
           glVertex3fv(&p0.x);
           glVertex3fv(&p1.x);
        }
    }
    glEnd();
}
