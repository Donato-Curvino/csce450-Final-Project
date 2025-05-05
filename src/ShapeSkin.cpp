#include <iostream>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "ShapeSkin.h"
#include "GLSL.h"
#include "Program.h"
#include "TextureMatrix.h"

using namespace std;
using namespace glm;

ShapeSkin::ShapeSkin() :
	prog(NULL),
	elemBufID(0),
	posBufID(0),
	norBufID(0),
	texBufID(0)
{
	T = make_shared<TextureMatrix>();
}

ShapeSkin::~ShapeSkin()
{
}

void ShapeSkin::setTextureMatrixType(const std::string &meshName)
{
	T->setType(meshName);
}

void ShapeSkin::loadMesh(const string &meshName)
{
	// Load geometry
	// This works only if the OBJ file has the same indices for v/n/t.
	// In other words, the 'f' lines must look like:
	// f 70/70/70 41/41/41 67/67/67
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	string warnStr, errStr;
	bool rc = tinyobj::LoadObj(&attrib, &shapes, &materials, &warnStr, &errStr, meshName.c_str());
	if(!rc) {
		cerr << errStr << endl;
	} else {
		posBuf = attrib.vertices;
		norBuf = attrib.normals;
		texBuf = attrib.texcoords;
		assert(posBuf.size() == norBuf.size());
		// Loop over shapes
		for(size_t s = 0; s < shapes.size(); s++) {
			// Loop over faces (polygons)
			const tinyobj::mesh_t &mesh = shapes[s].mesh;
			size_t index_offset = 0;
			for(size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
				size_t fv = mesh.num_face_vertices[f];
				// Loop over vertices in the face.
				for(size_t v = 0; v < fv; v++) {
					// access to vertex
					tinyobj::index_t idx = mesh.indices[index_offset + v];
					elemBuf.push_back(idx.vertex_index);
				}
				index_offset += fv;
				// per-face material (IGNORE)
				//shapes[s].mesh.material_ids[f];
			}
		}
	}
}

void ShapeSkin::loadAttachment(const std::string &filename)
{
	// TODO	
    std::ifstream fin(filename);
    if (!fin.good()) {
        std::cout << "Cannot read " << filename << std::endl;
        return;
    }

    int vertCount = 0;
    int boneCount = 0;
    int maxInfluences = 0;

    int i = 0;
    std::string line;
    while (getline(fin, line)) {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream stin(line);
        if (!vertCount || !boneCount || !maxInfluences) {
            if (!(stin >> vertCount >> boneCount >> maxInfluences)) {
                std::cout << "Error parsing " << filename << std::endl;
                return;
            }
            if (vertCount != (posBuf.size() / 3)) {
                std::cout << "Incorrect number of verticies in " << filename << std::endl;
                return;
            }

            // bone_indicies.resize(vertCount, std::vector<int>(maxInfluences, 0));
            // weights.resize(vertCount, std::vector<float>(maxInfluences, 0.f));
            continue;
        }

        int influences;
        if (!(stin >> influences)) {
            std::cout << "Error parsing " << filename << std::endl;
            return;
        }
        if (influences <= 0 || influences > maxInfluences) {
            std::cout << "Bad number of influences " << std::endl;
            return;
        }

        for (int j = 0; j < influences; j++) {
            if (!(stin >> bone_indicies[i][j] >> weights[i][j])) {
                std::cout << "Error parsing " << filename << std::endl;
                return;
            }
        }

        i++;
    }
}

