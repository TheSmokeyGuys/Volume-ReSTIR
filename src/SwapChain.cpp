#include "SwapChain.hpp"

#include <vector>

#include "Window.hpp"

#include "spdlog/spdlog.h"

namespace volume_restir {
// Specify the color channel format and color space type
VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) {
  // VK_FORMAT_UNDEFINED indicates that the surface has no preferred format, so
  // we can choose any
  if (availableFormats.size() == 1 &&
      availableFormats[0].format == VK_FORMAT_UNDEFINED) {
    return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
  }

  // Otherwise, choose a preferred combination
  for (const auto& availableFormat : availableFormats) {
    // Ideal format and color space
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }

  // Otherwise, return any format
  return availableFormats[0];
}

// Specify the presentation mode of the swap chain
VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> availablePresentModes) {
  // Second choice
  VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

  for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      // First choice
      return availablePresentMode;
    } else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
      // Third choice
      bestMode = availablePresentMode;
    }
  }

  return bestMode;
}

// Specify the swap extent (resolution) of the swap chain
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities,
                            GLFWwindow* window) {
  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height)};

    actualExtent.width = std::max(
        capabilities.minImageExtent.width,
        std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(
        capabilities.minImageExtent.height,
        std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
  }
}

SwapChain::SwapChain(volume_restir::RenderContext* a_renderContext,
                     VkSurfaceKHR vkSurface)
    : renderContext(a_renderContext), vkSurface(vkSurface) {
  Create();

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  if (vkCreateSemaphore( a_renderContext->Device().device,
          &semaphoreInfo, nullptr,
                        &imageAvailableSemaphore) != VK_SUCCESS ||
      vkCreateSemaphore(a_renderContext->Device().device, &semaphoreInfo,
                        nullptr,
                        &renderFinishedSemaphore) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create semaphores");
  }
}

void SwapChain::Create() {

    // create swapchain
  vkb::SwapchainBuilder swapchain_builder{renderContext->Device()};
  auto swapchain_success =
      swapchain_builder.set_old_swapchain(swapchain_).build();
  if (!swapchain_success) {
    spdlog::error("Failed to create swapchain: {}",
                  swapchain_success.error().message());
    throw std::runtime_error("Failed to create swapchain");
  }
  vkb::destroy_swapchain(swapchain_);
  swapchain_ = swapchain_success.value();
  spdlog::debug("Successfully created Vulkan swapchain");

}

void SwapChain::Destroy() {
  //vkDestroySwapchainKHR(renderContext->Device().device, vkSwapChain, nullptr);

  vkDestroySurfaceKHR(renderContext->Instance().instance, vkSurface,
                      renderContext->Instance().allocation_callbacks);
  vkb::destroy_swapchain(swapchain_);
}

vkb::Swapchain SwapChain::GetVkBSwapChain() const { return swapchain_; }

VkFormat SwapChain::GetVkImageFormat() const { return vkSwapChainImageFormat; }

VkExtent2D SwapChain::GetVkExtent() const { return vkSwapChainExtent; }

uint32_t SwapChain::GetIndex() const { return imageIndex; }

uint32_t SwapChain::GetCount() const {
  return static_cast<uint32_t>(vkSwapChainImages.size());
}

VkImage SwapChain::GetVkImage(uint32_t index) const {
  return vkSwapChainImages[index];
}

VkSemaphore SwapChain::GetImageAvailableVkSemaphore() const {
  return imageAvailableSemaphore;
}

VkSemaphore SwapChain::GetRenderFinishedVkSemaphore() const {
  return renderFinishedSemaphore;
}

void SwapChain::Recreate() {
  Destroy();
  Create();
}

bool SwapChain::Acquire() {
  //if (ENABLE_VALIDATION) {
  //  // the validation layer implementation expects the application to explicitly
  //  // synchronize with the GPU
  //  vkQueueWaitIdle(device->GetQueue(QueueFlags::Present));
  //}
  VkResult result = vkAcquireNextImageKHR(
      renderContext->Device().device, swapchain_.swapchain, std::numeric_limits<uint64_t>::max(),
      imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("Failed to acquire swap chain image");
  }

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    Recreate();
    return false;
  }

  return true;
}

bool SwapChain::Present() {
 // VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};

  // Submit result back to swap chain for presentation
 /* VkPresentInfoKHR presentInfo   = {};
  presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapchain_.swapchain};
  presentInfo.swapchainCount  = 1;
  presentInfo.pSwapchains     = swapChains;
  presentInfo.pImageIndices   = &imageIndex;
  presentInfo.pResults        = nullptr;

  VkResult result =
      vkQueuePresentKHR(device->GetQueue(QueueFlags::Present), &presentInfo);

  if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to present swap chain image");
  }*/


  // get present queue
  auto present_queue =
      renderContext->Device().get_queue(vkb::QueueType::present);
  if (!present_queue.has_value()) {
    spdlog::error("Failed to get present queue: {}",
                  present_queue.error().message());
    throw std::runtime_error("Failed to get present queue");
  }

  if (present_queue.vk_result() == VK_ERROR_OUT_OF_DATE_KHR ||
      present_queue.vk_result() == VK_SUBOPTIMAL_KHR) {
    Recreate();
    return false;
  }

  return true;
}

SwapChain::~SwapChain() {
  vkDestroySemaphore(renderContext->Device().device, imageAvailableSemaphore, nullptr);
  vkDestroySemaphore(renderContext->Device().device, renderFinishedSemaphore, nullptr);
  Destroy();
}

}  // namespace volume_restir