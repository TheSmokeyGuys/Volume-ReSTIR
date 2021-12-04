#include "Renderer.hpp"

#include <fstream>
#include <stdexcept>

#include "ShaderModule.hpp"
#include "SingtonManager.hpp"
#include "config/static_config.hpp"
#include "config/build_config.h"
#include "model/Cube.hpp"
#include "spdlog/spdlog.h"
#include"nvh/fileoperations.hpp"

bool IgnorePointLight = true;

namespace volume_restir {

Renderer::Renderer() {
  float aspect_ratio =
      static_config::kWindowWidth * 1.0f / static_config::kWindowHeight;

  render_context_ = std::make_unique<RenderContext>();

  swapchain_ = std::make_unique<nvvk::SwapChain>(
      render_context_->GetNvvkContext().m_device,
      render_context_->GetNvvkContext().m_physicalDevice,
      render_context_->GetQueues()[QueueFlags::GRAPHICS],
      render_context_->GetQueueFamilyIndices()[QueueFlags::GRAPHICS],
      render_context_->Surface());

  swapchain_->update(static_config::kWindowWidth, static_config::kWindowHeight);

  camera_ = std::make_unique<Camera>(
      RenderContextPtr(), static_config::kFOVInDegrees, aspect_ratio);
  scene_ = std::make_unique<Scene>(RenderContextPtr());

  SingletonManager::GetWindow().BindCamera(camera_.get());

  // CreateSwapChain();  // CreateImageViews()
  CreateRenderPass();
  CreateCameraDiscriptorSetLayout();
  CreateDescriptorPool();
  CreateCameraDescriptorSet();
  CreateFrameResources();

  CreateGraphicsPipeline();
  CreateCommandPools();
  RecordCommandBuffers();

  //NVVK Stuff
  m_alloc.init(render_context_->GetNvvkContext().m_device,
               render_context_->GetNvvkContext().m_physicalDevice);
  m_debug.setup(render_context_->GetNvvkContext().m_device);
};

void Renderer::CreateScene(std::string scenefile) {


    std::vector<std::string> prjctDir{PROJECT_DIRECTORY};

  std::string filename = nvh::findFile(scenefile, prjctDir);

    m_gltfLoad.LoadScene(scenefile);

  if (IgnorePointLight) {
      m_gltfLoad.m_gltfScene.m_lights.clear();
  }
  // Create descriptor set Pool // Already Done 
  
  //TODO
  //Create Scene Buffers
  m_sceneBuffers.create(m_gltfLoad.m_gltfScene, m_gltfLoad.m_tmodel, &m_alloc, render_context_->GetNvvkContext().m_device,
                        render_context_->GetNvvkContext().m_physicalDevice, render_context_->GetQueueFamilyIndex(QueueFlags::GRAPHICS));
   
 
  //CreateSceneBuffers();
  CreateDescriptorSetScene(); 

  //TODO Create buffers for scene
  //m_sceneBuffers.create(m_gltfScene, m_tmodel, &m_alloc, m_device,
  //                      m_physicalDevice, m_graphicsQueueIndex);

  //
 
}

//[[nodiscard]] void CreateSceneBuffers(const nvh::GltfScene& gltfScene,
//                          tinygltf::Model& tmodel, nvvk::Allocator* alloc,
//                          const vk::Device& device,
//                          const vk::PhysicalDevice& physicalDevice,
//                          uint32_t graphicsQueueIndex) {
//  m_debug.setup(device);
//  m_alloc              = alloc;
//  m_device             = device;
//  m_graphicsQueueIndex = graphicsQueueIndex;
//  m_physicalDevice     = physicalDevice;
//
//  using vkBU = vk::BufferUsageFlagBits;
//  using vkMP = vk::MemoryPropertyFlagBits;
//  nvvk::CommandPool cmdBufGet(device, graphicsQueueIndex);
//  vk::CommandBuffer cmdBuf = cmdBufGet.createCommandBuffer();
//
//  m_pointLights    = collectPointLights(gltfScene);
//  m_triangleLights = collectTriangleLights(gltfScene);
//  if (m_pointLights.empty() && m_triangleLights.empty()) {
//    m_pointLights = generatePointLights(gltfScene.m_dimensions.min,
//                                        gltfScene.m_dimensions.max);
//  }
//
//  _loadEnvironment();
//
//  // Lights
//  m_pointLightCount = m_pointLights.size();
//  std::vector<float> pdf;
//  if (!m_pointLights.empty()) {
//    for (auto& lt : m_pointLights) {
//      pdf.push_back(lt.emission_luminance.w);
//    }
//  } else {
//    for (auto& lt : m_triangleLights) {
//      float triangleLightPower = lt.emission_luminance.w * lt.normalArea.w;
//      pdf.push_back(triangleLightPower);
//    }
//  }
//  std::vector<shader::aliasTableCell> aliasTable = createAliasTable(pdf);
//  m_aliasTableCount  = static_cast<uint32_t>(aliasTable.size());
//  m_aliasTableBuffer = alloc->createBuffer(
//      cmdBuf, aliasTable, vkBU::eStorageBuffer, vkMP::eDeviceLocal);
//
//  std::cout << "Point Lights Num: " << m_pointLightCount << std::endl;
//  if (m_pointLightCount == 0) {
//    // Dummy
//    m_pointLights.push_back(shader::pointLight{});
//  }
//  m_ptLightsBuffer = alloc->createBuffer(
//      cmdBuf, m_pointLights, vkBU::eStorageBuffer, vkMP::eDeviceLocal);
//
//  m_triangleLightCount = m_triangleLights.size();
//  std::cout << "Tri Lights Num: " << m_triangleLightCount << std::endl;
//  if (m_triangleLightCount == 0) {
//    // Dummy
//    m_triangleLights.push_back(shader::triangleLight{});
//  }
//  m_triangleLightsBuffer = alloc->createBuffer(
//      cmdBuf, m_triangleLights, vkBU::eStorageBuffer, vkMP::eDeviceLocal);
//
//  m_vertices =
//      alloc->createBuffer(cmdBuf, gltfScene.m_positions,
//                          vkBU::eStorageBuffer | vkBU::eShaderDeviceAddress);
//  m_indices =
//      alloc->createBuffer(cmdBuf, gltfScene.m_indices,
//                          vkBU::eStorageBuffer | vkBU::eShaderDeviceAddress);
//  m_normals =
//      alloc->createBuffer(cmdBuf, gltfScene.m_normals, vkBU::eStorageBuffer);
//  m_texcoords =
//      alloc->createBuffer(cmdBuf, gltfScene.m_texcoords0, vkBU::eStorageBuffer);
//  m_tangents =
//      alloc->createBuffer(cmdBuf, gltfScene.m_tangents, vkBU::eStorageBuffer);
//  m_colors =
//      alloc->createBuffer(cmdBuf, gltfScene.m_colors0, vkBU::eStorageBuffer);
//
//  std::vector<shader::GltfMaterials> shadeMaterials;
//  for (auto& m : gltfScene.m_materials) {
//    shader::GltfMaterials smat;
//    smat.pbrBaseColorFactor           = m.pbrBaseColorFactor;
//    smat.pbrBaseColorTexture          = m.pbrBaseColorTexture;
//    smat.pbrMetallicFactor            = m.pbrMetallicFactor;
//    smat.pbrRoughnessFactor           = m.pbrRoughnessFactor;
//    smat.pbrMetallicRoughnessTexture  = m.pbrMetallicRoughnessTexture;
//    smat.khrDiffuseFactor             = m.khrDiffuseFactor;
//    smat.khrSpecularFactor            = m.khrSpecularFactor;
//    smat.khrDiffuseTexture            = m.khrDiffuseTexture;
//    smat.shadingModel                 = m.shadingModel;
//    smat.khrGlossinessFactor          = m.khrGlossinessFactor;
//    smat.khrSpecularGlossinessTexture = m.khrSpecularGlossinessTexture;
//    smat.emissiveTexture              = m.emissiveTexture;
//    smat.emissiveFactor               = m.emissiveFactor;
//    smat.alphaMode                    = m.alphaMode;
//    smat.alphaCutoff                  = m.alphaCutoff;
//    smat.doubleSided                  = m.doubleSided;
//    smat.normalTexture                = m.normalTexture;
//    smat.normalTextureScale           = m.normalTextureScale;
//    smat.uvTransform                  = m.uvTransform;
//    shadeMaterials.emplace_back(smat);
//  }
//
//  m_materials =
//      alloc->createBuffer(cmdBuf, shadeMaterials, vkBU::eStorageBuffer);
//  std::vector<shader::ModelMatrices> nodeMatrices;
//  for (auto& node : gltfScene.m_nodes) {
//    shader::ModelMatrices mat;
//    mat.transform                  = node.worldMatrix;
//    mat.transformInverseTransposed = invert(node.worldMatrix);
//    nodeMatrices.emplace_back(mat);
//  }
//  m_matrices = alloc->createBuffer(cmdBuf, nodeMatrices, vkBU::eStorageBuffer);
//
//  vk::Format format = vk::Format::eR8G8B8A8Unorm;
//
//  using vkIU = vk::ImageUsageFlagBits;
//
//  vk::SamplerCreateInfo samplerCreateInfo{{},
//                                          vk::Filter::eLinear,
//                                          vk::Filter::eLinear,
//                                          vk::SamplerMipmapMode::eLinear};
//  samplerCreateInfo.setMaxLod(FLT_MAX);
//  // format = vk::Format::eR8G8B8A8Srgb;
//
//  auto addDefaultTexture = [this, cmdBuf, alloc]() {
//    std::array<uint8_t, 4> white = {255, 255, 255, 255};
//    m_textures.emplace_back(alloc->createTexture(
//        cmdBuf, 4, white.data(),
//        nvvk::makeImage2DCreateInfo(vk::Extent2D{1, 1}), {}));
//    m_debug.setObjectName(m_textures.back().image, "dummy");
//  };
//  if (tmodel.images.empty()) {
//    // No images, add a default one.
//    addDefaultTexture();
//  } else {
//    m_textures.resize(tmodel.textures.size());
//    // load textures
//    for (int i = 0; i < tmodel.textures.size(); ++i) {
//      int sourceImage = tmodel.textures[i].source;
//      if (sourceImage >= tmodel.images.size() || sourceImage < 0) {
//        // Incorrect source image
//        addDefaultTexture();
//        continue;
//      }
//      auto& gltfimage = tmodel.images[sourceImage];
//      if (gltfimage.width == -1 || gltfimage.height == -1 ||
//          gltfimage.image.empty()) {
//        // Image not present or incorrectly loaded (image.empty)
//        addDefaultTexture();
//        continue;
//      }
//      void* buffer            = &gltfimage.image[0];
//      VkDeviceSize bufferSize = gltfimage.image.size();
//      auto imgSize            = vk::Extent2D(gltfimage.width, gltfimage.height);
//
//      // std::cout << "Loading Texture: " << gltfimage.uri << std::endl;
//      if (tmodel.textures[i].sampler > -1) {
//        // Retrieve the texture sampler
//        auto gltfSampler  = tmodel.samplers[tmodel.textures[i].sampler];
//        samplerCreateInfo = _gltfSamplerToVulkan(gltfSampler);
//      }
//      vk::ImageCreateInfo imageCreateInfo =
//          nvvk::makeImage2DCreateInfo(imgSize, format, vkIU::eSampled, true);
//
//      nvvk::Image image =
//          alloc->createImage(cmdBuf, bufferSize, buffer, imageCreateInfo);
//      nvvk::cmdGenerateMipmaps(cmdBuf, image.image, format, imgSize,
//                               imageCreateInfo.mipLevels);
//      vk::ImageViewCreateInfo ivInfo =
//          nvvk::makeImageViewCreateInfo(image.image, imageCreateInfo);
//      m_textures[i] = alloc->createTexture(image, ivInfo, samplerCreateInfo);
//      m_debug.setObjectName(m_textures[i].image,
//                            std::string("Txt" + std::to_string(i)).c_str());
//    }
//  }
//  cmdBufGet.submitAndWait(cmdBuf);
//  alloc->finalizeAndReleaseStaging();
//
//  _createRtBuffer(gltfScene);
//}


//TODO this is for scene buffers
void Renderer:: CreateDescriptorSetScene(vk::DescriptorPool& staticDescPool) {
  using vkDT      = vk::DescriptorType;
  using vkSS      = vk::ShaderStageFlagBits;
  using vkDSLB    = vk::DescriptorSetLayoutBinding;
  auto nbTextures = static_cast<uint32_t>(m_textures.size());

  nvvk::DescriptorSetBindings bind;
  bind.addBinding(vkDSLB(B_ACCELERATION_STRUCTURE,
                         vkDT::eAccelerationStructureKHR, 1,
                         vkSS::eRaygenKHR | vkSS::eClosestHitKHR));  // TLAS
  bind.addBinding(
      vkDSLB(B_PLIM_LOOK_UP, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));
  bind.addBinding(
      vkDSLB(B_VERTICES, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));
  bind.addBinding(
      vkDSLB(B_NORMALS, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));
  bind.addBinding(
      vkDSLB(B_TEXCOORDS, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));
  bind.addBinding(
      vkDSLB(B_INDICES, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));
  bind.addBinding(
      vkDSLB(B_MATERIALS, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));
  bind.addBinding(
      vkDSLB(B_MATRICES, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));
  bind.addBinding(vkDSLB(B_TEXTURES, vkDT::eCombinedImageSampler, nbTextures,
                         vkSS::eClosestHitKHR));
  bind.addBinding(
      vkDSLB(B_TANGENTS, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));
  bind.addBinding(
      vkDSLB(B_COLORS, vkDT::eStorageBuffer, 1, vkSS::eClosestHitKHR));

  
  vk::Device device_ = render_context_->GetNvvkContext().m_device;

  scene_descriptorset_layout_ = bind.createLayout(device_);
  descriptor_pool_            = bind.createPool(device_);

  scene_descriptorset_         = device_.allocateDescriptorSets(
      {descriptor_pool_, 1, &scene_descriptorset_layout_})[0];

  vk::AccelerationStructureKHR tlas = m_rtBuilder.getAccelerationStructure();
  vk::WriteDescriptorSetAccelerationStructureKHR descASInfo;
  descASInfo.setAccelerationStructureCount(1);
  descASInfo.setPAccelerationStructures(&tlas);

  vk::DescriptorBufferInfo primInfo{m_primlooks.buffer, 0, VK_WHOLE_SIZE};
  vk::DescriptorBufferInfo verInfo{m_vertices.buffer, 0, VK_WHOLE_SIZE};
  vk::DescriptorBufferInfo norInfo{m_normals.buffer, 0, VK_WHOLE_SIZE};
  vk::DescriptorBufferInfo texInfo{m_texcoords.buffer, 0, VK_WHOLE_SIZE};
  vk::DescriptorBufferInfo idxInfo{m_indices.buffer, 0, VK_WHOLE_SIZE};
  vk::DescriptorBufferInfo mateInfo{m_materials.buffer, 0, VK_WHOLE_SIZE};
  vk::DescriptorBufferInfo mtxInfo{m_matrices.buffer, 0, VK_WHOLE_SIZE};
  vk::DescriptorBufferInfo tanInfo{m_tangents.buffer, 0, VK_WHOLE_SIZE};
  vk::DescriptorBufferInfo colInfo{m_colors.buffer, 0, VK_WHOLE_SIZE};

  std::vector<vk::WriteDescriptorSet> writes;
  writes.emplace_back(
      bind.makeWrite(m_sceneDescSet, B_ACCELERATION_STRUCTURE, &descASInfo));
  writes.emplace_back(
      bind.makeWrite(m_sceneDescSet, B_PLIM_LOOK_UP, &primInfo));
  writes.emplace_back(bind.makeWrite(m_sceneDescSet, B_VERTICES, &verInfo));
  writes.emplace_back(bind.makeWrite(m_sceneDescSet, B_NORMALS, &norInfo));
  writes.emplace_back(bind.makeWrite(m_sceneDescSet, B_TEXCOORDS, &texInfo));
  writes.emplace_back(bind.makeWrite(m_sceneDescSet, B_INDICES, &idxInfo));
  writes.emplace_back(bind.makeWrite(m_sceneDescSet, B_MATERIALS, &mateInfo));
  writes.emplace_back(bind.makeWrite(m_sceneDescSet, B_MATRICES, &mtxInfo));
  writes.emplace_back(bind.makeWrite(m_sceneDescSet, B_TANGENTS, &tanInfo));
  writes.emplace_back(bind.makeWrite(m_sceneDescSet, B_COLORS, &colInfo));

  std::vector<vk::DescriptorImageInfo> diit;
  for (auto& texture : m_textures) diit.emplace_back(texture.descriptor);
  writes.emplace_back(
      bind.makeWriteArray(m_sceneDescSet, B_TEXTURES, diit.data()));

  m_device.updateDescriptorSets(static_cast<uint32_t>(writes.size()),
                                writes.data(), 0, nullptr);
};



void Renderer::SetFrameBufferResized(bool val) {
  this->frame_buffer_resized_ = val;
}

Renderer::~Renderer() {
  vkDeviceWaitIdle(render_context_->GetNvvkContext().m_device);
  vkDestroyCommandPool(render_context_->GetNvvkContext().m_device,
                       graphics_command_pool_, nullptr);
  for (auto framebuffer : framebuffers_) {
    vkDestroyFramebuffer(render_context_->GetNvvkContext().m_device,
                         framebuffer, nullptr);
  }
  vkDestroyPipeline(render_context_->GetNvvkContext().m_device,
                    graphics_pipeline_, nullptr);
  vkDestroyPipelineLayout(render_context_->GetNvvkContext().m_device,
                          graphics_pipeline_layout_, nullptr);

  vkDestroyDescriptorSetLayout(render_context_->GetNvvkContext().m_device,
                               camera_descriptorset_layout_, nullptr);

  vkDestroyDescriptorPool(render_context_->GetNvvkContext().m_device,
                          descriptor_pool_, nullptr);

  vkDestroyRenderPass(render_context_->GetNvvkContext().m_device, render_pass_,
                      nullptr);
  // Desturctur for swap chain will be called automatically
}

void Renderer::CreateRenderPass() {
  VkAttachmentDescription color_attachment{};
  color_attachment.format         = swapchain_->getFormat();
  color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_ref = {};
  color_attachment_ref.attachment            = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments    = &color_attachment_ref;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass          = 0;
  dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo render_pass_info = {};
  render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = 1;
  render_pass_info.pAttachments    = &color_attachment;
  render_pass_info.subpassCount    = 1;
  render_pass_info.pSubpasses      = &subpass;
  render_pass_info.dependencyCount = 1;
  render_pass_info.pDependencies   = &dependency;

  if (vkCreateRenderPass(render_context_->GetNvvkContext().m_device,
                         &render_pass_info, nullptr,
                         &render_pass_) != VK_SUCCESS) {
    spdlog::error("Failed to create render pass!");
    throw std::runtime_error("Failed to create render pass");
  }
  spdlog::debug("Created render pass");
}

void Renderer::CreateGraphicsPipeline() {
  // Create shader module
  VkShaderModule vert_module = VK_NULL_HANDLE;
  VkShaderModule frag_module = VK_NULL_HANDLE;
  if (static_config::kShaderMode == 0) {
    vert_module = ShaderModule::Create(
        std::string(BUILD_DIRECTORY) + "/shaders/graphics.vert.spv",
        render_context_->GetNvvkContext().m_device);
    frag_module = ShaderModule::Create(
        std::string(BUILD_DIRECTORY) + "/shaders/graphics.frag.spv",
        render_context_->GetNvvkContext().m_device);
  } else if (static_config::kShaderMode == 1) {
    vert_module = ShaderModule::Create(
        std::string(BUILD_DIRECTORY) + "/shaders/lambert.vert.spv",
        render_context_->GetNvvkContext().m_device);
    frag_module = ShaderModule::Create(
        std::string(BUILD_DIRECTORY) + "/shaders/lambert.frag.spv",
        render_context_->GetNvvkContext().m_device);
  }
  if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE) {
    spdlog::error("Failed to create shader module!");
    throw std::runtime_error("Failed to create shader module");
  }
  spdlog::debug("Created shader module");

