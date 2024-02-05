#pragma once

#include "DataStructures/GStack.h"
#include "DataStructures/RegularGrid.h"

template<typename T>
class QuadStack
{
private:
	uint16_t                            _width, _height, _depth;
	std::vector<std::vector<GStack<T>>> _gStacks;
	GStack<T>*							_quadStack;

private:
	enum StorageUnit : long
	{
		BYTE = 1, KILOBYTE = 1024, MEGABYTE = KILOBYTE * 1024, GIGABYTE = MEGABYTE * 1024
	};

	std::string storageUnitToStr(StorageUnit unit) { static std::vector<std::string> unitTitle{ "Bytes", "KBs", "MBs", "GBs" }; return unitTitle[std::log2(static_cast<float>(unit)) / 10]; }

private:
	void backgroundWriteImage(T* layer, const std::string& filename, const uint16_t width, const uint16_t height);
	void buildQuadStack();
	void compressQuadStack(GStack<T>* root, uint16_t depth);
	void compressQuadStack(GStack<T>*& root, uint16_t depth, bool foo);
	GStack<T>* createGStack(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, bool leafNode = false, GStack<T>* node = nullptr);
	uint32_t flattenIndex(uint16_t x, uint16_t y);
	uint32_t flattenIndex(uint16_t x, uint16_t y, uint16_t z);
	std::vector<std::vector<GStack<T>>> matchGS(GStack<T>* root, std::vector<std::vector<uint8_t>>& iteratorCount, std::vector<std::vector<uint8_t>>& numLayers, float& scBest);
	void recursiveSplitQuadtree(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, GStack<T>* root);

	void getLeaves(GStack<T>* root, std::vector<GStack<T>*>& leaves)
	{
		if (!root)
			return;

		if (root->getNumIntervals())
			leaves.push_back(root);

		for (int x = 0; x < 2; ++x)
			for (int y = 0; y < 2; ++y)
				this->getLeaves(root->_children[x][y], leaves);
	}

public:
	QuadStack();
	virtual ~QuadStack();
	QuadStack(const QuadStack& cube) = delete;
	QuadStack& operator=(const QuadStack& cube) = delete;

	void calculateCompression(StorageUnit unit = GIGABYTE);
	void compress_x();
	void compress_y();
	bool loadCube(RegularGrid* voxelization);
	bool openCheckpoint(const std::string& filename);
	void saveCheckpoint(const std::string& filename);
	bool writeBand(const std::string& filename, uint16_t layer = 0);
};

template<typename T>
QuadStack<T>::QuadStack() : _width(0), _height(0), _depth(0), _quadStack(nullptr)
{
}

template<typename T>
QuadStack<T>::~QuadStack()
{
	delete _quadStack;
}

template<typename T>
void QuadStack<T>::calculateCompression(StorageUnit unit)
{
	size_t defaultOccupancy = sizeof(uint16_t) * _width * _depth * _height;
	size_t compressedOccupancy = 0;

	for (int x = 0; x < _width; ++x)
		#pragma omp parallel for reduction(+:compressedOccupancy)
		for (int y = 0; y < _height; ++y)
			compressedOccupancy += _gStacks[x][y].size();

	size_t quadStackOccupancy = _quadStack->size();

	std::cout << "Default size: " << defaultOccupancy / static_cast<float>(unit) << " " << storageUnitToStr(unit) << std::endl;
	std::cout << "Compressed size: " << compressedOccupancy / static_cast<float>(unit) << " " << storageUnitToStr(unit) << std::endl;
	std::cout << "Compression rate with initial phase: " << (static_cast<float>(defaultOccupancy - compressedOccupancy)) / defaultOccupancy << "%" << std::endl;
	std::cout << "Compression rate with second phase: " << (static_cast<float>(defaultOccupancy - quadStackOccupancy)) / defaultOccupancy << "%" << std::endl;
}

