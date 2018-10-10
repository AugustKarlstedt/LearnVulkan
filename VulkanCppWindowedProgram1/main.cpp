

#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <limits>
#include <fstream>
#include <iostream>
#include <vector>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT                  messageType,
	const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
	void*                                            pUserData)
{
	std::cerr << "DEBUG: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

auto vertexShader = readFile("shaders/vert.spv");
auto fragmentShader = readFile("shaders/frag.spv");

int main()
{
	// Create an SDL window that supports Vulkan rendering.
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		std::cout << "Could not initialize SDL." << std::endl;
		return 1;
	}

	SDL_Window* window = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN);

	if (window == NULL)
	{
		std::cout << "Could not create SDL window." << std::endl;
		return 1;
	}


	// Get WSI extensions from SDL (we can add more if we like - we just can't remove these)
	unsigned extensionCount;
	if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, NULL)) 
	{
		std::cout << "Could not get the number of required instance extensions from SDL." << std::endl;
		return 1;
	}

	std::vector<const char*> instanceExtensions(extensionCount);
	if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, instanceExtensions.data())) 
	{
		std::cout << "Could not get the names of required instance extensions from SDL." << std::endl;
		return 1;
	}

	std::cout << "required instance extensions: " << std::endl;
	for (const auto& extension : instanceExtensions)
	{
		std::cout << "\t" << extension << std::endl;
	}

	auto availableInstanceExtensions = vk::enumerateInstanceExtensionProperties();
	std::cout << "available instance extensions: " << std::endl;
	for (const auto& extension : availableInstanceExtensions)
	{
		std::cout << "\t" << extension.extensionName << std::endl;
	}

#if defined(_DEBUG)
	instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif


	auto availableInstanceLayers = vk::enumerateInstanceLayerProperties();
	std::cout << "available instance layers: " << std::endl;
	for (const auto& layer : availableInstanceLayers)
	{
		std::cout << "\t" << layer.layerName << ": " << layer.description << std::endl;
	}

	// Use validation layers if this is a debug build
	std::vector<const char*> instanceLayers;
#if defined(_DEBUG)
	instanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
