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

#include "stdafx.h"
#include "Tetravoxelizer.h"

// Simple vertex shader for voxelization
static const char* voxelVertSource = R"(
    #version 330

    layout(location = 1) in vec3 tVertex;

    void main() {
        gl_Position = vec4(tVertex, 1.0);
    }
)";

// Geometry shader for voxelization
// Receives the 4 vertices of a tetrahedron and
// computes an slice (one or two triangles)
static const char* voxelGeomSource = R"(
    #version 330

    #define INTERP(A,B,s) (mix(A, B, (s - A[1]) / (B[1] - A[1])))

    #define tVertexA gl_in[0].gl_Position
    #define tVertexB gl_in[1].gl_Position
    #define tVertexC gl_in[2].gl_Position
    #define tVertexD gl_in[3].gl_Position

    layout(lines_adjacency) in;
    layout(triangle_strip, max_vertices = 6) out;

    uniform float slice;

    void main() {
        
        if (tVertexA.y < slice && slice <= tVertexD.y) {
            gl_Position = vec4(INTERP(tVertexA, tVertexD, slice).xz, 0.0, 1.0);
            EmitVertex();
            
            vec4 v1 = (slice <= tVertexB.y) ?
                vec4(INTERP(tVertexA, tVertexB, slice).xz, 0.0, 1.0) :
                vec4(INTERP(tVertexB, tVertexD, slice).xz, 0.0, 1.0);
            gl_Position = v1;
            EmitVertex();

            vec4 v2 = (slice <= tVertexC.y) ?
                vec4(INTERP(tVertexA, tVertexC, slice).xz, 0.0, 1.0) :
                vec4(INTERP(tVertexC, tVertexD, slice).xz, 0.0, 1.0);
            gl_Position = v2;
            EmitVertex();
            
            EndPrimitive();

            // Extra triangle between vertices B and C
            if (tVertexB.y < slice && slice <= tVertexC.y) {
                gl_Position = vec4(INTERP(tVertexB, tVertexC, slice).xz, 0.0, 1.0);
                EmitVertex();
                
                gl_Position = v2;
                EmitVertex();
                
                gl_Position = v1;
                EmitVertex();
                
                EndPrimitive();
            }
        }
    }
)";

// Simple fragment shader for voxelization
static const char* voxelFragSource = R"(
    #version 330

    out int result;

    void main() {
        result = 1;
    }
)";

void Tetravoxelizer::checkError(GLint status, const char* msg)
{
	if (!status)
	{
		std::cerr << msg << std::endl;
		exit(EXIT_FAILURE);
	}
}

void Tetravoxelizer::checkShaderError(GLint status, GLint shader, const char* msg)
{
	if (status == GL_FALSE) {
		std::cerr << msg << std::endl;

		char log[1024];
		GLsizei written;
		glGetShaderInfoLog(shader, sizeof(log), &written, log);
		std::cerr << "Shader log:" << log << std::endl;
	}
}

void Tetravoxelizer::checkProgramError(GLint status, GLint program, const char* msg)
{
	if (status == GL_FALSE) {
		std::cerr << msg << std::endl;

		char log[1024];
		GLsizei written;
		glGetProgramInfoLog(program, sizeof(log), &written, log);
		std::cerr << "Program log:" << log << std::endl;
	}
}

void Tetravoxelizer::initialize(const ivec3& res)
{
	this->res = res;

	/** Prepare shaders ************************************************/
	GLint status;

	// Create program and shader objects
	GLint vShader = glCreateShader(GL_VERTEX_SHADER);
	GLint gShader = glCreateShader(GL_GEOMETRY_SHADER);
	GLint fShader = glCreateShader(GL_FRAGMENT_SHADER);
	voxelizerProgram = glCreateProgram();

	// Attach shaders to the program object
	glAttachShader(voxelizerProgram, vShader);
	glAttachShader(voxelizerProgram, gShader);
	glAttachShader(voxelizerProgram, fShader);

	// Read shaders
	glShaderSource(vShader, 1, (const GLchar**)&voxelVertSource, 0);
	glShaderSource(gShader, 1, (const GLchar**)&voxelGeomSource, 0);
	glShaderSource(fShader, 1, (const GLchar**)&voxelFragSource, 0);

	// Compile shaders
	glCompileShader(vShader);
	glGetShaderiv(vShader, GL_COMPILE_STATUS, &status);
	checkShaderError(status, vShader, "Failed to compile the vertex shader.");

	glCompileShader(gShader);
	glGetShaderiv(gShader, GL_COMPILE_STATUS, &status);
	checkShaderError(status, gShader, "Failed to compile the geometry shader.");

	glCompileShader(fShader);
	glGetShaderiv(fShader, GL_COMPILE_STATUS, &status);
	checkShaderError(status, fShader, "Failed to compile the fragment shader.");

	// Link shader program
	glLinkProgram(voxelizerProgram);
	glGetProgramiv(voxelizerProgram, GL_LINK_STATUS, &status);
	checkProgramError(status, voxelizerProgram, "Failed to link the shader program object.");

	// Get param ids
	sliceParam = glGetUniformLocation(voxelizerProgram, "slice");

	/** Prepare render target to texture *********************************************/

	// Renderbuffer for voxelization results
	glGenRenderbuffers(1, &resultRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, resultRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_R8UI, res.x, res.z);

	// Framebuffer for rendering to renderbuffer
	glGenFramebuffers(1, &resultFBO); // frame buffer id
	glBindFramebuffer(GL_FRAMEBUFFER, resultFBO);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, resultRBO);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0, res.x, res.z);
}

