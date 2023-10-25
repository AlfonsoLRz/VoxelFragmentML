#include "stdafx.h"
#include "MarchingCubes.h"

#include "Graphics/Core/ShaderList.h"

const int MarchingCubes::_triangleTable[256 * 16] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1,
        3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1,
        3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1,
        3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1,
        9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1,
        1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1,
        9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1,
        2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1,
        8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1,
        9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1,
        4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1,
        3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1,
        1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1,
        4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1,
        4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1,
        9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1,
        1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1,
        5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1,
        2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1,
        9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1,
        0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1,
        2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1,
        10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1,
        4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1,
        5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1,
        5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1,
        9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1,
        0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1,
        1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1,
        10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1,
        8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1,
        2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1,
        7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1,
        9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1,
        2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1,
        11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1,
        9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1,
        5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1,
        11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1,
        11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1,
        1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1,
        9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1,
        5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1,
        2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1,
        0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1,
        5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1,
        6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1,
        0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1,
        3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1,
        6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1,
        5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1,
        1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1,
        10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1,
        6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1,
        1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1,
        8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1,
        7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1,
        3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1,
        5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1,
        0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1,
        9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1,
        8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1,
        5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1,
        0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1,
        6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1,
        10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1,
        10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1,
        8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1,
        1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1,
        3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1,
        0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1,
        10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1,
        0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1,
        3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1,
        6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1,
        9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1,
        8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1,
        3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1,
        6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1,
        0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1,
        10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1,
        10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1,
        1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1,
        2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1,
        7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1,
        7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1,
        2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1,
        1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1,
        11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1,
        8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1,
        0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1,
        7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1,
        10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1,
        2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1,
        6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1,
        7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1,
        2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1,
        1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1,
        10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1,
        10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1,
        0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1,
        7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1,
        6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1,
        8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1,
        9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1,
        6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1,
        1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1,
        4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1,
        10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1,
        8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1,
        0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1,
        1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1,
        8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1,
        10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1,
        4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1,
        10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1,
        5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1,
        11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1,
        9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1,
        6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1,
        7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1,
        3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1,
        7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1,
        9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1,
        3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1,
        6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1,
        9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1,
        1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1,
        4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1,
        7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1,
        6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1,
        3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1,
        0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1,
        6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1,
        1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1,
        0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1,
        11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1,
        6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1,
        5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1,
        9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1,
        1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1,
        1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1,
        10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1,
        0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1,
        5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1,
        10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1,
        11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1,
        0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1,
        9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1,
        7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1,
        2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1,
        8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1,
        9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1,
        9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1,
        1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1,
        9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1,
        9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1,
        5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1,
        0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1,
        10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1,
        2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1,
        0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1,
        0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1,
        9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1,
        5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1,
        3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1,
        5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1,
        8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1,
        0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1,
        9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1,
        0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1,
        1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1,
        3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1,
        4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1,
        9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1,
        11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1,
        11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1,
        2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1,
        9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1,
        3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1,
        1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1,
        4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1,
        4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1,
        0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1,
        3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1,
        3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1,
        0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1,
        9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1,
        1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

const int MarchingCubes::_edgeTable[256] = {
	0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
	0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
	0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
	0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
	0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
	0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
	0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
	0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
	0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
	0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
	0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
	0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
	0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
	0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
	0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
	0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
	0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
	0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
	0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
	0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
	0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
	0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
	0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
	0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
	0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
	0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
	0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
	0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
	0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
	0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
	0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
	0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0
};

// [Public methods]

