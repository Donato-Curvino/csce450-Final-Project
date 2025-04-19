#pragma once
#ifndef SHAPESKIN_H
#define SHAPESKIN_H

#include <memory>
#include <vector>
#include <string>

#define GLEW_STATIC
#include <GL/glew.h>

#include <glm/mat4x4.hpp>

#include "Skeleton.h"

class MatrixStack;
class Program;
class TextureMatrix;

class ShapeSkin {
  private:
    std::shared_ptr<Program> prog;
    std::vector<unsigned int> elemBuf;
    std::vector<float> posBuf;
    std::vector<float> norBuf;
    std::vector<float> texBuf;
    std::vector<float> uBuf;
    GLuint elemBufID;
    GLuint posBufID;
    GLuint norBufID;
    GLuint texBufID;
    GLuint uBufID;
    std::string textureFilename;
    std::shared_ptr<TextureMatrix> T;

    Skeleton skel;
    std::vector<glm::ivec4> bone_indicies;
    std::vector<glm::vec4>  weights;

  public:
	ShapeSkin();
	virtual ~ShapeSkin();

	void setTextureMatrixType(const std::string &meshName);
	void loadMesh(const std::string &meshName);
	void loadAttachment(const std::string &filename);
    void init();

	void setProgram(std::shared_ptr<Program> p) { prog = p; }
	void update(int k);
	void draw(int k) const;
	void setTextureFilename(const std::string &f) { textureFilename = f; }
	std::string getTextureFilename() const { return textureFilename; }
	std::shared_ptr<TextureMatrix> getTextureMatrix() { return T; }

    // void setSkeleton(Skeleton* sk) { skel = sk; }

    void makeCylinder(unsigned int resolution, unsigned int height = 1, unsigned int radius = 1);
    void calcSplinePos();
};

#endif
