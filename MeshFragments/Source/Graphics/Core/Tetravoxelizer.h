/*
  Created by Antonio J. Rueda on 13/09/2019.
  Copyright © 2019 Antonio Jesús Rueda Ruiz

 Redistribution and use in source and binary forms, with or without modification, are
 permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this list of
 conditions and the following disclaimer.
 Redistributions in binary form must reproduce the above copyright notice, this list of
 conditions and the following disclaimer in the documentation and/or other materials provided
 with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "Model3D.h"

// Activates an optimization that only sends tetrahedra to the GPU that are intersected
// by the current slice. Requires sorting all the tetrahedra by their highest Y-coordinate
#define FILTER_TETRAHEDRA_BY_Y

/**
	Fast and simple hardware-accelerated voxelizer based on the algorithm described in:

	Ogayar, C. J., Rueda, A. J., Segura, R. J., Feito, F. R., Fast and Simple Hardware Accelerated Voxelizations using Simplicial Coverings. Visual Computer, 23, pp. 535-543, 2007.

	The original suggested implementation has been improved according to modern OpenGL.
 */
class Tetravoxelizer {
	/** VAO & vertex buffers */
	GLuint VAO, tetrahedraVerticesVBO;

	/** Shader program & parameters */
	GLuint voxelizerProgram;
	GLuint sliceParam;

	/** Renderbuffer object and framebuffer object for results */
	GLuint resultRBO;
	GLuint resultFBO;
	GLuint resultTexture;

	// Tetrahedra vertices
	std::vector<glm::vec3> tetrahedraVertices;

	// Resolution
	ivec3 res;

	/** Shader reader
	 Creates null terminated string from file */
	void checkError(GLint status, const char* msg);

	/** Compile error printing function */
	void checkShaderError(GLint status, GLint shader, const char* msg);

	/** Program link error printing function */
	void checkProgramError(GLint status, GLint program, const char* msg);

	/** Scale vertex to given resolution */
	static glm::vec3 scaleToNDC(const glm::vec3& v, const glm::vec3& centerBBox, const glm::vec3& dimBBox) {
		return 2.0f * (v - centerBBox) / dimBBox;
	}

	/** Sorting network for 4 elements */
	static void sort4Vec3ByLowerY(std::vector<glm::vec3>::iterator v) {
		if (v[0].y > v[1].y) std::swap(v[0], v[1]);
		if (v[2].y > v[3].y) std::swap(v[2], v[3]);
		if (v[0].y > v[2].y) std::swap(v[0], v[2]);
		if (v[1].y > v[3].y) std::swap(v[1], v[3]);
		if (v[1].y > v[2].y) std::swap(v[1], v[2]);
	}

	/** Sorting two tetrahedra by its fourth point (with highest Y) */
	static int sortTetrahedraByLowerHighestY(const void* a, const void* b) {
		glm::vec3* vA = (glm::vec3*)a;
		glm::vec3* vB = (glm::vec3*)b;

		return (vA + 3)->y - (vB + 3)->y;
	}

public:
	Tetravoxelizer() { }

	/** Initialize voxelizer. Do not call several times without deleting resources */
	void initialize(const ivec3& res);

	/** Prepare model. Do not call several times without deleting resources */
	void initializeModel(const std::vector<Model3D::VertexGPUData>& meshVertices, const std::vector<Model3D::FaceGPUData>& meshFaces, const AABB& aabb);

	/* Perform voxelization */
	void compute(std::vector<unsigned char>& result);

	/** Delete model resources */
	void deleteModelResources();

	/** Delete resources */
	void deleteResources();
};