  // Create Pipelines
  VkPipelineShaderStageCreateInfo vert_stage_info = {};
  vert_stage_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_stage_info.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vert_stage_info.module = vert_module;
  vert_stage_info.pName  = "main";

  VkPipelineShaderStageCreateInfo frag_stage_info = {};
  frag_stage_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_stage_info.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_stage_info.module = frag_module;
  frag_stage_info.pName  = "main";

  VkPipelineShaderStageCreateInfo shader_stages[] = {vert_stage_info,
                                                     frag_stage_info};

  // Vertex Input
  VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
  vertex_input_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount   = 0;
  vertex_input_info.vertexAttributeDescriptionCount = 0;

  auto bindingDescription    = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  vertex_input_info.vertexBindingDescriptionCount = 1;
  vertex_input_info.pVertexBindingDescriptions    = &bindingDescription;
  vertex_input_info.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertex_input_info.pVertexAttributeDescriptions = attributeDescriptions.data();

  // Input Assembly
  VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
  input_assembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology               = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  // Viewports and Scissors (rectangles that define in which regions pixels are
  // stored)
  VkViewport viewport = {};
  viewport.x          = 0.0f;
  viewport.y          = 0.0f;
  viewport.width      = (float)swapchain_->getWidth();
  viewport.height     = (float)swapchain_->getHeight();
  viewport.minDepth   = 0.0f;
  viewport.maxDepth   = 1.0f;

