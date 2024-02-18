#include "stdafx.h"
#include "ResourceTracker.h"

#include "ChronoUtilities.h"
#include "Graphics/Core/ComputeShader.h"

const char* ResourceTracker::Measurements_STR[] = { "CPU", "RAM", "VIRTUAL_MEMORY", "GPU"};
const char* ResourceTracker::Event_STR[] = { "MEMORY_ALLOCATION", "VOXELIZATION", "LOAD_MODEL", "FRAGMENTATION", "DATA_TYPE_CONVERSION", "STORAGE", "NULL" };

// Public methods

ResourceTracker::ResourceTracker(): _currentEvent(NULL_EVENT), _writingSemaphore(1)
{
	this->initCPUResources();
}

ResourceTracker::~ResourceTracker()
{

}

bool ResourceTracker::openStream(const std::string& filename)
{
	auto memory = ComputeShader::getMaxUsableMemory() / 1024;
	_interrupt = false;

	_stream = std::ofstream(filename, std::ios::out);
	if (_stream.is_open())
		return true;

	throw std::runtime_error("Failed to open file " + filename);
}

bool ResourceTracker::closeStream()
{
	_interrupt = true;
	_thread.join();
	return true;
}

ResourceTracker::Measurement ResourceTracker::measure()
{
	Measurement measurement;
	this->measureCPUUsage(measurement);
	this->measureMemoryUsage(measurement);
	this->measureGPUMemory(measurement);

	return measurement;
}

void ResourceTracker::print(const Measurement& measurement)
{
	for (int i = 0; i < NUM_MEASUREMENT_TYPES; ++i)
		std::cout << Measurements_STR[i] << ": " << measurement._measurement[i] << " " << measurement._total[i] << std::endl;
	std::cout << "----------------------------------------" << std::endl;
}

void ResourceTracker::recordEvent(const EventType eventType)
{
	if (_currentEvent != eventType)
	{
		if (_currentEvent != NULL_EVENT)
		{
			_writingSemaphore.acquire();
			_stream << "* " << Event_STR[_currentEvent] << ": " << ChronoUtilities::getDuration() << " ms" << std::endl;
			_writingSemaphore.release();
		}

		ChronoUtilities::initChrono();
		_currentEvent = eventType;
	}
}

void ResourceTracker::recordFilename(const std::string& filename)
{
	_writingSemaphore.acquire();
	_stream << "- " << filename << std::endl;
	_writingSemaphore.release();
}

void ResourceTracker::track(long waitMilliseconds)
{
	_thread = std::thread(&ResourceTracker::threadedWatch, this, waitMilliseconds);
}

// Protected methods

void ResourceTracker::initCPUResources()
{
	SYSTEM_INFO sysInfo;
	FILETIME ftime, fsys, fuser;

	GetSystemInfo(&sysInfo);
	_numProcessors = sysInfo.dwNumberOfProcessors;

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&_lastCPU, &ftime, sizeof(FILETIME));

	_self = GetCurrentProcess();
	GetProcessTimes(_self, &ftime, &ftime, &fsys, &fuser);
	memcpy(&_lastSysCPU, &fsys, sizeof(FILETIME));
	memcpy(&_lastUserCPU, &fuser, sizeof(FILETIME));
}

void ResourceTracker::measureCPUUsage(Measurement& measurement)
{
	FILETIME ftime, fsys, fuser;
	ULARGE_INTEGER now, sys, user;
	double percent;

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&now, &ftime, sizeof(FILETIME));

	GetProcessTimes(_self, &ftime, &ftime, &fsys, &fuser);
	memcpy(&sys, &fsys, sizeof(FILETIME));
	memcpy(&user, &fuser, sizeof(FILETIME));
	percent = (sys.QuadPart - _lastSysCPU.QuadPart) + (user.QuadPart - _lastUserCPU.QuadPart);
	percent /= (now.QuadPart - _lastCPU.QuadPart);
	percent /= _numProcessors;
	_lastCPU = now;
	_lastUserCPU = user;
	_lastSysCPU = sys;

	measurement._measurement[CPU] = (float)percent * 100.f;
	measurement._total[CPU] = 100;
}

void ResourceTracker::measureGPUMemory(Measurement& measurement)
{
	measurement._measurement[GPU] = ComputeShader::getMemoryFootprint() / 1024 / 1024;
	measurement._total[GPU] = ComputeShader::getMaxUsableMemory() / 1024;
}

void ResourceTracker::measureMemoryUsage(Measurement& measurement)
{
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);

	PROCESS_MEMORY_COUNTERS_EX pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));

	// Virtual memory
	DWORDLONG totalVirtualMemory = memInfo.ullTotalPageFile;
	SIZE_T virtualMemoryUsedByMe = pmc.PrivateUsage;

	// RAM
	DWORDLONG totalRAMMemory = memInfo.ullTotalPhys;
	SIZE_T RAMMemORYUsedByMe = pmc.WorkingSetSize;

	measurement._measurement[RAM] = (float)RAMMemORYUsedByMe / 1024 / 1024;
	measurement._total[RAM] = (float)totalRAMMemory / 1024 / 1024;

	measurement._measurement[VIRTUAL_MEMORY] = (float)virtualMemoryUsedByMe / 1024 / 1024;
	measurement._total[VIRTUAL_MEMORY] = (float)totalVirtualMemory / 1024 / 1024;
}

void ResourceTracker::threadedWatch(long waitMilliseconds)
{
	Measurement measurement;
	while (!_interrupt)
	{
		this->measureCPUUsage(measurement);
		this->measureMemoryUsage(measurement);
		this->measureGPUMemory(measurement);

		// Write measurements to file
		if (!_interrupt && _stream.is_open())
		{
			_writingSemaphore.acquire();
			measurement.write(_stream);
			_writingSemaphore.release();
			std::this_thread::sleep_for(std::chrono::milliseconds(waitMilliseconds));
		}
		else
		{
			_stream.close();
			break;
		}
	}
}