void Tetravoxelizer::initializeModel(const std::vector<Model3D::VertexGPUData>& meshVertices, const std::vector<Model3D::FaceGPUData>& meshFaces, const AABB& aabb)
{
	// Prepare vertex data *************************************************/
	// 1.- Collect tetrahedra vertices: 3 vertices from each input model face + centroid
	// 2.- Translate and scale vertices to the voxelization space
	// 3.- Sort tetrahedra vertices by its Y coordinate

	// Compute minimal bounding box
	glm::vec3 centroid = glm::vec3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i < meshVertices.size(); ++i)
		centroid += meshVertices[i]._position;

	// Compute centroid
	centroid /= meshVertices.size();

	glm::vec3 dimBBox = aabb.max() - aabb.min();
	glm::vec3 maxDimBBox = aabb.size();
	glm::vec3 centerBBox = 0.5f * (aabb.min() + aabb.max());
	centroid = scaleToNDC(centroid, centerBBox, maxDimBBox);
	tetrahedraVertices.resize(meshFaces.size() * 4);

#pragma omp parallel for
	for (int i = 0; i < meshFaces.size(); ++i) {
		tetrahedraVertices[i * 4 + 0] = scaleToNDC(meshVertices[meshFaces[i]._vertices.x]._position, centerBBox, maxDimBBox);
		tetrahedraVertices[i * 4 + 1] = scaleToNDC(meshVertices[meshFaces[i]._vertices.y]._position, centerBBox, maxDimBBox);
		tetrahedraVertices[i * 4 + 2] = scaleToNDC(meshVertices[meshFaces[i]._vertices.z]._position, centerBBox, maxDimBBox);
		tetrahedraVertices[i * 4 + 3] = centroid;

		sort4Vec3ByLowerY(tetrahedraVertices.begin() + i * 4);
	}

#ifdef FILTER_TETRAHEDRA_BY_Y
	qsort(&tetrahedraVertices[0], tetrahedraVertices.size() / 4, 4 * sizeof(glm::vec3), sortTetrahedraByLowerHighestY);
#endif

	/** Prepare VAO for rendering ******************************************/

	// Generate VAO and VBOs ids
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &tetrahedraVerticesVBO);

	// Activate VAO, bind VBOs and transfer data
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, tetrahedraVerticesVBO);
	glBufferData(GL_ARRAY_BUFFER, tetrahedraVertices.size() * sizeof(glm::vec3), &tetrahedraVertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
}

void Tetravoxelizer::deleteResources() {
	glDeleteProgram(voxelizerProgram);
	glDeleteRenderbuffers(1, &resultRBO);
	glDeleteFramebuffers(1, &resultFBO);
	glDeleteBuffers(1, &tetrahedraVerticesVBO);
	glDeleteVertexArrays(1, &VAO);
}

void Tetravoxelizer::deleteModelResources() {
	tetrahedraVertices.clear();

	glDeleteBuffers(1, &tetrahedraVerticesVBO);
	glDeleteVertexArrays(1, &VAO);
}

void Tetravoxelizer::compute(std::vector<unsigned char>& result)
{
	// Activate render to texture
	glBindFramebuffer(GL_FRAMEBUFFER, resultFBO);

	glClearColor(0, 0, 0, 0);
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_XOR);
	glDisable(GL_DEPTH_TEST);

	// Activate vertex array
	glBindVertexArray(VAO);

	// Activate the program
	glUseProgram(voxelizerProgram);

	float ySlice = -1.0f;
	float yStep = 2.0f / res.y;
	int firstTetrahedraIndex = 0;
	int numVertices = (int)tetrahedraVertices.size();

	// Render slices
	for (int slice = 0; slice < res.y; ++slice) {
		glClear(GL_COLOR_BUFFER_BIT);
		glUniform1f(sliceParam, ySlice);
		// We use GL_LINES_ADJACENCY just to supply the four tetrahedron vertices to the geometry shader
		// Then this shader generates one or two triangles by interpolating its vertices in the current slice
		glDrawArrays(GL_LINES_ADJACENCY, firstTetrahedraIndex, numVertices);

		ySlice += yStep;

#ifdef FILTER_TETRAHEDRA_BY_Y
		// Discard tetrahedra not intersected by the slice
		while (firstTetrahedraIndex < numVertices && tetrahedraVertices[firstTetrahedraIndex + 3].y < ySlice) {
			firstTetrahedraIndex += 4;
			numVertices -= 4;
		}
#endif
		// Transfer results to CPU memory buffer
		glReadPixels(0, 0, res.x, res.z, GL_RED_INTEGER, GL_UNSIGNED_BYTE, &result[res.x * res.z * slice]);
		glFinish();
	}

	glDisable(GL_COLOR_LOGIC_OP);

	// Deactivate program
	glUseProgram(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glEnable(GL_DEPTH_TEST);
}







