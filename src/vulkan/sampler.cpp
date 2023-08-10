#include "sampler.h"

#include "stdexcept"

Sampler::Sampler(Device& device): m_Device(device) 
{
	VkSamplerCreateInfo samplerInfo {};
	samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter               = VK_FILTER_LINEAR;
	samplerInfo.minFilter               = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.mipLodBias              = 0.0f;
	samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
	samplerInfo.minLod                  = 0.0f;
	samplerInfo.maxLod                  = 0.0f;
	samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.maxAnisotropy           = m_Device.GetDeviceProperties().limits.maxSamplerAnisotropy;
	samplerInfo.anisotropyEnable        = VK_TRUE;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable           = VK_FALSE;
	if(vkCreateSampler(m_Device.GetDevice(), &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS) { throw std::runtime_error("failed to create texture sampler!"); }
}

Sampler::~Sampler() 
{ 
	vkDestroySampler(m_Device.GetDevice(), m_Sampler, nullptr); 
}