  VkRect2D scissor = {};
  scissor.offset   = {0, 0};
  scissor.extent   = swapchain_->getExtent();

  VkPipelineViewportStateCreateInfo viewport_state = {};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.pViewports    = &viewport;
  viewport_state.scissorCount  = 1;
  viewport_state.pScissors     = &scissor;

  // Rasterizer
  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable        = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth               = 1.0f;
  rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable         = VK_FALSE;

  // Multisampling
  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable  = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  // Color blending (turned off here, but showing options for learning)
  // --> Configuration per attached framebuffer
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable         = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

  // --> Global color blending settings
  VkPipelineColorBlendStateCreateInfo color_blending = {};
  color_blending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable     = VK_FALSE;
  color_blending.logicOp           = VK_LOGIC_OP_COPY;
  color_blending.attachmentCount   = 1;
  color_blending.pAttachments      = &colorBlendAttachment;
  color_blending.blendConstants[0] = 0.0f;
  color_blending.blendConstants[1] = 0.0f;
  color_blending.blendConstants[2] = 0.0f;
  color_blending.blendConstants[3] = 0.0f;

  std::vector<VkDescriptorSetLayout> descriptor_set_layouts{
      camera_descriptorset_layout_};

  VkPipelineLayoutCreateInfo pipeline_layout_info = {};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount =
      static_cast<uint32_t>(descriptor_set_layouts.size());
  pipeline_layout_info.pSetLayouts            = descriptor_set_layouts.data();
  pipeline_layout_info.pushConstantRangeCount = 0;
  pipeline_layout_info.pPushConstantRanges    = VK_NULL_HANDLE;

