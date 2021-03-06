#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>

#include "magma/vk/pipeline/PipelineInfo.h"
#include "magma/vk/ShaderModule.h"
#include "magma/vk/vulkan_common.h"

class GraphicsPipeline {
public:
    GraphicsPipeline(vk::Device device,
        const std::vector<vk::PipelineShaderStageCreateInfo> &shaderStages,
        const PipelineInfo &pipelineInfo, vk::RenderPass renderPass);
    NON_COPYABLE(GraphicsPipeline);
    ~GraphicsPipeline();

    vk::Pipeline        getPipeline()       const { return _graphicsPipeline;   }
    vk::PipelineLayout  getPipelineLayout() const { return _pipelineLayout;     }

private:
    vk::Device          _device;
    vk::PipelineLayout  _pipelineLayout;
    vk::Pipeline        _graphicsPipeline;

};