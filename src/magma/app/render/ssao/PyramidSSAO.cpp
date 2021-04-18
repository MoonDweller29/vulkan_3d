#include "magma/app/render/ssao/PyramidSSAO.h"

#include "magma/app/render/GBuffer.h"
#include "magma/vk/textures/TexPyramid.h"


PyramidSSAO::PyramidSSAO(vk::Device device, Texture ssaoTex, int pyramidSize, Queue queue) :
        _device(device), _ssaoTex(ssaoTex), _queue(queue),
        _extent(toExtent2D(_ssaoTex.getInfo()->imageInfo.extent)),
        _cmdBuf(device, queue.cmdPool),
        _renderFinished(device),
        _depthNormalsSampler(createDepthNormalsSampler()),
        _noiseSampler(createNoiseSampler()),
        _depthPyramidSampler(createDepthPyramidSampler()),
        _renderPass(std::move(createRenderPass())),
        _frameBuffer(_device, {_ssaoTex.getView()},
                     _renderPass.get(), _extent),
        _descriptorSetLayout(_device, DescriptorSetLayoutInfo()
                .addCombinedImageSampler(vk::ShaderStageFlagBits::eFragment)
                .addCombinedImageSampler(vk::ShaderStageFlagBits::eFragment)
                .addCombinedImageSampler(vk::ShaderStageFlagBits::eFragment)
                .addArrayOfCombinedImageSamplers(vk::ShaderStageFlagBits::eFragment, pyramidSize)
                .addUniformBuffer(1, vk::ShaderStageFlagBits::eFragment)
                .addUniformBuffer(1, vk::ShaderStageFlagBits::eFragment)
        )
{
    PipelineLayoutInfo pipelineLayoutInfo(_descriptorSetLayout.getLayout());
    PipelineInfo pipelineInfo(_extent);
    pipelineInfo.setLayout(pipelineLayoutInfo);

    Shader vertShader("imageProcessVert", _device, "shaders/imageProcess.vert.spv", Shader::Stage::VERT_SH);
    Shader fragShader("pyramidSSAOFrag", _device, "shaders/ssao/pyramidSSAO.frag.spv", Shader::Stage::FRAG_SH);
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = {
            vertShader.getStageInfo(),
            fragShader.getStageInfo()
    };
    _graphicsPipeline = std::make_unique<GraphicsPipeline>(_device, shaderStages, pipelineInfo, _renderPass.get());
}

vk::UniqueRenderPass PyramidSSAO::createRenderPass() {
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format         = _ssaoTex.getInfo()->imageInfo.format;
    colorAttachment.samples        = vk::SampleCountFlagBits::e1; //for multisampling
    colorAttachment.loadOp         = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.storeOp        = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout  = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout    = vk::ImageLayout::eShaderReadOnlyOptimal;

    vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = nullptr;

    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    //external dependency subpass
    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0; //this subpass
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = vk::AccessFlags();
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    auto [result, renderPass] = _device.createRenderPassUnique(renderPassInfo);
    VK_CHECK_ERR(result, "failed to create render pass!");

    return std::move(renderPass);
}

vk::UniqueSampler PyramidSSAO::createDepthNormalsSampler() {
    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.magFilter = vk::Filter::eNearest;
    samplerInfo.minFilter = vk::Filter::eNearest;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = vk::CompareOp::eAlways;

    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    auto[result, sampler] = _device.createSamplerUnique(samplerInfo);
    VK_CHECK_ERR(result, "failed to create texture sampler!");

    return std::move(sampler);
}

vk::UniqueSampler PyramidSSAO::createNoiseSampler() {
    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.magFilter = vk::Filter::eNearest;
    samplerInfo.minFilter = vk::Filter::eNearest;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = vk::CompareOp::eAlways;

    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    auto[result, sampler] = _device.createSamplerUnique(samplerInfo);
    VK_CHECK_ERR(result, "failed to create texture sampler!");

    return std::move(sampler);
}