  if (vkCreatePipelineLayout(render_context_->GetNvvkContext().m_device,
                             &pipeline_layout_info, nullptr,
                             &graphics_pipeline_layout_) != VK_SUCCESS) {
    spdlog::error("Failed to create pipeline layout!");
    throw std::runtime_error("Failed to create pipeline layout");
  }
  spdlog::debug("Created graphics pipeline layout");

  std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT,
                                                VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamic_info = {};
  dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
  dynamic_info.pDynamicStates    = dynamic_states.data();

  // Depth Pass
  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable       = VK_TRUE;
  depthStencil.depthWriteEnable      = VK_TRUE;
  depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds        = 0.0f;     // Optional
  depthStencil.maxDepthBounds        = 1000.0f;  // Optional
  depthStencil.stencilTestEnable     = VK_FALSE;
  depthStencil.front                 = {};  // Optional
  depthStencil.back                  = {};  // Optional

  VkGraphicsPipelineCreateInfo pipeline_info = {};
  pipeline_info.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = 2;
  pipeline_info.pStages    = shader_stages;
  pipeline_info.pVertexInputState   = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState      = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState   = &multisampling;
  pipeline_info.pColorBlendState    = &color_blending;
  pipeline_info.pDynamicState       = &dynamic_info;
  pipeline_info.layout              = graphics_pipeline_layout_;
  pipeline_info.renderPass          = render_pass_;
  pipeline_info.subpass             = 0;
  pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;
  pipeline_info.pDepthStencilState  = &depthStencil;

