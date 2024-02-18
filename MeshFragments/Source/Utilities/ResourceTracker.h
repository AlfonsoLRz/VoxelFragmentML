#pragma once

#include "Utilities/Singleton.h"

class ResourceTracker: public Singleton<ResourceTracker>
{
	friend class Singleton<ResourceTracker>;

protected:
	std::ofstream _stream;

public:
	enum MeasurementType { CPU, RAM, VIRTUAL_MEMORY, GPU, NUM_MEASUREMENT_TYPES };
	const static char* Measurements_STR[NUM_MEASUREMENT_TYPES];

	enum EventType { MEMORY_ALLOCATION, MODEL_LOAD, VOXELIZATION, FRACTURE, DATA_TYPE_CONVERSION, STORAGE, NULL_EVENT, NUM_EVENTS };
	const static char* Event_STR[NUM_EVENTS];

	struct Measurement
	{
		float _measurement[NUM_MEASUREMENT_TYPES];
		float _total[NUM_MEASUREMENT_TYPES];

		Measurement()
		{
			for (int i = 0; i < NUM_MEASUREMENT_TYPES; ++i) _measurement[i] = _total[i] = 0.f;
		}

		void write(std::ofstream& stream)
		{
			for (int i = 0; i < NUM_MEASUREMENT_TYPES; ++i)
				stream << _measurement[i] << " " << _total[i] << " ";
			stream << std::endl;
		}
	};

protected:
	EventType 				_currentEvent;
	bool					_interrupt;
	std::thread				_thread;
	std::binary_semaphore	_writingSemaphore;

	// CPU
	ULARGE_INTEGER	_lastCPU, _lastSysCPU, _lastUserCPU;
	int				_numProcessors;
	HANDLE			_self;

protected:
	ResourceTracker();

	void initCPUResources();
	void measureCPUUsage(Measurement& measurement);
	void measureGPUMemory(Measurement& measurement);
	void measureMemoryUsage(Measurement& measurement);
	void threadedWatch(long waitMilliseconds);

public:
	~ResourceTracker();

	bool openStream(const std::string& filename);
	bool closeStream();

	Measurement measure();
	static void print(const Measurement& measurement);
	void recordEvent(const EventType eventType);
	void recordFilename(const std::string& filename);
	void track(long waitMilliseconds = 1000);
};