MarchingCubes::MarchingCubes(RegularGrid& regularGrid, unsigned subdivisions, const uvec3& numDivs, unsigned maxTriangles)
{
    _gridSubdivisions = subdivisions;
    _numDivs = numDivs + uvec3(2);
    _maxTriangles = maxTriangles;
    _steps = glm::ceil(vec3(_numDivs) / vec3(_gridSubdivisions));
    _numThreads = _steps.x * _steps.y * _steps.z;
    _numGroups = ComputeShader::getNumGroups(_numThreads);
	//unsigned maxNumPoints = regularGrid.numOccupiedVoxels() * _maxTriangles * 3;
    unsigned maxNumPoints = regularGrid.calculateMaxQuadrantOccupancy(subdivisions) * _maxTriangles * 3;
	_indices = new unsigned[maxNumPoints];
	std::iota(_indices, _indices + maxNumPoints, 0);

	// Shaders
	_buildMarchingCubesShader = ShaderList::getInstance()->getComputeShader(RendEnum::BUILD_MARCHING_CUBES_FACES);
	_fuseSimilarVerticesShader_01 = ShaderList::getInstance()->getComputeShader(RendEnum::FUSE_VERTICES_01);
	_fuseSimilarVerticesShader_02 = ShaderList::getInstance()->getComputeShader(RendEnum::FUSE_VERTICES_02);
	_marchingCubesShader = ShaderList::getInstance()->getComputeShader(RendEnum::MARCHING_CUBES);
	_resetBufferShader = ShaderList::getInstance()->getComputeShader(RendEnum::RESET_BUFFER);

	_computeMortonShader = ShaderList::getInstance()->getComputeShader(RendEnum::COMPUTE_MORTON_CODES_FRACTURER);
	_bitMaskShader = ShaderList::getInstance()->getComputeShader(RendEnum::BIT_MASK_RADIX_SORT);
	_reduceShader = ShaderList::getInstance()->getComputeShader(RendEnum::REDUCE_PREFIX_SCAN);
	_downSweepShader = ShaderList::getInstance()->getComputeShader(RendEnum::DOWN_SWEEP_PREFIX_SCAN);
	_resetPositionShader = ShaderList::getInstance()->getComputeShader(RendEnum::RESET_LAST_POSITION_PREFIX_SCAN);
	_reallocatePositionShader = ShaderList::getInstance()->getComputeShader(RendEnum::REALLOCATE_RADIX_SORT);

    _finishLaplacianShader = ShaderList::getInstance()->getComputeShader(RendEnum::FINISH_LAPLACIAN_SMOOTHING);
    _laplacianShader = ShaderList::getInstance()->getComputeShader(RendEnum::LAPLACIAN_SMOOTHING);
    _markBoundaryTrianglesShader = ShaderList::getInstance()->getComputeShader(RendEnum::MARK_BOUNDARY_TRIANGLES);
    _resetLaplacianShader = ShaderList::getInstance()->getComputeShader(RendEnum::RESET_LAPLACIAN_SMOOTHING);

    // Buffers 
    _verticesSSBO = ComputeShader::setWriteBuffer(vec4(), maxNumPoints, GL_DYNAMIC_DRAW);
    _supportVerticesSSBO = ComputeShader::setWriteBuffer(vec4(), _numThreads * 12, GL_DYNAMIC_DRAW);
	_nonUpdatedVerticesSSBO = ComputeShader::setWriteBuffer(unsigned(), 1, GL_DYNAMIC_DRAW);
    _numVerticesSSBO = ComputeShader::setWriteBuffer(unsigned(), 1, GL_DYNAMIC_DRAW);

    _edgeTableSSBO = ComputeShader::setReadBuffer(_edgeTable, 256, GL_STATIC_DRAW);
    _triangleTableSSBO = ComputeShader::setReadBuffer(_triangleTable, 256 * 16, GL_STATIC_DRAW);

	_mortonCodeSSBO = ComputeShader::setWriteBuffer(unsigned(), maxNumPoints, GL_DYNAMIC_DRAW);
	_indicesBufferID_1 = ComputeShader::setWriteBuffer(GLuint(), maxNumPoints, GL_DYNAMIC_DRAW);
	_indicesBufferID_2 = ComputeShader::setWriteBuffer(GLuint(), maxNumPoints, GL_DYNAMIC_DRAW);					// Substitutes indicesBufferID_1 for the next iteration
	_indices4ID = ComputeShader::setWriteBuffer(uvec4(), maxNumPoints, GL_DYNAMIC_DRAW);
	_pBitsBufferID = ComputeShader::setWriteBuffer(GLuint(), maxNumPoints, GL_DYNAMIC_DRAW);
	_nBitsBufferID = ComputeShader::setWriteBuffer(GLuint(), maxNumPoints, GL_DYNAMIC_DRAW);
	_positionBufferID = ComputeShader::setWriteBuffer(GLuint(), maxNumPoints, GL_DYNAMIC_DRAW);

	_vertexSSBO = ComputeShader::setWriteBuffer(vec4(), maxNumPoints, GL_DYNAMIC_DRAW);
	_faceSSBO = ComputeShader::setWriteBuffer(uvec4(), maxNumPoints / 3, GL_DYNAMIC_DRAW);

    _laplacianSSBO = ComputeShader::setWriteBuffer(ivec4(), maxNumPoints, GL_DYNAMIC_DRAW);
    _gridSSBO = ComputeShader::setWriteBuffer(float(), _numDivs.x * _numDivs.y * _numDivs.z, GL_STATIC_DRAW);
}

