// [Libraries]

#include "GL/glew.h"								// Don't swap order between GL and GLFW includes!
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/epsilon.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/norm.hpp"

// [Image]

#include "lodepng.h"

// [Standard libraries: basic]

#include <algorithm>
#include <cmath>
#include <cassert>
#include <chrono>
#include <climits>
#include <cstdint>
#include <exception>
#include <execution>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <regex>
#include <sstream>
#include <stdexcept>
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

// [Our own classes]

#include "Geometry/General/Adapter.h"

// [CGAL]

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_3.h>

typedef CGAL::Exact_predicates_inexact_constructions_kernel     K;
typedef CGAL::Delaunay_triangulation_3<K, CGAL::Fast_location>  Delaunay;
typedef Delaunay::Point                                         Point;
