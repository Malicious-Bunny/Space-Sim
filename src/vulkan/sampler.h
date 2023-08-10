#pragma once

#include "device.h"

class Sampler {
public:
	Sampler(Device& device);
	~Sampler();

	inline VkSampler GetSampler() { return m_Sampler; }

private:
	Device& m_Device;
	VkSampler m_Sampler;
};