  if (vkCreateGraphicsPipelines(render_context_->GetNvvkContext().m_device,
                                VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                                &graphics_pipeline_) != VK_SUCCESS) {
    spdlog::error("Failed to create pipline!");
    throw std::runtime_error("Failed to create pipline");
  }
  spdlog::debug("Created graphics pipeline");

  vkDestroyShaderModule(render_context_->GetNvvkContext().m_device, frag_module,
                        nullptr);
  vkDestroyShaderModule(render_context_->GetNvvkContext().m_device, vert_module,
                        nullptr);

  spdlog::debug(
      "Destroyed unused shader module after creating graphics pipeline");
}

void Renderer::CreateFrameResources() {
  // Swap chain image view are in swap chain class now

  framebuffers_.resize(swapchain_->getImageCount());

  for (size_t i = 0; i < swapchain_->getImageCount(); i++) {
    VkImageView attachments[] = {swapchain_->getImageView(i)};

    VkFramebufferCreateInfo framebuffer_info = {};
    framebuffer_info.sType      = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = render_pass_;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments    = attachments;
    framebuffer_info.width           = swapchain_->getWidth();
    framebuffer_info.height          = swapchain_->getHeight();
    framebuffer_info.layers          = 1;

    if (vkCreateFramebuffer(render_context_->GetNvvkContext().m_device,
                            &framebuffer_info, nullptr,
                            &framebuffers_[i]) != VK_SUCCESS) {
      spdlog::error("Failed to create frame buffer {} of {}", i,
                    swapchain_->getImageCount());
      throw std::runtime_error("Failed to create frame buffer");
    }
    spdlog::debug("Created frame buffer [{}] of {}", i,
                  swapchain_->getImageCount());
  }
}