vk::UniqueSampler PyramidSSAO::createDepthPyramidSampler() {
    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = vk::CompareOp::eAlways;

    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    auto[result, sampler] = _device.createSamplerUnique(samplerInfo);
    VK_CHECK_ERR(result, "failed to create texture sampler!");

    return std::move(sampler);
}

void PyramidSSAO::writeDescriptorSets(
        const GBuffer &gBuffer,
        vk::ImageView noiseTexView,
        const TexPyramid &depthPyramid,
        const Buffer &uniformBuffer, uint32_t uboSize,
        const Buffer &inverseProjUniform, uint32_t invProjUboSize
) {
    int pyramidSize = depthPyramid.getSize();
    std::vector<CombinedImageSampler> pyramidImgs;
    for (int i = 0; i < pyramidSize; ++i) {
        pyramidImgs.emplace_back(depthPyramid[i].getView(), _depthPyramidSampler.get());
    }
    _descriptorSetLayout.allocateSets(1);
    _descriptorSetLayout.beginSet(0);
    {
        _descriptorSetLayout.bindCombinedImageSampler(0, gBuffer.getDepth().getView(), _depthNormalsSampler.get());
        _descriptorSetLayout.bindCombinedImageSampler(1, gBuffer.getNormals().getView(), _depthNormalsSampler.get());
        _descriptorSetLayout.bindCombinedImageSampler(2, noiseTexView, _noiseSampler.get());
        _descriptorSetLayout.bindArrayOfCombinedImageSamplers(3, pyramidImgs);
        _descriptorSetLayout.bindUniformBuffer(4, uniformBuffer.getBuf(), 0, uboSize);
        _descriptorSetLayout.bindUniformBuffer(5, inverseProjUniform.getBuf(), 0, invProjUboSize);
    }
    _descriptorSet = _descriptorSetLayout.recordAndReturnSets()[0];
}

void PyramidSSAO::recordCmdBuffers() {
    vk::CommandBuffer cmdBuf = _cmdBuf.begin();
    {
        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = _renderPass.get();
        renderPassInfo.framebuffer = _frameBuffer.getFrameBuf();

        renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
        renderPassInfo.renderArea.extent = _extent;

        renderPassInfo.clearValueCount = 0;
        renderPassInfo.pClearValues = nullptr;

        cmdBuf.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
        {
            cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, _graphicsPipeline->getPipeline());
            cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                      _graphicsPipeline->getPipelineLayout(), 0, 1, &_descriptorSet, 0, nullptr);
            cmdBuf.draw(3, 1, 0, 0);
        }
        cmdBuf.endRenderPass();
    }
    _cmdBuf.end();
}

const CmdSync &PyramidSSAO::draw(
        const std::vector<vk::Semaphore> &waitSemaphores,
        const std::vector<vk::Fence> &waitFences
) {
    vk::Result result;
    if (!waitFences.empty()) {
        result = _device.waitForFences(waitFences, VK_TRUE, UINT64_MAX);
    }

    vk::SubmitInfo submitInfo;

    std::vector<vk::PipelineStageFlags> waitStages(waitSemaphores.size(), vk::PipelineStageFlagBits::eEarlyFragmentTests);
    if (waitSemaphores.empty()) {
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = nullptr;
    } else if (
            std::find(waitSemaphores.begin(), waitSemaphores.end(), _renderFinished.getSemaphore()) == waitSemaphores.end()
            ) {
        submitInfo.waitSemaphoreCount = waitSemaphores.size();
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();
    } else {
        LOG_AND_THROW std::logic_error("cyclic dependency in render graph");
    }

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_cmdBuf.getCmdBuf();

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &_renderFinished.getSemaphore();

    _renderFinished.resetFence();

    result = _queue.queue.submit(1, &submitInfo, _renderFinished.getFence());
    VK_CHECK_ERR(result, "failed to submit draw command buffer!");

    return _renderFinished;
}