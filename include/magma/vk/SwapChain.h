#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>

#include "magma/vk/physicalDevice/PhysicalDevice.h"
#include "magma/vk/LogicalDevice.h"
#include "magma/vk/Window.h"
#include "magma/vk/textures/CustomImageView.h"
#include "magma/vk/vulkan_common.h"

class SwapChain {
public:
    SwapChain(LogicalDevice &device, const Window &window);
    NON_COPYABLE(SwapChain);
    ~SwapChain();

    vk::SwapchainKHR     getSwapChain()   const { return _swapChain;                    }
    vk::Format           getImageFormat() const { return _imageFormat;                  }
    vk::Extent2D         getExtent()      const { return _extent;                       }
    size_t               getImgCount()       const { return _imageViews.size();            }
    vk::ImageView        getView(int i)   const { return _imageViews.at(i).getView();   }

private:
    vk::SwapchainKHR              _swapChain;
    vk::Device                    _device;
    std::vector<vk::Image>        _images;
    std::vector<CustomImageView>  _imageViews;
    vk::Format                    _imageFormat;
    vk::Extent2D                  _extent;

    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
    static vk::PresentModeKHR   chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
    static vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities, vk::Extent2D actualExtent);
    static uint32_t     chooseImageCount(const vk::SurfaceCapabilitiesKHR &capabilities);

    void acquireImages();
    void createImageViews();
};