void Renderer::CreateCommandPools() {
  // Create graphics command pool
  VkCommandPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.queueFamilyIndex =
      render_context_->GetQueueFamilyIndices()[QueueFlags::GRAPHICS];

  if (vkCreateCommandPool(render_context_->GetNvvkContext().m_device,
                          &pool_info, nullptr,
                          &graphics_command_pool_) != VK_SUCCESS) {
    spdlog::error("Failed to create graphics command pool!");
    throw std::runtime_error("Failed to create graphics command pool");
  }
  spdlog::debug("Created graphics command pool");
}

void Renderer::RecordCommandBuffers() {
  command_buffers_.resize(framebuffers_.size());

  // Specify the command pool and number of buffers to allocate
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool        = graphics_command_pool_;
  allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)command_buffers_.size();

  if (vkAllocateCommandBuffers(render_context_->GetNvvkContext().m_device,
                               &allocInfo,
                               command_buffers_.data()) != VK_SUCCESS) {
    spdlog::error("Failed to allocate command buffers!");
    throw std::runtime_error("Failed to allocate command buffers");
  }

  // Start command buffer recording
  for (size_t i = 0; i < command_buffers_.size(); i++) {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    begin_info.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(command_buffers_[i], &begin_info) != VK_SUCCESS) {
      spdlog::error("Failed to begin command buffer!");
      throw std::runtime_error("Failed to begin command buffer");
    }

    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass  = render_pass_;
    render_pass_info.framebuffer = framebuffers_[i];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = swapchain_->getExtent();

    VkClearValue clearColor{{{0.0f, 0.0f, 0.0f, 1.0f}}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues    = &clearColor;

    VkViewport viewport = {};
    viewport.x          = 0.0f;
    viewport.y          = 0.0f;
    viewport.width      = (float)swapchain_->getWidth();
    viewport.height     = (float)swapchain_->getHeight();
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;

    VkRect2D scissor = {};
    scissor.offset   = {0, 0};
    scissor.extent   = swapchain_->getExtent();

    vkCmdSetViewport(command_buffers_[i], 0, 1, &viewport);
    vkCmdSetScissor(command_buffers_[i], 0, 1, &scissor);

    // Bind the camera descriptor set. This is set 0 in all pipelines so it will
    // be inherited
    vkCmdBindDescriptorSets(
        command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphics_pipeline_layout_, 0, 1, &camera_descriptorset_, 0, nullptr);

    vkCmdBeginRenderPass(command_buffers_[i], &render_pass_info,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphics_pipeline_);

    for (size_t j = 0; j < scene_->GetObjects().size(); ++j) {
      // Bind the vertex and index buffers
      VkBuffer vertexBuffers[] = {scene_->GetObjects()[j]->GetVertexBuffer()};
      VkDeviceSize offsets[]   = {0};
      vkCmdBindVertexBuffers(command_buffers_[i], 0, 1, vertexBuffers, offsets);

      /*vkCmdBindIndexBuffer(command_buffers_[i],
                           scene_->GetObjects()[j]->GetIndexBuffer(), 0,
                           VK_INDEX_TYPE_UINT32);*/

      /*vkCmdDrawIndexed(
          command_buffers_[i],
          static_cast<uint32_t>(scene_->GetObjects()[j]->GetIndices().size()),
          1, 0, 0, 0);*/

      vkCmdDraw(
          command_buffers_[i],
          static_cast<uint32_t>(scene_->GetObjects()[j]->GetVertices().size()),
          1, 0, 0);
    }

    vkCmdEndRenderPass(command_buffers_[i]);

    if (vkEndCommandBuffer(command_buffers_[i]) != VK_SUCCESS) {
      spdlog::error("Failed to record command buffer {} of {}", i,
                    command_buffers_.size());
      throw std::runtime_error("Failed to record command buffer");
    }
  }
}