void ShapeSkin::init() {
    GLSL::checkError(GET_FILE_LINE);
	// Send the position array to the GPU
	glGenBuffers(1, &posBufID);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
    // glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, bindings.size()*sizeof(glm::vec3), &bindings[0], GL_STATIC_DRAW);
    GLSL::checkError(GET_FILE_LINE);

	// Send the normal array to the GPU
	glGenBuffers(1, &norBufID);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
    // glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, bind_norms.size() * sizeof(glm::vec3), &bind_norms[0], GL_STATIC_DRAW);
	
	// Send the texcoord array to the GPU
	glGenBuffers(1, &texBufID);
	glBindBuffer(GL_ARRAY_BUFFER, texBufID);
	glBufferData(GL_ARRAY_BUFFER, texBuf.size()*sizeof(float), &texBuf[0], GL_STATIC_DRAW);
	
	// Send the element array to the GPU
	glGenBuffers(1, &elemBufID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elemBuf.size()*sizeof(unsigned int), &elemBuf[0], GL_STATIC_DRAW);

    // send u array to GPU
    glGenBuffers(1, &uBufID);
    glBindBuffer(GL_ARRAY_BUFFER, uBufID);
    glBufferData(GL_ARRAY_BUFFER, uBuf.size() * sizeof(float), &uBuf[0], GL_STATIC_DRAW);
	
	// Unbind the arrays
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	GLSL::checkError(GET_FILE_LINE);
}

