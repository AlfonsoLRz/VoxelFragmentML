#pragma once

class ResourceTracker
{
protected:
	std::ofstream _stream;

public:
	enum MeasurementType { CPU, RAM, VIRTUAL_MEMORY, GPU, NUM_MEASUREMENT_TYPES };
	const static char* Measurements_STR[NUM_MEASUREMENT_TYPES];

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
	bool			_interrupt;
	std::thread		_thread;

	// CPU
	ULARGE_INTEGER	_lastCPU, _lastSysCPU, _lastUserCPU;
	int				_numProcessors;
	HANDLE			_self;

protected:
	void initCPUResources();
	void measureCPUUsage(Measurement& measurement);
	void measureGPUMemory(Measurement& measurement);
	void measureMemoryUsage(Measurement& measurement);
	void threadedWatch(long waitMilliseconds);

public:
	ResourceTracker();
	~ResourceTracker();

	bool openStream(const std::string& filename);
	bool closeStream();

	Measurement measure();
	static void print(const Measurement& measurement);
	void track(long waitMilliseconds = 1000);
};