template<typename T>
void QuadStack<T>::compress_x()
{
	this->buildQuadStack();
	this->compressQuadStack(_quadStack, 0, true);
}

template<typename T>
void QuadStack<T>::compress_y()
{
	for (int x = 0; x < _width; ++x)
	{
		#pragma omp parallel for
		for (int y = 0; y < _height; ++y)
		{
			_gStacks[x][y] = _gStacks[x][y].compress(x, y);
		}
	}
}

template<typename T>
bool QuadStack<T>::loadCube(RegularGrid* voxelization)
{
	if (voxelization)
	{
		uvec3 voxelizationSize = voxelization->getNumSubdivisions();

		_width = voxelizationSize.x;
		_height = voxelizationSize.y;
		_depth = voxelizationSize.z;
		if (!_width or !_height or !_depth) return false;

		//printf("Size is %dx%dx%d\n", _width, _height, _depth);

		static std::vector<std::vector<std::vector<T>>> materialMatrix;
		voxelization->getData<T>(materialMatrix);

		_gStacks.resize(_width);
		for (int w = 0; w < _width; ++w)
		{
			_gStacks[w].resize(_height);
			#pragma omp parallel for
			for (int h = 0; h < _height; ++h)
			{
				_gStacks[w][h].loadMaterials(materialMatrix[w][h]);
			}
		}

		return true;
	}

	return false;
}

template<typename T>
inline bool QuadStack<T>::openCheckpoint(const std::string& filename)
{
	//std::ifstream fin(filename, std::ios::in | std::ios::binary);
	//if (!fin.is_open())
	//{
	//    return false;
	//}

	//size_t newSize;
	//const size_t tSize = sizeof(T);
	//fin.read((char*)&newSize, sizeof(size_t));
	//if (tSize != newSize) return false;

	//fin.read((char*)&_width, sizeof(uint16_t));
	//fin.read((char*)&_height, sizeof(uint16_t));
	//fin.read((char*)&_depth, sizeof(uint16_t));

	//const size_t numCells = _width * _height * _depth;

	//if (_cube) delete[] _cube;
	//_cube = new T[numCells];
	//fin.read((char*)&_cube[0], numCells * tSize);

	//if (_indices) delete[] _indices;
	//_indices = new size_t[numCells];
	//fin.read((char*)&_indices[0], numCells * sizeof(size_t));

	//if (_colorLayer) delete[] _colorLayer;
	//_colorLayer = new ColorLayers<T>[_width * _height];

	//fin.close();

	return true;
}

template<typename T>
inline void QuadStack<T>::saveCheckpoint(const std::string& filename)
{
	std::ofstream fout(filename, std::ios::out | std::ios::binary);
	if (!fout.is_open())
	    return;

	const size_t numCells = _width * _height * _depth;
	const size_t tSize = sizeof(T);
	std::vector<GStack<T>*> leaves;
	this->getLeaves(_quadStack, leaves);
	size_t numNodes = leaves.size();

	fout.write((char*)&tSize, sizeof(size_t));
	fout.write((char*)&_width, sizeof(uint16_t));
	fout.write((char*)&_height, sizeof(uint16_t));
	fout.write((char*)&_depth, sizeof(uint16_t));
	fout.write((char*)&numNodes, sizeof(size_t));

	for (GStack<T>* leaf : leaves)
	{
		size_t numIntervals = leaf->getNumIntervals();
		fout.write((char*)&numIntervals, sizeof(size_t));
		fout.write((char*)&leaf->_maxPoints, sizeof(glm::uvec2));
		fout.write((char*)&leaf->_minPoints, sizeof(glm::uvec2));

		for (size_t interval = 0; interval < numIntervals; ++interval)
		{
			uint8_t lengthWidth = leaf->_intervals[interval]._length.size();
			uint8_t lengthHeight = leaf->_intervals[interval]._length[0].size();

			fout.write((char*)&lengthWidth, sizeof(uint8_t));
			fout.write((char*)&lengthHeight, sizeof(uint8_t));

			fout.write((char*)&leaf->_intervals[interval]._value, sizeof(T));
			for (int x = 0; x < leaf->_intervals[interval]._length.size(); ++x)
				for (int y = 0; y < leaf->_intervals[interval]._length[x].size(); ++y)
					fout.write((char*)&leaf->_intervals[interval]._length[x][y], sizeof(uint16_t));
		}
	}

	fout.close();
}

