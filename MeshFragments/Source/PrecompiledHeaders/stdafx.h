#define _USE_MATH_DEFINES

// [Libraries]

#include "GL/glew.h"								// Don't swap order between GL and GLFW includes!
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/epsilon.hpp"
#include "glm/gtx/norm.hpp"

// [Image]

#include "lodepng.h"

// [Standard libraries: basic]

#include <algorithm>
#include <cmath>
#include <chrono>
#include <climits>
#include <cstdint>
#include <exception>
#include <execution>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <thread>

// [Standard libraries: data structures]

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// [Noise]

#include "FastNoise/FastNoise.h"
#include "FastNoise/FastNoiseMetadata.h"

// [Our own classes]

#include "Geometry/General/Adapter.h"