MarchingCubes::~MarchingCubes()
{
    GLuint buffers[] = { 
		_verticesSSBO, _edgeTableSSBO, _triangleTableSSBO, _supportVerticesSSBO, _nonUpdatedVerticesSSBO, _numVerticesSSBO, _mortonCodeSSBO,
		_indicesBufferID_1, _indicesBufferID_2, _pBitsBufferID, _nBitsBufferID, _positionBufferID, _vertexSSBO, _faceSSBO, _laplacianSSBO
	};
    glDeleteBuffers(sizeof(buffers) / sizeof(GLuint), buffers);

	delete[] _indices;
}

CADModel* MarchingCubes::triangulateFieldGPU(GLuint gridSSBO, float targetValue, FractureParameters& fractureParams, const mat4& modelMatrix)
{
	//std::cout << "Solving fragment " << static_cast<unsigned>(targetValue) << std::endl;

    CADModel* model = new CADModel();
    unsigned numSteps = _gridSubdivisions * _gridSubdivisions * _gridSubdivisions, stepIdx = 0;

    std::vector<vec4> triangleVertices;
    std::vector<uvec4> triangleIndices;

    for (int x = 0; x < _numDivs.x; x += _steps.x)
    {
        for (int y = 0; y < _numDivs.y; y += _steps.y)
        {
            for (int z = 0; z < _numDivs.z; z += _steps.z)
            {
                uvec3 start = uvec3(x, y, z), size = glm::min(start + _steps, _numDivs) - start;
                ++stepIdx;

                this->resetCounter(_numVerticesSSBO);

                _marchingCubesShader->bindBuffers(std::vector<GLuint>{ _gridSSBO, _verticesSSBO, _numVerticesSSBO, _triangleTableSSBO, _edgeTableSSBO, _supportVerticesSSBO });
                _marchingCubesShader->use();
                _marchingCubesShader->setUniform("gridDims", _numDivs);
                _marchingCubesShader->setUniform("isolevel", 0.5f);
                _marchingCubesShader->setUniform("localSize", size);
                _marchingCubesShader->setUniform("start", start);
                _marchingCubesShader->setUniform("targetValue", int(targetValue));
                _marchingCubesShader->execute(_numGroups, 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

                unsigned numVertices = *ComputeShader::readData(_numVerticesSSBO, unsigned());
                //vec4* vertexData = ComputeShader::readData(_verticesSSBO, vec4());

                if (numVertices)
                {
                    this->calculateMortonCodes(numVertices);
                    this->sortMortonCodes(numVertices);
                    unsigned newNumVertices = this->fuseSimilarVertices(numVertices, modelMatrix);
                    this->buildMarchingCubesFaces(numVertices);
                    this->markBoundaryTriangles(numVertices / 3);
                    this->smoothSurface(newNumVertices, numVertices / 3, 10, 2);

                    vec4* vertices = ComputeShader::readData(_vertexSSBO, vec4(), 0, sizeof(vec4) * newNumVertices);
                    uvec4* faces = ComputeShader::readData(_faceSSBO, uvec4(), 0, sizeof(uvec4) * numVertices / 3);

                    model->insert(vertices, newNumVertices, faces, numVertices / 3);
                }
            }
        }
    }

    //if (_gridSubdivisions > 1)
    //{
    //    ComputeShader::updateReadBuffer(_verticesSSBO, triangleVertices.data(), triangleVertices.size(), GL_DYNAMIC_DRAW);
    //    ComputeShader::updateReadBuffer(_faceSSBO, triangleIndices.data(), triangleIndices.size(), GL_DYNAMIC_DRAW);

    //    unsigned numVertices = triangleVertices.size();

    //    this->calculateMortonCodes(numVertices);
    //    this->sortMortonCodes(numVertices);
    //    unsigned newNumVertices = this->fuseSimilarVertices(numVertices, modelMatrix);
    //    this->buildMarchingCubesFaces(numVertices);

    //    vec4* vertices = ComputeShader::readData(_vertexSSBO, vec4(), 0, sizeof(vec4) * newNumVertices);
    //    uvec4* faces = ComputeShader::readData(_faceSSBO, uvec4(), 0, sizeof(uvec4) * triangleVertices.size() / 3);

    //    model->insert(vertices, newNumVertices, faces, numVertices / 3);
    //}

    model->endBatch(false, fractureParams._renderMesh, fractureParams._targetTriangles);

    return model;
}

CADModel* MarchingCubes::triangulateFieldExtendedCPU(float targetValue, const mat4& modelMatrix)
{

}

// [Protected methods]

void MarchingCubes::buildMarchingCubesFaces(unsigned numVertices)
{
	this->resetCounter(_numVerticesSSBO);

	_buildMarchingCubesShader->bindBuffers(std::vector<GLuint> { _indicesBufferID_2, _indicesBufferID_1, _faceSSBO, _numVerticesSSBO });
	_buildMarchingCubesShader->use();
	_buildMarchingCubesShader->setUniform("numPoints", numVertices);
	_buildMarchingCubesShader->execute(ComputeShader::getNumGroups(numVertices), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	//Model3D::FaceGPUData* faceData = ComputeShader::readData(_faceSSBO, Model3D::FaceGPUData());
	//std::vector<Model3D::FaceGPUData> faces = std::vector<Model3D::FaceGPUData>(faceData, faceData + numVertices / 3);

	//std::unordered_set<unsigned> indices;
	//for (const Model3D::FaceGPUData& face : faces)
	//{
	//	indices.insert(face._vertices.x);
	//	indices.insert(face._vertices.y);
	//	indices.insert(face._vertices.z);
	//}
};

void MarchingCubes::calculateMortonCodes(unsigned numVertices)
{
	_computeMortonShader->bindBuffers(std::vector<GLuint> { _verticesSSBO, _mortonCodeSSBO });
	_computeMortonShader->use();
	_computeMortonShader->setUniform("numPoints", numVertices);
	_computeMortonShader->setUniform("sceneMaxBoundary", vec3(_numDivs));
	_computeMortonShader->setUniform("sceneMinBoundary", vec3(.0f));
	_computeMortonShader->execute(ComputeShader::getNumGroups(numVertices), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);
}

unsigned MarchingCubes::fuseSimilarVertices(unsigned numVertices, const mat4& modelMatrix)
{
	unsigned defaultValue = UINT_MAX;

	this->resetCounter(_numVerticesSSBO);

	//unsigned* data = ComputeShader::readData(_indicesBufferID_1, unsigned());
	//std::vector<unsigned> buffer = std::vector<unsigned>(data, data + numVertices);

	_fuseSimilarVerticesShader_01->bindBuffers(std::vector<GLuint> { _indicesBufferID_2, _indicesBufferID_1, _verticesSSBO, _vertexSSBO, _numVerticesSSBO });
	_fuseSimilarVerticesShader_01->use();
	_fuseSimilarVerticesShader_01->setUniform("defaultValue", defaultValue);
	_fuseSimilarVerticesShader_01->setUniform("modelMatrix", modelMatrix);
	_fuseSimilarVerticesShader_01->setUniform("numPoints", numVertices);
	_fuseSimilarVerticesShader_01->execute(ComputeShader::getNumGroups(numVertices), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

	//unsigned nv = *ComputeShader::readData(_numVerticesSSBO, unsigned());
	//data = ComputeShader::readData(_indicesBufferID_1, unsigned());
	//buffer = std::vector<unsigned>(data, data + numVertices);

	unsigned nonUpdatedVertices = 1;

	while (nonUpdatedVertices > 0)
	{
		this->resetCounter(_nonUpdatedVerticesSSBO);

		_fuseSimilarVerticesShader_02->bindBuffers(std::vector<GLuint> { _indicesBufferID_2, _indicesBufferID_1, _verticesSSBO, _nonUpdatedVerticesSSBO });
		_fuseSimilarVerticesShader_02->use();
		_fuseSimilarVerticesShader_02->setUniform("defaultValue", defaultValue);
		_fuseSimilarVerticesShader_02->setUniform("numPoints", numVertices);
		_fuseSimilarVerticesShader_02->execute(ComputeShader::getNumGroups(numVertices), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

		nonUpdatedVertices = *ComputeShader::readData(_nonUpdatedVerticesSSBO, unsigned());
	}

	//data = ComputeShader::readData(_indicesBufferID_1, unsigned());
	//buffer = std::vector<unsigned>(data, data + numVertices);

	return *ComputeShader::readData(_numVerticesSSBO, unsigned());
}

void MarchingCubes::resetCounter(GLuint ssbo)
{
	unsigned zero = 0;
	ComputeShader::updateReadBuffer(ssbo, &zero, 1, GL_DYNAMIC_DRAW);
}

void MarchingCubes::markBoundaryTriangles(unsigned numFaces)
{
    _markBoundaryTrianglesShader->bindBuffers(std::vector<GLuint> { _vertexSSBO, _faceSSBO });
    _markBoundaryTrianglesShader->use();
    _markBoundaryTrianglesShader->setUniform("numFaces", numFaces);
    _markBoundaryTrianglesShader->execute(ComputeShader::getNumGroups(numFaces), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

 //   uvec4* faces = ComputeShader::readData(_faceSSBO, uvec4());
 //   std::vector<uvec4> buffer = std::vector<uvec4>(faces, faces + numFaces);

 //   unsigned count = 0;
 //   for (const uvec4& face : buffer)
 //   {
	//	if (face.w == 1)
	//		++count;
	//}

 //   std::cout << count << std::endl;
}

void MarchingCubes::smoothSurface(unsigned numVertices, unsigned numFaces, unsigned numIterations, unsigned boundaryIterations)
{
    for (int i = 0; i < numIterations; ++i)
    {
        _resetLaplacianShader->bindBuffers(std::vector<GLuint> { _laplacianSSBO });
        _resetLaplacianShader->use();
        _resetLaplacianShader->setUniform("numVertices", numVertices);
        _resetLaplacianShader->execute(ComputeShader::getNumGroups(numVertices), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

        _laplacianShader->bindBuffers(std::vector<GLuint> { _vertexSSBO, _faceSSBO, _laplacianSSBO });
        _laplacianShader->use();
        _laplacianShader->setUniform("numFaces", numFaces);
        _laplacianShader->execute(ComputeShader::getNumGroups(numFaces), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

        //vec4* vertices = ComputeShader::readData(_vertexSSBO, vec4(), 0, numVertices);
        //std::vector<vec4> buffer2 = std::vector<vec4>(vertices, vertices + numVertices);

        _finishLaplacianShader->bindBuffers(std::vector<GLuint> { _vertexSSBO, _laplacianSSBO });
        _finishLaplacianShader->use();
        _finishLaplacianShader->setUniform("numVertices", numVertices);
        _finishLaplacianShader->setUniform("weight", i >= boundaryIterations ? 0.08f : .0f);
        _finishLaplacianShader->execute(ComputeShader::getNumGroups(numVertices), 1, 1, ComputeShader::getMaxGroupSize(), 1, 1);

        //ivec4* data = ComputeShader::readData(_laplacianSSBO, ivec4());
        //std::vector<ivec4> buffer = std::vector<ivec4>(data, data + numVertices);

        //vertices = ComputeShader::readData(_vertexSSBO, vec4(), 0, numVertices);
        //buffer2 = std::vector<vec4>(vertices, vertices + numVertices);

        //std::cout << std::endl;
    }
}

void MarchingCubes::setGrid(RegularGrid& regularGrid)
{
    float* gridData = (float*)malloc(sizeof(float) * _numDivs.x * _numDivs.y * _numDivs.z);
    for (int x = 0; x < _numDivs.x; ++x)
        for (int y = 0; y < _numDivs.y; ++y)
            for (int z = 0; z < _numDivs.z; ++z)
                gridData[x * _numDivs.y * _numDivs.z + y * _numDivs.z + z] = VOXEL_FREE;

    for (int x = 1; x < _numDivs.x - 1; ++x)
        for (int y = 1; y < _numDivs.y - 1; ++y)
            for (int z = 1; z < _numDivs.z - 1; ++z)
                gridData[x * _numDivs.y * _numDivs.z + y * _numDivs.z + z] = regularGrid.at(x - 1, y - 1, z - 1);

    ComputeShader::updateReadBuffer(_gridSSBO, gridData, _numDivs.x * _numDivs.y * _numDivs.z, GL_STATIC_DRAW);
    free(gridData);
}

void MarchingCubes::sortMortonCodes(unsigned numVertices)
{
	const unsigned numBits	= 30;	// 10 bits per coordinate (3D)
	unsigned arraySize		= numVertices, currentBits = 0;
	const int numGroups		= ComputeShader::getNumGroups(arraySize);
	const int maxGroupSize	= ComputeShader::getMaxGroupSize();

	// Binary tree parameters
	const unsigned startThreads		= std::ceil(arraySize / 2.0f);
	const int numExec				= std::ceil(std::log2(arraySize));
	const int numGroups2Log			= ComputeShader::getNumGroups(startThreads);
	unsigned numThreads				= 0, iteration;

	ComputeShader::updateReadBuffer(_indicesBufferID_2, _indices, arraySize);

	while (currentBits < numBits)
	{
		std::vector<GLuint> threadCount{ startThreads };
		threadCount.reserve(numExec);

		std::swap(_indicesBufferID_1, _indicesBufferID_2);							// indicesBufferID_2 is initialized with indices cause it's swapped here

		// FIRST STEP: BIT MASK, check if a morton code gives zero or one for a certain mask (iteration)
		unsigned bitMask = 1 << currentBits++;

		_bitMaskShader->bindBuffers(std::vector<GLuint> { _mortonCodeSSBO, _indicesBufferID_1, _pBitsBufferID, _nBitsBufferID });
		_bitMaskShader->use();
		_bitMaskShader->setUniform("arraySize", arraySize);
		_bitMaskShader->setUniform("bitMask", bitMask);
		_bitMaskShader->execute(numGroups, 1, 1, maxGroupSize, 1, 1);

		// SECOND STEP: build a binary tree with a summatory of the array
		_reduceShader->bindBuffers(std::vector<GLuint> { _nBitsBufferID });
		_reduceShader->use();
		_reduceShader->setUniform("arraySize", arraySize);

		iteration = 0;
		while (iteration < numExec)
		{
			numThreads = threadCount[threadCount.size() - 1];

			_reduceShader->setUniform("iteration", iteration++);
			_reduceShader->setUniform("numThreads", numThreads);
			_reduceShader->execute(numGroups2Log, 1, 1, maxGroupSize, 1, 1);

			threadCount.push_back(std::ceil(numThreads / 2.0f));
		}

		// THIRD STEP: set last position to zero, its faster to do it in GPU than retrieve the array in CPU, modify and write it again to GPU
		_resetPositionShader->bindBuffers(std::vector<GLuint> { _nBitsBufferID });
		_resetPositionShader->use();
		_resetPositionShader->setUniform("arraySize", arraySize);
		_resetPositionShader->execute(1, 1, 1, 1, 1, 1);

		// FOURTH STEP: build tree back to first level and compute position of each bit
		_downSweepShader->bindBuffers(std::vector<GLuint> { _nBitsBufferID });
		_downSweepShader->use();
		_downSweepShader->setUniform("arraySize", arraySize);

		iteration = threadCount.size() - 2;
		while (iteration >= 0 && iteration < numExec)
		{
			_downSweepShader->setUniform("iteration", iteration);
			_downSweepShader->setUniform("numThreads", threadCount[iteration--]);
			_downSweepShader->execute(numGroups2Log, 1, 1, maxGroupSize, 1, 1);
		}

		_reallocatePositionShader->bindBuffers(std::vector<GLuint> { _pBitsBufferID, _nBitsBufferID, _indicesBufferID_1, _indicesBufferID_2 });
		_reallocatePositionShader->use();
		_reallocatePositionShader->setUniform("arraySize", arraySize);
		_reallocatePositionShader->execute(numGroups, 1, 1, maxGroupSize, 1, 1);
	}

	//GLuint* data = ComputeShader::readData(_indicesBufferID_2, GLuint());
	//std::vector<GLuint> dataBuffer = std::vector<GLuint>(data, data + arraySize);
}