template<typename T>
bool QuadStack<T>::writeBand(const std::string& filename, uint16_t layer)
{
	//if (layer >= _depth) return false;

	//QuadStack::backgroundWriteImage(&_cube[this->flattenIndex(0, 0, layer)], filename, _width, _height);
	//writeImageThread.detach();

	return true;
}

template<typename T>
void QuadStack<T>::backgroundWriteImage(T* layer, const std::string& filename, const uint16_t width, const uint16_t height)
{
	std::vector<unsigned char> input(width * height), output;
	for (int idx = 0; idx < width * height; ++idx)
		input[idx] = static_cast<unsigned char>(float(layer[idx]) * std::numeric_limits<uint8_t>::max() / std::numeric_limits<T>::max());

	unsigned error = lodepng::encode(output, input, width, height, LodePNGColorType::LCT_GREY);
	if (!error)
	{
		lodepng::save_file(output, filename);
	}
}

template<typename T>
inline void QuadStack<T>::buildQuadStack()
{
	_quadStack = this->createGStack(0, _width, 0, _height);

	this->recursiveSplitQuadtree(0, _width, 0, _height, _quadStack);
}

template<typename T>
inline void QuadStack<T>::compressQuadStack(GStack<T>* root, uint16_t depth)
{
	if (!root->getNumChildren())
		return;
	else
	{
		std::vector<GStack<T>*> children;
		std::vector<GStack<T>*> newGStacks;
		std::vector<GStack<T>*> temporalStacks;
		uint32_t numIntervals = 0;

		for (int x = 0; x < 2; ++x)
		{
			for (int y = 0; y < 2; ++y)
			{
				if (root->_children[x][y])
				{
					this->compressQuadStack(root->_children[x][y], depth + 1);
					numIntervals += root->_children[x][y]->getNumIntervals();

					children.push_back(root->_children[x][y]);
					GStack<T>* gStack = new GStack<T>;
					GStack<T>* tStack = new GStack<T>;
					gStack->updateBoundaries(root->_children[x][y]);

					newGStacks.push_back(gStack);
					temporalStacks.push_back(tStack);
				}
			}
		}

		if (numIntervals)
		{
			uint16_t bestChoice = 0;
			std::vector<uint16_t> iteratorCount(children.size(), 0);

			for (int layer = 0; layer < children[0]->getNumIntervals(); ++layer)
			{
				iteratorCount[0] = layer;
				this->matchGS(children, newGStacks, temporalStacks, iteratorCount, 0);
			}
		}
	}
}

template<typename T>
inline void QuadStack<T>::compressQuadStack(GStack<T>*& root, uint16_t depth, bool foo)
{
	if (!root or !root->getNumChildren())
		return;
	else
	{
		for (int x = 0; x < 2; ++x)
			for (int y = 0; y < 2; ++y)
				this->compressQuadStack(root->_children[x][y], depth + 1, foo);

		std::vector<GStack<T>*> children;
		for (int x = 0; x < 2; ++x)
			for (int y = 0; y < 2; ++y)
				if (root->_children[x][y])
					children.push_back(root->_children[x][y]);

		GStack<T>::mergeStacks(root, children);
	}
}