#endif



	vk::ApplicationInfo appInfo = vk::ApplicationInfo()
		.setPApplicationName("Vulkan C++ Windowed Program Template")
		.setApplicationVersion(1)
		.setPEngineName("LunarG SDK")
		.setEngineVersion(1)
		.setApiVersion(VK_API_VERSION_1_0);

	vk::InstanceCreateInfo instInfo = vk::InstanceCreateInfo()
		.setPApplicationInfo(&appInfo)
		.setEnabledExtensionCount(static_cast<uint32_t>(instanceExtensions.size()))
		.setPpEnabledExtensionNames(instanceExtensions.data())
		.setEnabledLayerCount(static_cast<uint32_t>(instanceLayers.size()))
		.setPpEnabledLayerNames(instanceLayers.data());

	// Create the Vulkan instance.
	vk::Instance instance;
	try 
	{
		instance = vk::createInstance(instInfo);
	}
	catch (const std::exception& e) 
	{
		std::cout << "Could not create a Vulkan instance: " << e.what() << std::endl;
		return 1;
	}

	vk::DebugUtilsMessengerCreateInfoEXT debugInfo = vk::DebugUtilsMessengerCreateInfoEXT()
		.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
		.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
		.setPfnUserCallback(debugCallback)
		.setPUserData(nullptr);

	auto debugUtilsMessenger = instance.createDebugUtilsMessengerEXT(debugInfo, nullptr, vk::DispatchLoaderDynamic(instance));

	vk::DebugUtilsMessengerCallbackDataEXT testMessage = vk::DebugUtilsMessengerCallbackDataEXT().setPMessage("Hey");
	instance.submitDebugUtilsMessageEXT(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose, vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral, testMessage, vk::DispatchLoaderDynamic(instance));
	

	//
	auto physicalDevices = instance.enumeratePhysicalDevices();
	std::cout << "available physical devices: " << std::endl;
	std::cout << "\tDeviceID\tDeviceName" << std::endl;
	for (const auto& physicalDevice : physicalDevices)
	{
		auto properties = physicalDevice.getProperties();
		std::cout << "\t" << properties.deviceID << "\t" << properties.deviceName << "\t with queue properties:" << std::endl;
		unsigned int family = 0;
		for (const auto& queueProperty : physicalDevice.getQueueFamilyProperties())
		{
			std::cout << "\tFamily " << family << " Queue Count: " << queueProperty.queueCount << std::endl;

			auto flags = queueProperty.queueFlags;
			std::cout << "\t\tCompute: " << (bool)(flags & vk::QueueFlagBits::eCompute) << std::endl;
			std::cout << "\t\tGraphics: " << (bool)(flags & vk::QueueFlagBits::eGraphics) << std::endl;
			std::cout << "\t\tProtected: " << (bool)(flags & vk::QueueFlagBits::eProtected) << std::endl;
			std::cout << "\t\tSparseBinding: " << (bool)(flags & vk::QueueFlagBits::eSparseBinding) << std::endl;
			std::cout << "\t\tTransfer: " << (bool)(flags & vk::QueueFlagBits::eTransfer) << std::endl;

			++family;
		}
	}

	// Just select the first one for now
	vk::PhysicalDevice physicalDevice = physicalDevices.at(0);

	auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
	std::cout << "available device extensions: " << std::endl;
	for (const auto& extension : availableDeviceExtensions)
	{
		std::cout << "\t" << extension.extensionName << std::endl;
	}

	std::vector<const char*> deviceExtensions;
	deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	auto availableDeviceLayers = physicalDevice.enumerateDeviceLayerProperties();
	std::cout << "available device layers: " << std::endl;
	for (const auto& layer : availableDeviceLayers)
	{
		std::cout << "\t" << layer.layerName << ": " << layer.description << std::endl;
	}
	
	float priority = 1.0f;
	vk::DeviceQueueCreateInfo deviceQueueInfo = vk::DeviceQueueCreateInfo()
		.setQueueFamilyIndex(0)
		.setQueueCount(1)
		.setPQueuePriorities(&priority);

	vk::DeviceCreateInfo deviceInfo = vk::DeviceCreateInfo()
		.setPQueueCreateInfos(&deviceQueueInfo)
		.setQueueCreateInfoCount(1)
		.setPEnabledFeatures({})
		.setEnabledExtensionCount(static_cast<uint32_t>(deviceExtensions.size()))
		.setPpEnabledExtensionNames(deviceExtensions.data())
		.setEnabledLayerCount(static_cast<uint32_t>(instanceLayers.size()))
		.setPpEnabledLayerNames(instanceLayers.data());

	vk::Device logicalDevice = physicalDevice.createDevice(deviceInfo);
	vk::Queue queue = logicalDevice.getQueue(0, 0);

	// Create a Vulkan surface for rendering
	VkSurfaceKHR c_surface;
	if (!SDL_Vulkan_CreateSurface(window, static_cast<VkInstance>(instance), &c_surface))
	{
		std::cout << "Could not create a Vulkan surface." << std::endl;
		return 1;
	}
	vk::SurfaceKHR surface = vk::SurfaceKHR(c_surface);

	vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
	std::vector<vk::SurfaceFormatKHR> surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
	std::vector<vk::PresentModeKHR> surfacePresentModes = physicalDevice.getSurfacePresentModesKHR(surface);

	vk::SurfaceFormatKHR chosenSurfaceFormat;
	chosenSurfaceFormat.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	chosenSurfaceFormat.format = vk::Format::eB8G8R8A8Unorm; //vk::Format::eB8G8R8A8Srgb;

	vk::PresentModeKHR chosenPresentMode = vk::PresentModeKHR::eMailbox;
	//vk::PresentModeKHR chosenPresentMode = vk::PresentModeKHR::eFifo;

	vk::Extent2D extent = surfaceCapabilities.currentExtent;

	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
		imageCount = surfaceCapabilities.maxImageCount;
	}

	uint32_t result = physicalDevice.getSurfaceSupportKHR(0, surface);
	std::cout << "physical device surface support? " << result << std::endl;

	vk::SwapchainCreateInfoKHR swapchainInfo = vk::SwapchainCreateInfoKHR()
		.setSurface(surface)
		.setMinImageCount(imageCount)
		.setImageFormat(chosenSurfaceFormat.format)
		.setImageColorSpace(chosenSurfaceFormat.colorSpace)
		.setImageExtent(extent)
		.setImageArrayLayers(1)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
		.setImageSharingMode(vk::SharingMode::eExclusive)
		.setQueueFamilyIndexCount(0)
		.setPQueueFamilyIndices(nullptr)
		.setPreTransform(surfaceCapabilities.currentTransform)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setPresentMode(chosenPresentMode)
		.setClipped(VK_TRUE)
		.setOldSwapchain(VK_NULL_HANDLE);

	vk::SwapchainKHR swapchain = logicalDevice.createSwapchainKHR(swapchainInfo);

	std::vector<vk::Image> swapchainImages = logicalDevice.getSwapchainImagesKHR(swapchain);
	std::vector<vk::ImageView> swapchainImageViews(swapchainImages.size());
	for (size_t i = 0; i < swapchainImageViews.size(); ++i)
	{
		vk::ImageViewCreateInfo imageViewInfo = vk::ImageViewCreateInfo()
			.setImage(swapchainImages[i])
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(chosenSurfaceFormat.format);

		imageViewInfo.components.r = vk::ComponentSwizzle::eIdentity;
		imageViewInfo.components.g = vk::ComponentSwizzle::eIdentity;
		imageViewInfo.components.b = vk::ComponentSwizzle::eIdentity;
		imageViewInfo.components.a = vk::ComponentSwizzle::eIdentity;

		imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageViewInfo.subresourceRange.baseMipLevel = 0;
		imageViewInfo.subresourceRange.levelCount = 1;
		imageViewInfo.subresourceRange.baseArrayLayer = 0;
		imageViewInfo.subresourceRange.layerCount = 1;

		vk::ImageView imageView = logicalDevice.createImageView(imageViewInfo);
		swapchainImageViews[i] = imageView;
	}

	vk::ShaderModuleCreateInfo vertexShaderCreateInfo = vk::ShaderModuleCreateInfo()
		.setCodeSize(vertexShader.size())
		.setPCode(reinterpret_cast<const uint32_t*>(vertexShader.data()));
	vk::ShaderModule vertexShaderModule = logicalDevice.createShaderModule(vertexShaderCreateInfo);

	vk::ShaderModuleCreateInfo fragmentShaderCreateInfo = vk::ShaderModuleCreateInfo()
		.setCodeSize(fragmentShader.size())
		.setPCode(reinterpret_cast<const uint32_t*>(fragmentShader.data()));
	vk::ShaderModule fragmentShaderModule = logicalDevice.createShaderModule(fragmentShaderCreateInfo);



	vk::PipelineShaderStageCreateInfo pipelineVertexShaderStageInfo = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setModule(vertexShaderModule)
		.setPName("main");

	vk::PipelineShaderStageCreateInfo pipelineFragmentShaderStageInfo = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setModule(fragmentShaderModule)
		.setPName("main");

	vk::PipelineShaderStageCreateInfo shaderStages[] = { pipelineVertexShaderStageInfo, pipelineFragmentShaderStageInfo };


	vk::PipelineVertexInputStateCreateInfo vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
		.setVertexBindingDescriptionCount(0)
		.setPVertexBindingDescriptions(nullptr)
		.setVertexAttributeDescriptionCount(0)
		.setPVertexAttributeDescriptions(nullptr);

	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo()
		.setTopology(vk::PrimitiveTopology::eTriangleList)
		.setPrimitiveRestartEnable(VK_FALSE);

	vk::Viewport viewport = vk::Viewport()
		.setX(0.0f)
		.setY(0.0f)
		.setWidth(extent.width)
		.setHeight(extent.height)
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f);

	vk::Rect2D scissor = vk::Rect2D()
		.setOffset(vk::Offset2D(0, 0))
		.setExtent(extent);

	vk::PipelineViewportStateCreateInfo viewportInfo = vk::PipelineViewportStateCreateInfo()
		.setViewportCount(1)
		.setPViewports(&viewport)
		.setScissorCount(1)
		.setPScissors(&scissor);

	vk::PipelineRasterizationStateCreateInfo rasterizerInfo = vk::PipelineRasterizationStateCreateInfo()
		.setDepthClampEnable(VK_FALSE)
		.setRasterizerDiscardEnable(VK_FALSE)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setLineWidth(1.0f)
		.setCullMode(vk::CullModeFlagBits::eBack)
		.setFrontFace(vk::FrontFace::eClockwise)
		.setDepthBiasEnable(VK_FALSE)
		.setDepthBiasConstantFactor(0.0f)
		.setDepthBiasClamp(0.0f)
		.setDepthBiasSlopeFactor(0.0f);

	vk::PipelineMultisampleStateCreateInfo multisamplingInfo = vk::PipelineMultisampleStateCreateInfo()
		.setSampleShadingEnable(VK_FALSE)
		.setRasterizationSamples(vk::SampleCountFlagBits::e1)
		.setMinSampleShading(1.0f)
		.setPSampleMask(nullptr)
		.setAlphaToCoverageEnable(VK_FALSE)
		.setAlphaToOneEnable(VK_FALSE);

	vk::PipelineColorBlendAttachmentState colorBlendAttachment = vk::PipelineColorBlendAttachmentState()
		.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
		.setBlendEnable(VK_FALSE)
		.setSrcColorBlendFactor(vk::BlendFactor::eOne)
		.setDstColorBlendFactor(vk::BlendFactor::eZero)
		.setColorBlendOp(vk::BlendOp::eAdd)
		.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
		.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
		.setAlphaBlendOp(vk::BlendOp::eAdd);

	vk::PipelineColorBlendStateCreateInfo colorBlendInfo = vk::PipelineColorBlendStateCreateInfo()
		.setLogicOpEnable(VK_FALSE)
		.setLogicOp(vk::LogicOp::eCopy)
		.setAttachmentCount(1)
		.setPAttachments(&colorBlendAttachment)
		.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
		.setSetLayoutCount(0)
		.setPSetLayouts(nullptr)
		.setPushConstantRangeCount(0)
		.setPPushConstantRanges(nullptr);

	vk::PipelineLayout pipelineLayout = logicalDevice.createPipelineLayout(pipelineLayoutInfo);

	vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
		.setFormat(chosenSurfaceFormat.format)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	vk::AttachmentReference colorAttachmentReference = vk::AttachmentReference()
		.setAttachment(0)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::SubpassDescription subpass = vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(1)
		.setPColorAttachments(&colorAttachmentReference);

	vk::SubpassDependency subpassDependency = vk::SubpassDependency()
		.setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		//.setSrcAccessMask(0)
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

	vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
		.setAttachmentCount(1)
		.setPAttachments(&colorAttachment)
		.setSubpassCount(1)
		.setPSubpasses(&subpass)
		.setDependencyCount(1)
		.setPDependencies(&subpassDependency);

	vk::RenderPass renderPass = logicalDevice.createRenderPass(renderPassInfo);

	vk::GraphicsPipelineCreateInfo graphicsPipelineInfo = vk::GraphicsPipelineCreateInfo()
		.setStageCount(2)
		.setPStages(shaderStages)
		.setPVertexInputState(&vertexInputInfo)
		.setPInputAssemblyState(&inputAssemblyInfo)
		.setPViewportState(&viewportInfo)
		.setPRasterizationState(&rasterizerInfo)
		.setPMultisampleState(&multisamplingInfo)
		.setPDepthStencilState(nullptr)
		.setPColorBlendState(&colorBlendInfo)
		.setPDynamicState(nullptr)
		.setLayout(pipelineLayout)
		.setRenderPass(renderPass)
		.setSubpass(0)
		.setBasePipelineHandle(VK_NULL_HANDLE)
		.setBasePipelineIndex(-1);

	vk::Pipeline graphicsPipeline = logicalDevice.createGraphicsPipeline(VK_NULL_HANDLE, graphicsPipelineInfo);

	std::vector<vk::Framebuffer> swapchainFramebuffers(swapchainImageViews.size());
	for (size_t i = 0; i < swapchainImages.size(); ++i) 
	{
		vk::ImageView attachments[] = { swapchainImageViews[i] };

		vk::FramebufferCreateInfo framebufferInfo = vk::FramebufferCreateInfo()
			.setRenderPass(renderPass)
			.setAttachmentCount(1)
			.setPAttachments(attachments)
			.setWidth(extent.width)
			.setHeight(extent.height)
			.setLayers(1);

		vk::Framebuffer framebuffer = logicalDevice.createFramebuffer(framebufferInfo);
		swapchainFramebuffers[i] = framebuffer;
	}

	vk::CommandPoolCreateInfo commandPoolInfo = vk::CommandPoolCreateInfo()
		.setQueueFamilyIndex(0);

	vk::CommandPool commandPool = logicalDevice.createCommandPool(commandPoolInfo);

	vk::CommandBufferAllocateInfo allocateInfo = vk::CommandBufferAllocateInfo()
		.setCommandPool(commandPool)
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandBufferCount((uint32_t)swapchainFramebuffers.size());

	std::vector<vk::CommandBuffer> commandBuffers = logicalDevice.allocateCommandBuffers(allocateInfo);

	for (size_t i = 0; i < commandBuffers.size(); ++i)
	{
		vk::CommandBufferBeginInfo beginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
			.setPInheritanceInfo(nullptr);

		commandBuffers[i].begin(beginInfo);

		vk::ClearValue cv = vk::ClearValue();
		cv.color.setFloat32({ 0.0f, 0.0f, 0.0f, 1.0f });

		vk::RenderPassBeginInfo renderPassInfo = vk::RenderPassBeginInfo()
			.setRenderPass(renderPass)
			.setFramebuffer(swapchainFramebuffers[i])
			.setClearValueCount(1)
			.setPClearValues(&cv);

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = extent;

		commandBuffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
		commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
		commandBuffers[i].draw(3, 1, 0, 0);
		commandBuffers[i].endRenderPass();
		commandBuffers[i].end();
	}


	vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo();
	vk::Semaphore imageAvailable = logicalDevice.createSemaphore(semaphoreInfo);
	vk::Semaphore renderFinished = logicalDevice.createSemaphore(semaphoreInfo);



	// Poll for user input.
	bool stillRunning = true;
	while (stillRunning) 
	{
		SDL_Event event;
		while (SDL_PollEvent(&event)) 
		{

			switch (event.type) 
			{

			case SDL_QUIT:
				stillRunning = false;
				break;

			default:
				// Do nothing.
				break;
			}
		}

		// draw

		uint32_t imageIndex;
		logicalDevice.acquireNextImageKHR(swapchain, (std::numeric_limits<uint64_t>::max)(), imageAvailable, VK_NULL_HANDLE, &imageIndex);

		vk::Semaphore waitSemaphores[] = { imageAvailable };
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		vk::Semaphore signalSemaphores[] = { renderFinished };

		vk::SubmitInfo submitInfo = vk::SubmitInfo()
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(waitSemaphores)
			.setPWaitDstStageMask(waitStages)
			.setCommandBufferCount(1)
			.setPCommandBuffers(&commandBuffers[imageIndex])
			.setSignalSemaphoreCount(1)
			.setPSignalSemaphores(signalSemaphores);

		queue.submit(submitInfo, VK_NULL_HANDLE);

		vk::SwapchainKHR swapchains[] = { swapchain };

		vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(signalSemaphores)
			.setSwapchainCount(1)
			.setPSwapchains(swapchains)
			.setPImageIndices(&imageIndex)
			.setPResults(nullptr);

		queue.presentKHR(presentInfo);
	}

	logicalDevice.waitIdle();

	// Clean up.

	logicalDevice.destroyPipeline(graphicsPipeline);

	logicalDevice.destroyPipelineLayout(pipelineLayout);

	logicalDevice.destroyRenderPass(renderPass);

	instance.destroySurfaceKHR(surface);

	logicalDevice.destroy();

	SDL_DestroyWindow(window);
	SDL_Quit();

	instance.destroyDebugUtilsMessengerEXT(debugUtilsMessenger, nullptr, vk::DispatchLoaderDynamic(instance));

	instance.destroy();

	return 0;
}