void Renderer::RecreateSwapChain() {
  int width  = 0;
  int height = 0;
  do {
    glfwGetFramebufferSize(SingletonManager::GetWindow().WindowPtr(), &width,
                           &height);
    glfwWaitEvents();
  } while (width == 0 || height == 0);

  vkDeviceWaitIdle(render_context_->GetNvvkContext().m_device);

  // CleanupSwapChain();

  vkDestroyCommandPool(render_context_->GetNvvkContext().m_device,
                       graphics_command_pool_, nullptr);
  for (auto framebuffer : framebuffers_) {
    vkDestroyFramebuffer(render_context_->GetNvvkContext().m_device,
                         framebuffer, nullptr);
  }
  swapchain_->deinit();
  spdlog::debug("Destroyed old swapchain in frame");

  // Nicely done
  swapchain_->init(
      render_context_->GetNvvkContext().m_device,
      render_context_->GetNvvkContext().m_physicalDevice,
      render_context_->GetQueues()[QueueFlags::GRAPHICS],
      render_context_->GetQueueFamilyIndices()[QueueFlags::GRAPHICS],
      render_context_->Surface());

  CreateFrameResources();
  CreateCommandPools();
  RecordCommandBuffers();
}

// TODo : Deprecate !!
[[deprecated]] void Renderer::CleanupSwapChain() {
  for (int i = 0; i < static_config::kMaxFrameInFlight; i++) {
    vkDestroyFramebuffer(render_context_->GetNvvkContext().m_device,
                         framebuffers_[i], nullptr);
  }

  vkFreeCommandBuffers(
      render_context_->GetNvvkContext().m_device, graphics_command_pool_,
      static_cast<uint32_t>(command_buffers_.size()), command_buffers_.data());

  vkDestroyPipeline(render_context_->GetNvvkContext().m_device,
                    graphics_pipeline_, nullptr);
  vkDestroyPipelineLayout(render_context_->GetNvvkContext().m_device,
                          graphics_pipeline_layout_, nullptr);
  vkDestroyRenderPass(render_context_->GetNvvkContext().m_device, render_pass_,
                      nullptr);
}

