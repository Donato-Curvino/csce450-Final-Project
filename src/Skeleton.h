#pragma once
#ifndef SKELETON_H
#define SKELETON_H

#include <vector>
#include <string>
#include <glm/mat4x4.hpp>

#include "spline.h"

class Skeleton {
  private:
    std::vector< std::vector<glm::mat4> > frames;   //[frame][bone]
    // std::vector<glm::mat4> bind_pose;
    // std::vector<glm::mat4> pose;
    std::vector<Spline> splines;
    std::vector<glm::mat2x4> bones;

  public:
    size_t numFrames() const { return frames.size(); }
    size_t numBones() const  { return frames.empty() ? 0 : frames[0].size(); }
    void loadSkeleton(const std::string& skel_name);
    void draw(int k);

    friend class ShapeSkin;
};

#endif //SKELETON_H

/* splines defined in skeleton file in sets of 4 points
 * in bind pose, each point is duplicated b/c splies are straight
 *   2nd point is the base of the bone
 *   3rd point is at the end of the bone
 *
 *   TODO: relative or absolute coords?
 */
