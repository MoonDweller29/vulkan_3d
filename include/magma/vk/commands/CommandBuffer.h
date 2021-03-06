#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#include "magma/vk/vulkan_common.h"

class CommandBuffer {
public:
    CommandBuffer(vk::Device device, vk::CommandPool commandPool);
    NON_COPYABLE(CommandBuffer);
    ~CommandBuffer();

    const vk::CommandBuffer &getCmdBuf() const { return _commandBuffer; }

    vk::CommandBuffer begin(vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlags());
    void end();
    void submit_sync(vk::Queue queue);
    void endAndSubmit_sync(vk::Queue queue);
    void reset(vk::CommandBufferResetFlags flag=vk::CommandBufferResetFlags());

private:
    vk::Device          _device;
    vk::CommandPool     _commandPool;
    vk::CommandBuffer   _commandBuffer;
};