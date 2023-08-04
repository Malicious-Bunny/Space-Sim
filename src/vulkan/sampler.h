#pragma once

#include "device.h"

class Sampler {
public:
	Sampler(Device& device);
	~Sampler();

	inline VkSampler GetSampler() { return m_Sampler; }

	void CreateSimpleSampler();
	void CreateCubemapSampler();

private:
	Device& m_Device;
	VkSampler m_Sampler;
};