void Renderer::Draw() {
  /*vkWaitForFences(render_context_->GetNvvkContext().m_device, 1,
                  &swapchain_->fences_in_flight_[current_frame_idx_], VK_TRUE,
                  UINT64_MAX);*/
  if (!swapchain_->acquire()) {
    RecreateSwapChain();
    return;
  }

  VkSubmitInfo submitInfo = {};
  submitInfo.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore wait_semaphores[]      = {swapchain_->getActiveReadSemaphore()};
  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores    = wait_semaphores;
  submitInfo.pWaitDstStageMask  = wait_stages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers =
      &command_buffers_[swapchain_->getActiveImageIndex()];

  VkSemaphore signal_semaphores[] = {swapchain_->getActiveWrittenSemaphore()};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = signal_semaphores;

  if (vkQueueSubmit(render_context_->GetQueues()[QueueFlags::GRAPHICS], 1,
                    &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
    spdlog::error("Failed to submit draw command buffer");
    throw std::runtime_error("Failed to submit draw command buffer");
  }

  swapchain_->present();

  /*VkPresentInfoKHR present_info = {};
  present_info.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores    = signal_semaphores;

  VkSwapchainKHR swapChains[] = {swapchain_->getSwapchain()};
  present_info.swapchainCount = 1;
  present_info.pSwapchains    = swapChains;

  present_info.pImageIndices = swapchain_->getActiveImageIndex();

  result = vkQueuePresentKHR(render_context_->GetQueues()[QueueFlags::PRESENT],
                             &present_info);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    RecreateSwapChain();
  } else if (result != VK_SUCCESS) {
    spdlog::error("Failed to present swapchain image");
    throw std::runtime_error("Failed to present swapchain image");
  }

  current_frame_idx_ =
      (current_frame_idx_ + 1) % static_config::kMaxFrameInFlight;*/
}

RenderContext* Renderer::RenderContextPtr() const noexcept {
  return render_context_.get();
}

void Renderer::CreateCameraDiscriptorSetLayout() {
  // Describe the binding of the descriptor set layout
  VkDescriptorSetLayoutBinding uboLayoutBinding = {};
  uboLayoutBinding.binding                      = 0;
  uboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount    = 1;
  uboLayoutBinding.stageFlags         = VK_SHADER_STAGE_ALL;
  uboLayoutBinding.pImmutableSamplers = nullptr;

  std::vector<VkDescriptorSetLayoutBinding> bindings = {uboLayoutBinding};

  // Create the descriptor set layout
  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings    = bindings.data();

  if (vkCreateDescriptorSetLayout(
          render_context_->GetNvvkContext().m_device, &layoutInfo, nullptr,
          &camera_descriptorset_layout_) != VK_SUCCESS) {
    spdlog::error("Failed to create camera descriptor set layout");
    throw std::runtime_error("Failed to create camera descriptor set layout");
  }
}

void Renderer::CreateDescriptorPool() {
  // Describe which descriptor types that the descriptor sets will contain

    uint32_t maxSets = 100;
  using vkDT       = vk::DescriptorType;
  using vkDP       = vk::DescriptorPoolSize;

  std::vector<vk::DescriptorPoolSize> staticPoolSizes = {
      vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler,
                             maxSets),
      vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, maxSets),
      vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, maxSets),
      vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic,
                             maxSets),
      vk::DescriptorPoolSize(vk::DescriptorType::eAccelerationStructureKHR,
                             maxSets),
      vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, maxSets)};
  
  vk::Device device_ = render_context_->GetNvvkContext().m_device;

  descriptor_pool_ = nvvk::createDescriptorPool(device_,staticPoolSizes, maxSets)  ;
  //TODO : OLd descriptor pool for camera
  //std::vector<VkDescriptorPoolSize> poolSizes = {
  //    // Camera
  //    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}};

  /*VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes    = poolSizes.data();
  poolInfo.maxSets       = 5;

  if (vkCreateDescriptorPool(render_context_->GetNvvkContext().m_device,
                             &poolInfo, nullptr,
                             &descriptor_pool_) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor pool");
  }*/
}

void Renderer::CreateCameraDescriptorSet() {
  // Describe the descriptor set
  VkDescriptorSetLayout layouts[]       = {camera_descriptorset_layout_};
  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool     = descriptor_pool_;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts        = layouts;

  // Allocate descriptor sets
  if (vkAllocateDescriptorSets(render_context_->GetNvvkContext().m_device,
                               &allocInfo,
                               &camera_descriptorset_) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate camera descriptor set");
  }

  // Configure the descriptors to refer to buffers
  VkDescriptorBufferInfo cameraBufferInfo = {};
  cameraBufferInfo.buffer                 = camera_->GetBuffer();
  cameraBufferInfo.offset                 = 0;
  cameraBufferInfo.range                  = sizeof(CameraBufferObject);

  std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};
  descriptorWrites[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[0].dstSet           = camera_descriptorset_;
  descriptorWrites[0].dstBinding       = 0;
  descriptorWrites[0].dstArrayElement  = 0;
  descriptorWrites[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrites[0].descriptorCount  = 1;
  descriptorWrites[0].pBufferInfo      = &cameraBufferInfo;
  descriptorWrites[0].pImageInfo       = nullptr;
  descriptorWrites[0].pTexelBufferView = nullptr;

  // Update descriptor sets
  vkUpdateDescriptorSets(render_context_->GetNvvkContext().m_device,
                         static_cast<uint32_t>(descriptorWrites.size()),
                         descriptorWrites.data(), 0, nullptr);
}

}  // namespace volume_restir
