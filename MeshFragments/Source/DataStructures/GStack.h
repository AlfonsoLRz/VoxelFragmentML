#pragma once

template<typename T>
class GStack
{
protected:
	template<typename T>
	struct Interval
	{
		T									_value;
		std::vector<std::vector<uint16_t>>	_length;
	};

public:
	GStack<T>*					_children[2][2];
	std::vector<Interval<T>>	_intervals;
	uvec2						_maxPoints, _minPoints;

protected:
	static bool mergeStacks(GStack<T>* root, std::vector<GStack<T>*>& stacks, std::vector<uint8_t>& iteratorCount, uint8_t _depth);

public:
	GStack();
	GStack(const std::vector<uint16_t>& materials);
	virtual ~GStack();

	void addColor(T color);
	bool areColorEqual(const GStack<T>& layers);
	void addInterval(const Interval<T>& interval);
	void addWildcardInterval(uint8_t index);
	void clearIntervals() { _intervals.clear(); }
	GStack<T> compress(uint32_t x, uint32_t y);
	std::vector<std::vector<uint16_t>> createHeightfield(uint16_t length);
	void createInterval(T value, const std::vector<std::vector<uint16_t>>& heightfield);
	bool isWildcard(uint8_t interval);
	static void mergeInterval(Interval<T>& interval, GStack<T>* root, std::vector<GStack<T>*>& stacks, std::vector<uint8_t>& layers);
	static void mergeStacks(GStack<T>* root, std::vector<GStack<T>*>& stacks);
	bool operator==(const GStack<T>& stack);
	void removeInterval(uint8_t interval);
	void updateBoundaries(uint32_t x, uint32_t y);
	void updateBoundaries(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1);
	void updateBoundaries(GStack<T>* gStack);
	void swapIntervalWildcard(uint8_t interval);

	uint16_t getLength(uint16_t interval) { return _intervals[interval]._length[0][0]; }
	uint8_t getNumChildren() { uint8_t count = 0; for (int x = 0; x < 2; ++x) for (int y = 0; y < 2; ++y) count += static_cast<uint8_t>(_children[x][y] != nullptr); return count; }
	uint8_t getNumIntervals() { return _intervals.size(); }
	uint8_t getNumTerminalIntervals(uint8_t startingIndex);
	T getValue(uint16_t interval) { return _intervals[interval]._value; }
	size_t getNumLayers() const { return _intervals.size(); }
	size_t size() const;
};

template<typename T>
GStack<T>::GStack(): _maxPoints(0), _minPoints(UINT_MAX)
{
	for (int x = 0; x < 2; ++x)
		for (int y = 0; y < 2; ++y)
			_children[x][y] = nullptr;
}

template<typename T>
GStack<T>::GStack(const std::vector<uint16_t>& materials): GStack<T>()
{
	for (uint16_t material : materials)
	{
		Interval<T> interval;
		interval._value = static_cast<T>(material);
		interval._length = createHeightfield(1);
		_intervals.push_back(interval);
	}
}

template<typename T>
inline GStack<T>::~GStack()
{
	for (int x = 0; x < 2; ++x)
		for (int y = 0; y < 2; ++y)
		{
			delete _children[x][y];
			_children[x][y] = nullptr;
		}
}

template<typename T>
inline void GStack<T>::addColor(T color)
{
	size_t lastIndex = _intervals.size() - 1;

	if (_intervals.empty() or _intervals[lastIndex]._value != color)
	{
		_intervals.push_back(Interval<T>{ color, createHeightfield(1) });
	}
	else
	{
		++_intervals[lastIndex]._length[0][0];
	}
}

template<typename T>
inline bool GStack<T>::areColorEqual(const GStack<T>& stack)
{
	bool equal = true;
	for (int colorIdx = 0; colorIdx < stack.getNumLayers() and equal; ++colorIdx)
		equal &= stack._intervals[colorIdx]._value == _intervals[colorIdx]._value;

	return equal;
}

template<typename T>
inline void GStack<T>::addInterval(const Interval<T>& interval)
{
	this->_intervals.push_back(std::move(interval));
}

template<typename T>
inline void GStack<T>::addWildcardInterval(uint8_t index)
{
	_intervals.push_back(_intervals.begin() + index, Interval{ std::numeric_limits<T>::max() });
}

template<typename T>
inline GStack<T> GStack<T>::compress(uint32_t x, uint32_t y)
{
	GStack<T> newStack;
	for (Interval<T>& interval : _intervals)
	{
		for (int i = 0; i < interval._length[0][0]; ++i)
			newStack.addColor(interval._value);
	}

	newStack.updateBoundaries(x, y);

	return newStack;
}

template<typename T>
inline std::vector<std::vector<uint16_t>> GStack<T>::createHeightfield(uint16_t length)
{
	std::vector<std::vector<uint16_t>> heightfield (1);
	heightfield[0].resize(1);
	heightfield[0][0] = 1;

	return heightfield;
}

template<typename T>
inline void GStack<T>::createInterval(T value, const std::vector<std::vector<uint16_t>>& heightfield)
{
	Interval<T> interval;
	interval._value = value;
	interval._length = heightfield;

	_intervals.push_back(interval);
}