void ShapeSkin::update(int k)
{
	// TODO: CPU skinning calculations.
	// After computing the new positions and normals, send the new data
	// to the GPU by copying and pasting the relevant code from the
	// init() function.
    std::vector<glm::vec3> newPos(posBuf.size() / 3);
    std::vector<glm::vec3> newNor(norBuf.size() / 3);
    std::vector<glm::mat4> Mk(skel.numBones());
    // for (glm::mat4 M : skel->frames[k]) {
    for (unsigned int j = 0; j < skel.frames[k].size(); j++)
        Mk[j] = skel.frames[k][j] * glm::inverse(skel.frames[0][j]);
    // std::cout << glm::to_string(Mk[0]) << std::endl;

    for (unsigned int i = 0; i < posBuf.size() / 3; i++) {
        glm::mat4 sum(0);
        for (unsigned int j = 0; j < 4 && weights[i][j] != 0.f; j++)
            sum += weights[i][j] * Mk[bone_indicies[i][j]];
        newPos[i] = sum * glm::vec4(*(glm::vec3*)&posBuf[i*3], 1.f);
        newNor[i] = sum * glm::vec4(*(glm::vec3*)&norBuf[i*3], 0.f);
    }
	
    glBindBuffer(GL_ARRAY_BUFFER, posBufID);
    glBufferData(GL_ARRAY_BUFFER, posBuf.size() * sizeof(float), &newPos[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, norBufID);
    glBufferData(GL_ARRAY_BUFFER, norBuf.size() * sizeof(float), &newNor[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
	GLSL::checkError(GET_FILE_LINE);
}

void ShapeSkin::draw(int k) const {
	assert(prog);

	// Send texture matrix
    // glUniformMatrix3fv(prog->getUniform("T"), 1, GL_FALSE, glm::value_ptr(T->getMatrix()));
	int h_pos = prog->getAttribute("aPos");
	glEnableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, posBufID);
	glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

	int h_nor = prog->getAttribute("aNor");
	glEnableVertexAttribArray(h_nor);
	glBindBuffer(GL_ARRAY_BUFFER, norBufID);
	glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

    // int h_tex = prog->getAttribute("aTex");
    // glEnableVertexAttribArray(h_tex);
    // glBindBuffer(GL_ARRAY_BUFFER, texBufID);
    // glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);
    // GLSL::checkError(GET_FILE_LINE);

    int h_u = prog->getAttribute("u");
    glEnableVertexAttribArray(h_u);
    glBindBuffer(GL_ARRAY_BUFFER, uBufID);
    glVertexAttribPointer(h_u, 1, GL_FLOAT, GL_FALSE, 0, nullptr);

    glUniformMatrix4fv(prog->getUniform("B"), 1, GL_FALSE, (float*)&skel.splines[0].B);
    glUniformMatrix2x4fv(prog->getUniform("bone"), 1, GL_FALSE, (float*)&skel.bones[0]);

    // glUniformMatrix4x3fv(prog->getUniform("G"), 1, GL_FALSE, &G[0][0]);
    glUniformMatrix4x3fv(prog->getUniform("G"), 1, GL_FALSE, (float*)&skel.splines[0].G[0][0]);
	
	// Draw
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elemBufID);
	glDrawElements(GL_TRIANGLES, (int)elemBuf.size(), GL_UNSIGNED_INT, (const void *)0);
	
	glDisableVertexAttribArray(h_nor);
	glDisableVertexAttribArray(h_pos);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	GLSL::checkError(GET_FILE_LINE);
}

void ShapeSkin::makeCylinder(unsigned int resolution, unsigned int height, unsigned int radius) {
    unsigned int rows = resolution;
    unsigned int cols = resolution;
    posBuf.resize((rows + 1) * (cols + 1) * 3);
    norBuf.resize((rows + 1) * (cols + 1) * 3);
    texBuf.resize((rows + 1) * (cols + 1) * 2);
    elemBuf.resize(rows * cols * 6);
    glm::vec3* pos_vecs = (glm::vec3*)posBuf.data();
    glm::vec3* nor_vecs = (glm::vec3*)norBuf.data();
    glm::vec2* tex_vecs = (glm::vec2*)texBuf.data();
    for (unsigned int y = 0; y <= rows; y++) {
        for (unsigned int x = 0; x <= cols; x++) {
            int i = y * (rows + 1) + x;
            float theta = 2.0f * x * (float)M_PI / cols;
            pos_vecs[i] = {
                radius * -glm::cos(theta),
                height * (float)y / rows,
                radius * glm::sin(theta)
            };
            nor_vecs[i] = glm::normalize(pos_vecs[i] - glm::vec3(0.f, pos_vecs[i].y, 0.f));
            tex_vecs[i] = {
                x * 1.f / cols,
                y * 1.f / rows
            };
            if (x < cols && y < rows) {
                int ind = 6 * (y * rows + x);
                elemBuf[ind    ] = elemBuf[ind + 3] = i;
                elemBuf[ind + 2] = elemBuf[ind + 4] = i + cols + 2;
                elemBuf[ind + 1] = i + 1;
                elemBuf[ind + 5] = i + cols + 1;
            }
        }
    }

    // create skeleton
    skel.bones.push_back(glm::mat2x4(
        glm::vec4(0.f, 0.f, 0.f, 0.f),
        glm::vec4(0.f, (float)height, 0.f, 0.f)
    ));

    // points.resize(nverts);
    // for (unsigned i = 0; i < nverts; i++)  points[i] = pos_vecs[i];
    // exportMesh("cylinder-og.obj");

    skel.splines.push_back(Spline());
    // skel.splines[0].cps = {
    //     glm::vec3(-50, 0, -50),
    //     glm::vec3(0, 0, 0),
    //     glm::vec3(0, 50, 0),
    //     glm::vec3(50, 50, 50)
    // };
    skel.splines[0].G = {
        glm::vec3{50, 0, 0},
        glm::vec3{50, 50, 0},
        glm::vec3{0, 50, 0},
        glm::vec3{0, 0, 0}
    };

    // skel.splines[0].cps = {
    //     glm::vec3(skel.bones[0][0]),
    //     glm::vec3(skel.bones[0][0]),
    //     glm::vec3(skel.bones[0][1]),
    //     glm::vec3(skel.bones[0][1])
    // };
}

void ShapeSkin::calcSplinePos() {

}

void ShapeSkin::exportMesh(const std::string& filename) {
    std::ofstream fout(filename, std::ios_base::out);
    if (fout.fail())    std::cerr << "can't open" << std::endl;

    for (size_t v = 0; v < points.size(); v ++)
        fout << "v " << points[v].x << ' ' << points[v].y << ' ' << points[v].z << std::endl;

    for (size_t f = 0; f < elemBuf.size() - 2; f += 3)
        fout << "f " << (elemBuf[f] + 1) << ' ' << (elemBuf[f + 1] + 1) << ' ' << (elemBuf[f + 2] + 1) << std::endl;
}

void ShapeSkin::bindSkin() {
    int nverts = posBuf.size() / 3;
    uBuf.resize(nverts);
    bindings.resize(nverts);
    bind_norms.resize(nverts);
    vec3* pos_vecs = (vec3*)posBuf.data();
    vec3* nor_vecs = (vec3*)norBuf.data();
    glm::mat2x4& bone = skel.bones[0];
    vec3 bone_vec = bone[1] - bone[0];

    // calculate u for each vertex
    for (unsigned int i = 0; i < nverts; i++) {
        glm::vec3 a = pos_vecs[i] - glm::vec3(bone[0]);
        uBuf[i] = glm::dot(a, glm::normalize(bone_vec)) / glm::length(bone_vec);
    }

    // calculate bind positions and normals
    for (int i = 0; i < nverts; i++) {
        float u = uBuf[i];

        vec4 uVec   = vec4(1, u, u*u, u*u*u);
        vec4 uVec_  = vec4(0, 1, 2*u, 3*u*u);

        // need original basis
        // can precalculate at bone base and translate by u
        vec3 pos_og   = bone * vec2(1 - u, u);

        vec3 tan_og   = normalize(bone[1] - bone[0]);
        vec3 bnorm_og = (tan_og.x == 0 && tan_og.z == 0)
            ? normalize(vec3{-tan_og.y, tan_og.x, 0})
            : normalize(vec3{-tan_og.z, 0, tan_og.x});
        vec3 norm_og = normalize(cross(bnorm_og, tan_og));
        mat4 basis_og = mat4(vec4(norm_og, 0), vec4(bnorm_og, 0), vec4(tan_og, 0), vec4(pos_og, 1));

        // TODO: may need to change
        mat4 basis_og_inv = inverse(basis_og);

        // binding positions with respect to spline frame
        bindings[i] = vec3{basis_og_inv * vec4(pos_vecs[i], 1)};
        bind_norms[i] = vec3{basis_og_inv * vec4(nor_vecs[i], 0)};
    }
}

void ShapeSkin::splineDeform(const Spline& s) {
    int nverts = posBuf.size() / 3;
    const mat4x3& G = s.G;
    const mat4&   B = s.B;
    mat4 GB = mat4(G * B);

    // std::ofstream cyl_out("cylinder.csv");
    // std::ofstream aPos_out("aPos.csv");

    for (int i = 0; i < nverts; i++) {
        float u = uBuf[i];

        vec4 uVec   = vec4(1, u, u*u, u*u*u);
        vec4 uVec_  = vec4(0, 1, 2*u, 3*u*u);

        // compute spline basis
        vec4 origin = GB * uVec;
        vec3 p_     = vec3(GB * uVec_);

        vec3 tan = (p_ == vec3{0}) ? vec3{1, 0, 0} : normalize(p_);
        vec3 bnorm = (tan.x == 0 && tan.z == 0)
            ? (u <= 0.5)
                ? normalize(cross(G[2] - G[1], G[1] - G[0]))
                : normalize(cross(G[3] - G[2], G[2] - G[1]))
            : normalize(vec3(-tan.z, 0, tan.x));
        vec3 norm  = normalize(cross(bnorm, tan));

        mat4 basis = mat4(vec4(norm, 0), vec4(bnorm, 0), vec4(tan, 0), origin);

        vec4 pos = basis * vec4{bindings[i], 1};
        // cyl_out << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
        // aPos_out << bindings[i].x << ", " << bindings[i].y << ", " << bindings[i].z << std::endl;
        points[i] = vec3(pos);

    }
    // exportMesh("spline-cylinder.obj");

}






















