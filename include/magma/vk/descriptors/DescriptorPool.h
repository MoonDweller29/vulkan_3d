#pragma once
#include <vulkan/vulkan.hpp>
#include <unordered_map>
#include <vector>

#include "magma/vk/vulkan_common.h"

class DescriptorPool {
public:
    static uint32_t DEFAULT_SET_COUNT;

    DescriptorPool(
            vk::Device device,
            const std::unordered_map<vk::DescriptorType, uint32_t> &poolSizes,
            uint32_t maxSetCount);
    NON_COPYABLE(DescriptorPool);
    DescriptorPool(DescriptorPool &&otherPool);
    ~DescriptorPool();

    bool isFull() const { return _setCount == _maxSetCount; }
    vk::DescriptorSet allocateSet(vk::DescriptorSetLayout layout);
    //can return less descriptors than was required if pool gets full
    std::vector<vk::DescriptorSet> allocateSets(vk::DescriptorSetLayout layout, uint32_t count);

private:
    vk::Device _device;
    vk::DescriptorPool _pool;

    uint32_t _setCount;
    uint32_t _maxSetCount;
};