template<typename T>
inline bool GStack<T>::isWildcard(uint8_t interval)
{
	assert(interval < this->getNumIntervals());
	return _intervals[interval]._value == std::numeric_limits<T>::max();
}

template<typename T>
inline void GStack<T>::mergeInterval(Interval<T>& interval, GStack<T>* root, std::vector<GStack<T>*>& stacks, std::vector<uint8_t>& layers)
{
	interval._value = stacks[0]->_intervals[layers[0]]._value;
	interval._length = std::vector<std::vector<uint16_t>>(root->_maxPoints.x - root->_minPoints.x, std::vector<uint16_t>(root->_maxPoints.y - root->_minPoints.y, 0));

	for (size_t i = 0; i < stacks.size(); ++i)
	{
		for (unsigned x = stacks[i]->_minPoints.x; x < stacks[i]->_maxPoints.x; ++x)
			for (unsigned y = stacks[i]->_minPoints.y; y < stacks[i]->_maxPoints.y; ++y)
				interval._length[x - root->_minPoints.x][y - root->_minPoints.y] = stacks[i]->_intervals[layers[i]]._length[x - stacks[i]->_minPoints.x][y - stacks[i]->_minPoints.y];
	}
}

template<typename T>
inline void GStack<T>::mergeStacks(GStack<T>* root, std::vector<GStack<T>*>& stacks)
{
	std::vector<uint8_t> iteratorCount(stacks.size(), 0);
	mergeStacks(root, stacks, iteratorCount, 0);
}

template<typename T>
inline bool GStack<T>::operator==(const GStack<T>& stack)
{
	return stack._intervals.size() == _intervals.size() and this->areColorEqual(stack);
}

template<typename T>
inline void GStack<T>::removeInterval(uint8_t interval)
{
	_intervals.erase(_intervals.begin() + interval);
}

template<typename T>
inline uint8_t GStack<T>::getNumTerminalIntervals(uint8_t startingIndex)
{
	uint8_t count = 0;

	for (int i = startingIndex; i < _intervals.size(); ++i)
		count += uint8_t(_intervals[i]._value < std::numeric_limits<T>::max());

	return count;
}

template<typename T>
inline size_t GStack<T>::size() const
{
	long objectSize = 0;
	for (const Interval<T>& interval : _intervals)
		for (const std::vector<uint16_t>& heightfield : interval._length)
			objectSize += sizeof(uint16_t);

	for (int x = 0; x < 2; ++x)
		for (int y = 0; y < 2; ++y)
			if (_children[x][y])
				objectSize += _children[x][y]->size();
		
	return sizeof(T) * _intervals.size() + objectSize + sizeof(uvec2) * 2 + sizeof(GStack<T>*) * 4;
}

template<typename T>
inline void GStack<T>::updateBoundaries(uint32_t x, uint32_t y)
{
	_maxPoints = glm::max(_maxPoints, uvec2(x, y));
	_minPoints = glm::min(_minPoints, uvec2(x, y));
}

template<typename T>
inline void GStack<T>::updateBoundaries(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1)
{
	this->updateBoundaries(x0, y0);
	this->updateBoundaries(x1, y1);
}

template<typename T>
inline void GStack<T>::updateBoundaries(GStack<T>* gStack)
{
	this->updateBoundaries(gStack->_minPoints.x, gStack->_minPoints.y);
	this->updateBoundaries(gStack->_maxPoints.x, gStack->_maxPoints.y);
}

template<typename T>
inline void GStack<T>::swapIntervalWildcard(uint8_t interval)
{
	assert(interval < this->getNumIntervals());
}

template<typename T>
inline bool GStack<T>::mergeStacks(GStack<T>* root, std::vector<GStack<T>*>& stacks, std::vector<uint8_t>& iteratorCount, uint8_t depth)
{
	if (depth == iteratorCount.size())
	{
		bool same = true, wildcard = stacks[0]->isWildcard(iteratorCount[0]);

		for (size_t stackIdx = 1; stackIdx < stacks.size() && same && !wildcard; ++stackIdx)
		{
			same &= stacks[stackIdx]->_intervals[iteratorCount[stackIdx]]._value == stacks[stackIdx - 1]->_intervals[iteratorCount[stackIdx - 1]]._value;
			wildcard |= stacks[stackIdx]->isWildcard(iteratorCount[stackIdx]);
		}

		if (same && !wildcard)
		{
			Interval<T> interval;
			GStack<T>::mergeInterval(interval, root, stacks, iteratorCount);
			root->addInterval(interval);

			for (size_t stackIdx = 0; stackIdx < stacks.size() && same && !wildcard; ++stackIdx)
				stacks[stackIdx]->removeInterval(iteratorCount[stackIdx]);
		}

		return true;
	}
	else
	{
		size_t startingIndex = (depth > 0) ? iteratorCount[depth - 1] : 0;
		for (size_t i = startingIndex; i < stacks[depth]->getNumIntervals(); ++i)
		{
			iteratorCount[depth] = i;
			if (mergeStacks(root, stacks, iteratorCount, depth + 1) and depth > 0)
			{
				return true;
			}
		}
	}

	return false;
}