template<typename T>
inline GStack<T>* QuadStack<T>::createGStack(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, bool leafNode, GStack<T>* node)
{
	GStack<T>* gStack = (node != nullptr) ? node : new GStack<T>;
	gStack->updateBoundaries(x_start, y_start, x_end, y_end);

	if (leafNode)
	{
		size_t numIntervals = _gStacks[x_start][y_start].getNumLayers();
		std::vector<std::vector<uint16_t>> matrix(x_end - x_start, std::vector<uint16_t>(y_end - y_start, 0));

		for (size_t layer = 0; layer < numIntervals; ++layer)
		{
			for (uint16_t x = x_start; x < x_end; ++x)
			{
				for (uint16_t y = y_start; y < y_end; ++y)
				{
					matrix[x - x_start][y - y_start] += _gStacks[x][y].getLength(layer);
				}
			}

			gStack->createInterval(_gStacks[x_start][y_start].getValue(layer), matrix);
		}
	}

	return gStack;
}

template<typename T>
inline uint32_t QuadStack<T>::flattenIndex(uint16_t x, uint16_t y)
{
	return y * _width + x;
}

template<typename T>
uint32_t QuadStack<T>::flattenIndex(uint16_t x, uint16_t y, uint16_t z)
{
	return z * _width * _height + y * _width + x;
}

template<typename T>
inline std::vector<std::vector<GStack<T>>> QuadStack<T>::matchGS(GStack<T>* root, std::vector<std::vector<uint8_t>>& iteratorCount, std::vector<std::vector<uint8_t>>& numLayers, float& scBest)
{
	std::vector<std::vector<GStack<T>>> g = std::vector<std::vector<GStack<T>>>(2, std::vector<GStack<T>>(2));

	for (std::vector<GStack<T>>& stackArray : g)
	{
		for (GStack<T>& gStack : stackArray)
		{
			gStack.addWildcardInterval();
		}
	}

	return g;
}

template<typename T>
inline void QuadStack<T>::recursiveSplitQuadtree(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, GStack<T>* root)
{
	if ((x_end - x_start) <= 1 and (y_end - y_start) <= 1)
	{
		this->createGStack(x_start, x_end, y_start, y_end, true, root);
		return;
	}

	bool areIdentical = true;
	GStack<T>* firstStack = &_gStacks[x_start][y_start];

	for (uint16_t x = x_start; x < x_end and areIdentical; ++x)
	{
		for (uint16_t y = y_start; y < y_end and areIdentical; ++y)
		{
			areIdentical &= firstStack->operator==(_gStacks[x][y]);
		}
	}

	if (areIdentical)
	{
		this->createGStack(x_start, x_end, y_start, y_end, true, root);
		return;
	}
	else
	{
		size_t size_x = x_end - x_start, endIndex_x = (size_x + 1) / 2;
		size_t size_y = y_end - y_start, endIndex_y = (size_y + 1) / 2;

		{
			root->_children[0][0] = this->createGStack(x_start, x_start + endIndex_x, y_start, y_start + endIndex_y);
			this->recursiveSplitQuadtree(x_start, x_start + endIndex_x, y_start, y_start + endIndex_y, root->_children[0][0]);
		}

		if (size_y > 1)
		{
			root->_children[0][1] = this->createGStack(x_start, x_start + endIndex_x, y_start + endIndex_y, y_end);
			this->recursiveSplitQuadtree(x_start, x_start + endIndex_x, y_start + endIndex_y, y_end, root->_children[0][1]);
		}

		if (size_x > 1)
		{
			root->_children[1][0] = this->createGStack(x_start + endIndex_x, x_end, y_start, y_start + endIndex_y);
			this->recursiveSplitQuadtree(x_start + endIndex_x, x_end, y_start, y_start + endIndex_y, root->_children[1][0]);
		}

		if (size_x > 1 and size_y > 1)
		{
			root->_children[1][1] = this->createGStack(x_start + endIndex_x, x_end, y_start + endIndex_y, y_end);
			this->recursiveSplitQuadtree(x_start + endIndex_x, x_end, y_start + endIndex_y, y_end, root->_children[1][1]);
		}
	}
}