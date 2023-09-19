#include "stdafx.h"
#include "MarchingCubes.h"

// [Public methods]

MarchingCubes::MarchingCubes()
{
}

MarchingCubes::~MarchingCubes()
{
}

void MarchingCubes::printTriangles(const std::vector<Triangle3D>& triangles)
{
    for (size_t i = 0; i < triangles.size(); i++)
    {
        for (int j = 0; j < 3; j++)
            std::cout << triangles[i].getPoint(j).x << ",\t" << triangles[i].getPoint(j).y << ",\t" << triangles[i].getPoint(j).z << "\n";

        std::cout << "\n";
    }
}

void MarchingCubes::triangulateField(std::vector<std::vector<std::vector<float>>>& scalarFunction, const uvec3& numDivs, float isovalue, std::vector<Triangle3D>& triangles)
{
    for (int i = 0; i + 1 < numDivs.x; i++)
    {
        for (int j = 0; j + 1 < numDivs.y; j++)
        {
            for (int k = 0; k + 1 < numDivs.z; k++)
            {
                float x = i, y = j, z = k;
                // Cell ordered according to convention in referenced website
                GridCell cell =
                {
                    {
                        {x, y, z}, {x + 1.0f, y, z},
                        {x + 1.0f, y, z + 1.0f}, {x, y, z + 1.0f},
                        {x, y + 1.0f, z}, {x + 1.0f, y + 1.0f, z},
                        {x + 1.0f, y + 1.0f, z + 1.0f}, {x, y + 1.0f, z + 1.0f}
                    },
                    {
                        scalarFunction[i][j][k], scalarFunction[i + 1][j][k],
                        scalarFunction[i + 1][j][k + 1], scalarFunction[i][j][k + 1],
                        scalarFunction[i][j + 1][k], scalarFunction[i + 1][j + 1][k],
                        scalarFunction[i + 1][j + 1][k + 1], scalarFunction[i][j + 1][k + 1]
                    }
                };

                std::vector<Triangle3D> cellTriangles;
                triangulateCell(cell, isovalue, cellTriangles);
                triangles.insert(triangles.end(), cellTriangles.begin(), cellTriangles.end());
            }
        }
    }
}

// [Protected methods]

int MarchingCubes::calculateCubeIndex(GridCell& cell, float isovalue)
{
    int cubeIndex = 0;
    for (int i = 0; i < 8; i++)
        if (cell._value[i] < isovalue) cubeIndex |= (1 << i);

    return cubeIndex;
}

std::vector<vec3> MarchingCubes::getIntersectionCoordinates(GridCell& cell, float isovalue)
{
    std::vector<vec3> intersections(12);

    int cubeIndex = calculateCubeIndex(cell, isovalue);
    int intersectionsKey = _edgeTable[cubeIndex];

    int idx = 0;
    while (intersectionsKey)
    {
        if (intersectionsKey & 1)
        {
            int v1 = _edgeToVertices[idx].first, v2 = _edgeToVertices[idx].second;
            vec3 intersectionPoint = interpolate(cell._vertex[v1], cell._value[v1], cell._vertex[v2], cell._value[v2], isovalue);
            intersections[idx] = intersectionPoint;
        }

        idx++;
        intersectionsKey >>= 1;
    }


    return intersections;
}

void MarchingCubes::getTriangles(std::vector<vec3>& intersections, int cubeIndex, std::vector<Triangle3D>& triangles)
{
    for (int i = 0; _triangleTable[cubeIndex][i] != -1; i += 3)
    {
        triangles.push_back(Triangle3D(
            intersections[_triangleTable[cubeIndex][i + 0]], 
            intersections[_triangleTable[cubeIndex][i + 1]], 
            intersections[_triangleTable[cubeIndex][i + 2]]));
    }
}

vec3 MarchingCubes::interpolate(vec3& v1, float val1, vec3& v2, float val2, float isovalue)
{
    vec3 interpolated;
    float mu = (isovalue - val1) / (val2 - val1);

    interpolated.x = mu * (v2.x - v1.x) + v1.x;
    interpolated.y = mu * (v2.y - v1.y) + v1.y;
    interpolated.z = mu * (v2.z - v1.z) + v1.z;

    return interpolated;
}

void MarchingCubes::triangulateCell(GridCell& cell, float isovalue, std::vector<Triangle3D>& triangles)
{
    int cubeIndex = calculateCubeIndex(cell, isovalue);
    std::vector<vec3> intersections = getIntersectionCoordinates(cell, isovalue);
    getTriangles(intersections, cubeIndex, triangles);
}