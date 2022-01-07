//#define NDEBUG
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLM_FORCE_RADIANS
//#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <optional>
#include <set>


//---------------------------- vertex buffer -------------------------------
#include <glm/glm.hpp>
#include <array>

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        // VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex
        // VK_VERTEX_INPUT_RATE_INSTANCE : Move to the next data entry after each instance
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        // float: VK_FORMAT_R32_SFLOAT
        // vec2 : VK_FORMAT_R32G32_SFLOAT
        // vec3 : VK_FORMAT_R32G32B32_SFLOAT
        // vec4 : VK_FORMAT_R32G32B32A32_SFLOAT
        // ivec2: VK_FORMAT_R32G32_SINT, a 2 - component vector of 32 - bit signed integers
        // uvec4 : VK_FORMAT_R32G32B32A32_UINT, a 4 - component vector of 32 - bit unsigned integers
        // double : VK_FORMAT_R64_SFLOAT, a double - precision(64 - bit) float
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<float> vertices2 = {
    -0.5f, -0.5f, 1.0f, 0.0f,
    0.5f, -0.5f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.0f, 1.0f,
    -0.5f, 0.5f, 1.0f, 1.0f
};

const std::vector<float> outputIndex = {
    -0.5f, -0.5f, 1.0f, 0.0f,
    0.5f, -0.5f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.0f, 1.0f,
    0.5f, 0.5f, 0.0f, 1.0f,
    -0.5f, 0.5f, 1.0f, 1.0f,
    -0.5f, -0.5f, 1.0f, 0.0f
};

//const std::vector<float> outputIndex = {
//    0.0f,0.0f,0.0f,0.0f,
//    0.0f,0.0f,0.0f,0.0f,
//    0.0f,0.0f,0.0f,0.0f,
//    0.0f,0.0f,0.0f,0.0f,
//    0.0f,0.0f,0.0f,0.0f,
//    0.0f,0.0f,0.0f,0.0f
//};

//const std::vector<float> outputIndex = {
//	1.0f, 0.0f, 0.0f, 1.0f,
//	0.0f, 1.0f, 0.0f, 1.0f,
//	0.0f, 0.0f, 1.0f, 1.0f,
//	0.0f, 1.0f, 1.0f, 1.0f,
//    0.0f, 0.0f, 1.0f, 1.0f,
//    0.0f, 1.0f, 0.0f, 1.0f,
//};

const std::vector<int> indices = {
    0, 1, 2, 2, 3, 0
};

struct UniformBufferObject {
    glm::vec2 foo;
    alignas(16) glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

//--------------------------------------------------------------------------
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class HelloPathRenderingApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;

    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;

    // don't need to destory
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    // don't need to destory
    // Device queues are implicitly cleaned up when the device is destroyed
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    VkCommandPool commandPool;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    // if it is higher than the number of swap chain images
    // or vkAcquireNextImageKHR returns images out-of-order
    // then it's possible that we may start rendering to a swap chain image that is already in flight.
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

    bool framebufferResized = false;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    VkBuffer outputIndexBuffer;
    VkDeviceMemory outputIndexBufferMemory;
    VkBufferView outputIndexBufferView;
private:
    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloPathRenderingApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void initVulkan() {
        // to initialize the Vulkan library
        createInstance();
        // debug extension
        setupDebugMessenger();
        createSurface();
        // look for and select a graphics card
        pickPhysicalDevice();
        // logical device: describes the features we want to use
        createLogicalDevice();
        // Vulkan does not have the concept of a "default framebuffer".
        createSwapChain();
        createImageViews();
        // framebuffer attachment description
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();

        createFramebuffers();
        createCommandPool();
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        //createDepthResources();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createOutputIndexBuffer();
        createOutputIndexBufferView();
        // Descriptor sets can't be created directly, 
        // they must be allocated from a pool like command buffers
        createDescriptorPool();
        createDescriptorSets();

        createCommandBuffers();
        createSyncObjects();
    }

    //void createOutputIndexBuffer() {
    //    VkDeviceSize bufferSize = sizeof(outputIndex[0]) * outputIndex.size();
    //    VkBuffer stagingBuffer;
    //    VkDeviceMemory stagingBufferMemory;
    //    createBuffer(bufferSize
    //        , VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    //        , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    //        , stagingBuffer, stagingBufferMemory);


    //    void* data = nullptr;
    //    // 3 offset (bytes)
    //    // 4 size (bytes)
    //    // 5 flags
    //    // 6 ppdata
    //    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    //    // driver may **not immediately copy the data** into the buffer memory, for example because of caching
    //    // It is also possible that writes to the buffer are not visible in the mapped memory yet
    //    // how to solve: 
    //    // 1. Use a memory heap that is host coherent, indicated with `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`
    //    // 2. Call vkFlushMappedMemoryRanges after writing to the mapped memory, 
    //    //      and call vkInvalidateMappedMemoryRanges before reading from the mapped memory
    //    // in performance, method 1 < method 2
    //    memcpy(data, (void*)outputIndex.data(), (size_t)bufferSize);
    //    vkUnmapMemory(device, stagingBufferMemory);

    //    createBuffer(bufferSize
    //        , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT
    //        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    //        , outputIndexBuffer
    //        , outputIndexBufferMemory);

    //    copyBuffer(stagingBuffer, outputIndexBuffer, bufferSize);

    //    vkDestroyBuffer(device, stagingBuffer, nullptr);
    //    vkFreeMemory(device, stagingBufferMemory, nullptr);
    //}

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            // 1. 从swap chain 获取一张image
            // 2. 将这个image作为framebuffer的附件，并执行command
            // 3. 返回image给swap chain用于展示
            drawFrame();
        }
        printf("------------------------------------------------------------------------\n");
        printf("window close!\n");
        printf("------------------------------------------------------------------------\n");
        // 等待command queue中的操作执行完毕，再进行cleanup
        vkDeviceWaitIdle(device);
    }

    void cleanup() {

        cleanupSwapChain();

        vkDestroySampler(device, textureSampler, nullptr);
        vkDestroyImageView(device, textureImageView, nullptr);
        vkDestroyImage(device, textureImage, nullptr);
        vkFreeMemory(device, textureImageMemory, nullptr);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        //----------------------clean up buffer--------------------------------------
        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        //----------------------clean up commandPool----------------------------
        vkDestroyCommandPool(device, commandPool, nullptr);

        //----------------------clean up vulkan logical device-----------------------
        vkDestroyDevice(device, nullptr);

        //----------------------clean up vulkan validation---------------------------
        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
        //----------------------clean up vulkan surface------------------------------
        vkDestroySurfaceKHR(instance, surface, nullptr);
        //----------------------clean up vulkan instance-----------------------------
        vkDestroyInstance(instance, nullptr);

        //----------------------clean up glfw----------------------------------------
        glfwDestroyWindow(window);

        glfwTerminate();
    }

    void cleanupSwapChain() {
        //----------------------clean up framebuffers----------------------------
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

        //----------------------clean up pipeline----------------------------
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        //----------------------clean up vulkan imageview----------------------------
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        //----------------------clean up vulkan swapchain----------------------------
        vkDestroySwapchainKHR(device, swapChain, nullptr);

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }

    void recreateSwapChain() {
        //----------------------Handling minimization----------------------------
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }
        //-----------------------------------------------------------------------

        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();

        createCommandBuffers();

        imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);
    }

private:
    // connection between your application and the Vulkan library 
    // and creating it involves specifying some details about your application to the driver
    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        //------------------------------VkApplicationInfo-----------------------------------
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        //------------------------------VkInstanceCreateInfo--------------------------------
        VkInstanceCreateInfo createInfo{};

        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        //---------------VkInstanceCreateInfo >>> VkDebugUtilsMessengerCreateInfoEXT---------
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            // point to extension information in the future
            createInfo.pNext = &debugCreateInfo;
        }
        else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        // Pointer to struct with **creation info**
        // Pointer to custom **allocator callbacks**, always nullptr in this tutorial
        // Pointer to the variable that stores the handle to the new object
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        // Verbose: diagnostic message
        // Info: Informational message like the creation of a resource
        // Warning: Message about behavior that is not necessarily an error, but very likely a bug in your application
        // Error: Message about behavior that is invalid and may cause **crashes**
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        // General: Some event has happened that is unrelated to the specification or performance
        // Validation: Something has happened that **violates the specification or indicates a possible mistake**
        // Performance: Potential non-optimal use of Vulkan
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;

        // Optional
        // you could point to HelloRectangleApplication class
        createInfo.pUserData = nullptr;
    }

    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        // requested number of global layer properties
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                // FOR DEBUG
                VkPhysicalDeviceProperties deviceProperties;
                vkGetPhysicalDeviceProperties(device, &deviceProperties);
                //// 使用独显
                //if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                //    physicalDevice = device;
                //    break;
                //}
                VkPhysicalDeviceFeatures deviceFeatures;
                vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        // describes the number of queues we want for a queue families
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        // between 0.0 and 1.0
        // influence the scheduling of command buffer execution
        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // don't need anything special
        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = nullptr;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        // if non-existent extensions or specifying the desired usage of unsupported features
        // will not return `VK_SUCCESS`
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        // queue index less than queueCount
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }
    void createSwapChain() {

        // There are basically three kinds of properties we need to check:
        // - Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
        // - Surface formats (pixel format, color space)
        // - Available presentation modes
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        // Surface format (color depth)
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        // Presentation mode (conditions for "swapping" images to the screen)
        // `VK_PRESENT_MODE_IMMEDIATE_KHR`: Images submitted by your application are transferred to the screen right away, which may result in tearing.
        // `VK_PRESENT_MODE_FIFO_KHR`: swap chain被看成一个队列，display刷新时从队列取第一张图片。如果队列已满，program必须等待队列被消费。
        // `VK_PRESENT_MODE_FIFO_RELAXED_KHR`: 当队列为空时，如果生成了新图像，会立即被传送到显示器缓存中。而不是等待下一个vertical blank信号
        // `VK_PRESENT_MODE_MAILBOX_KHR`: 三重缓冲
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        // Swap extent (resolution of images in swap chain)
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            // 另一个queue family使用image时，需要显示地转移所有权
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            //VkImageViewCreateInfo createInfo{};
            //createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            //createInfo.image = swapChainImages[i];
            //createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            //createInfo.format = swapChainImageFormat;
            //createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            //createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            //createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            //createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            //createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            //createInfo.subresourceRange.baseMipLevel = 0;
            //createInfo.subresourceRange.levelCount = 1;
            //createInfo.subresourceRange.baseArrayLayer = 0;
            //createInfo.subresourceRange.layerCount = 1;

            //if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            //    throw std::runtime_error("failed to create image views!");
            //}

            swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        // what to do with the data in the attachment before rendering
        // `VK_ATTACHMENT_LOAD_OP_LOAD`: 保留已存在的内容
        // `VK_ATTACHMENT_LOAD_OP_CLEAR`: 在开始时将值清除为常数, 由VkClearValue定义
        // `VK_ATTACHMENT_LOAD_OP_DONT_CARE`: Existing contents are undefined; we don't care about them
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // after rendering
        // `VK_ATTACHMENT_STORE_OP_STORE`: 渲染内容会被存储在内容中，可以稍后再读取
        // `VK_ATTACHMENT_STORE_OP_DONT_CARE`: Contents of the framebuffer will be undefined after the rendering operation
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        // `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`: Images used as color attachment
        // `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR` : Images to be presented in the swap chain
        // `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` : Images 被看作为一块内存区域
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        //VkAttachmentDescription depthAttachment{};
        //depthAttachment.format = findDepthFormat();
        //depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        //depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        //depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        //depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        //depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        //depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // how to write color
        VkAttachmentReference colorAttachmentRef{};
        // layout(location = 0) out vec4 outColor
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        //VkAttachmentReference depthAttachmentRef{};
        //depthAttachmentRef.attachment = 1;
        //depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        // pInputAttachments: Attachments that are read from a shader
        // pResolveAttachments : Attachments used for multisampling color attachments
        // pDepthStencilAttachment : Attachment for depthand stencil data
        // pPreserveAttachments : Attachments that are not used by this subpass, but for which the data must be preserved
        //subpass.pDepthStencilAttachment = &depthAttachmentRef;


        VkSubpassDependency dependency{};
        // The dstSubpass must always be higher than srcSubpass to prevent cycles in the dependency graph
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 1> attachments = {
            colorAttachment,
            //depthAttachment 
        };

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        // only relevant for image sampling related descriptors
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding outputIndexLayoutBinding{};
        outputIndexLayoutBinding.binding = 2;
        outputIndexLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        outputIndexLayoutBinding.descriptorCount = 1;
        outputIndexLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        outputIndexLayoutBinding.pImmutableSamplers = nullptr;

        //---------------------------create descriptorSetLayout--------------------------------
        std::array<VkDescriptorSetLayoutBinding, 3> bindings = { uboLayoutBinding, samplerLayoutBinding, outputIndexLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";
        vertShaderStageInfo.pSpecializationInfo = nullptr;

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        auto bindingDesc = Vertex::getBindingDescription();
        auto attriDesc = Vertex::getAttributeDescriptions();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        //vertexInputInfo.vertexBindingDescriptionCount = 0;
        //vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = attriDesc.size();
        vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
        vertexInputInfo.pVertexAttributeDescriptions = attriDesc.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        // VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
        // VK_PRIMITIVE_TOPOLOGY_LINE_LIST : line from every 2 vertices without reuse
        // VK_PRIMITIVE_TOPOLOGY_LINE_STRIP : the end vertex of every line is used as start vertex for the next line
        // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : triangle from every 3 vertices without reuse
        // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : the second and third vertex of every triangle are used as first two vertices of the next triangle
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;


        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        // VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
        // VK_POLYGON_MODE_LINE : polygon edges are drawn as lines
        // VK_POLYGON_MODE_POINT : polygon vertices are drawn as points
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        //rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        //rasterizer.depthBiasClamp = 0.0f; // Optional
        //rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        //multisampling.minSampleShading = 1.0f; // Optional
        //multisampling.pSampleMask = nullptr; // Optional
        //multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        //multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // After a fragment shader has returned a color
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        //colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        //colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        //colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        //colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        //colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        //colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        // 如果想在不销毁pipeline的情况下更改viewport
        //VkDynamicState dynamicStates[] = {
        //    VK_DYNAMIC_STATE_VIEWPORT,
        //    VK_DYNAMIC_STATE_LINE_WIDTH
        //};
        //VkPipelineDynamicStateCreateInfo dynamicState{};
        //dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        //dynamicState.dynamicStateCount = 2;
        //dynamicState.pDynamicStates = dynamicStates;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        // ---------------------clean up shader-------------------------------
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void createDepthResources() {
        VkFormat depthFormat = findDepthFormat();

        createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
        depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }

    VkBufferView createBufferView(VkBuffer buffer, VkFormat format, VkDeviceSize range) {
        VkBufferViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        viewInfo.buffer = buffer;
        viewInfo.format = format;
        viewInfo.range = range;
        viewInfo.flags = 0;

        VkBufferView bufferView;
        if (vkCreateBufferView(device, &viewInfo, nullptr, &bufferView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer view!");
        }
        return bufferView;
    }

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            std::array<VkImageView, 1> attachments = {
                swapChainImageViews[i],
                //depthImageView
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        // `VK_COMMAND_POOL_CREATE_TRANSIENT_BIT`: Hint that command buffers are rerecorded with new commands very often(may change memory allocation behavior)
        // `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`: Allow command buffers to be rerecorded individually, without this flag they all have to be reset together
        poolInfo.flags = 0; // Optional

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    //void createVertexBuffer() {
    //    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    //    createBuffer(bufferSize
    //        , VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    //        , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    //        , vertexBuffer, vertexBufferMemory);

    //    void* data = nullptr;
    //    // 3 offset (bytes)
    //    // 4 size (bytes)
    //    // 5 flags
    //    // 6 ppdata
    //    vkMapMemory(device, vertexBufferMemory, 0, bufferSize, 0, &data);
    //    // driver may **not immediately copy the data** into the buffer memory, for example because of caching
    //    // It is also possible that writes to the buffer are not visible in the mapped memory yet
    //    // how to solve: 
    //    // 1. Use a memory heap that is host coherent, indicated with `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`
    //    // 2. Call vkFlushMappedMemoryRanges after writing to the mapped memory, 
    //    //      and call vkInvalidateMappedMemoryRanges before reading from the mapped memory
    //    // in performance, method 1 < method 2
    //    memcpy(data, (void*)vertices.data(), (size_t)bufferSize);
    //    vkUnmapMemory(device, vertexBufferMemory);
    //}

    void createTextureImage() {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);

        stbi_image_free(pixels);

        createImage(texWidth
            , texHeight
            , VK_FORMAT_R8G8B8A8_SRGB
            , VK_IMAGE_TILING_OPTIMAL
            , VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
            , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            , textureImage
            , textureImageMemory);

        transitionImageLayout(textureImage
            , VK_FORMAT_R8G8B8A8_SRGB
            , VK_IMAGE_LAYOUT_UNDEFINED
            , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer
            , textureImage
            , static_cast<uint32_t>(texWidth)
            , static_cast<uint32_t>(texHeight));

        transitionImageLayout(textureImage
            , VK_FORMAT_R8G8B8A8_SRGB
            , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createTextureImageView() {
        /*VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = textureImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }*/

        textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

    }

    void createTextureSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }

    }

    void createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize
            , VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            , stagingBuffer, stagingBufferMemory);


        void* data = nullptr;
        // 3 offset (bytes)
        // 4 size (bytes)
        // 5 flags
        // 6 ppdata
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        // driver may **not immediately copy the data** into the buffer memory, for example because of caching
        // It is also possible that writes to the buffer are not visible in the mapped memory yet
        // how to solve: 
        // 1. Use a memory heap that is host coherent, indicated with `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`
        // 2. Call vkFlushMappedMemoryRanges after writing to the mapped memory, 
        //      and call vkInvalidateMappedMemoryRanges before reading from the mapped memory
        // in performance, method 1 < method 2
        memcpy(data, (void*)vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize
            , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
            , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            , vertexBuffer
            , vertexBufferMemory);

        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize
            , VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            , stagingBuffer, stagingBufferMemory);


        void* data = nullptr;
        // 3 offset (bytes)
        // 4 size (bytes)
        // 5 flags
        // 6 ppdata
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        // driver may **not immediately copy the data** into the buffer memory, for example because of caching
        // It is also possible that writes to the buffer are not visible in the mapped memory yet
        // how to solve: 
        // 1. Use a memory heap that is host coherent, indicated with `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`
        // 2. Call vkFlushMappedMemoryRanges after writing to the mapped memory, 
        //      and call vkInvalidateMappedMemoryRanges before reading from the mapped memory
        // in performance, method 1 < method 2
        memcpy(data, (void*)indices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize
            , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
            , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            , indexBuffer
            , indexBufferMemory);

        copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }
    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(swapChainImages.size());
        uniformBuffersMemory.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            createBuffer(bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                uniformBuffers[i],
                uniformBuffersMemory[i]);
        }
    }

    void createOutputIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(outputIndex[0]) * outputIndex.size();
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize
            , VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            , stagingBuffer, stagingBufferMemory);

        void* data = nullptr;
        // 3 offset (bytes)
        // 4 size (bytes)
        // 5 flags
        // 6 ppdata
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        // driver may **not immediately copy the data** into the buffer memory, for example because of caching
        // It is also possible that writes to the buffer are not visible in the mapped memory yet
        // how to solve: 
        // 1. Use a memory heap that is host coherent, indicated with `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`
        // 2. Call vkFlushMappedMemoryRanges after writing to the mapped memory, 
        //      and call vkInvalidateMappedMemoryRanges before reading from the mapped memory
        // in performance, method 1 < method 2
        memcpy(data, (void*)outputIndex.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize
            , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            , outputIndexBuffer
            , outputIndexBufferMemory);

        copyBuffer(stagingBuffer, outputIndexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createOutputIndexBufferView() {
        outputIndexBufferView = createBufferView(outputIndexBuffer, VK_FORMAT_R32G32B32A32_SFLOAT, VK_WHOLE_SIZE);
    }

    void createDescriptorPool() {
        //VkDescriptorPoolSize poolSize{};
        //poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        //poolSize.descriptorCount = static_cast<uint32_t>(swapChainImages.size());
        std::array<VkDescriptorPoolSize, 3> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        poolSizes[2].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }

    }

    void createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(swapChainImages.size());
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

            VkDescriptorBufferInfo outputBufferInfo{};
            outputBufferInfo.buffer = outputIndexBuffer;
            outputBufferInfo.offset = 0;
            outputBufferInfo.range = VK_WHOLE_SIZE;

            std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = descriptorSets[i];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pBufferInfo = &outputBufferInfo;
            descriptorWrites[2].pTexelBufferView = &outputIndexBufferView;

            //VkWriteDescriptorSet descriptorWrite{};
            //descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            //descriptorWrite.dstSet = descriptorSets[i];
            //descriptorWrite.dstBinding = 0;
            //descriptorWrite.dstArrayElement = 0;
            //descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            //descriptorWrite.descriptorCount = 1;
            //descriptorWrite.pBufferInfo = &bufferInfo;
            //descriptorWrite.pImageInfo = nullptr; // Optional
            //descriptorWrite.pTexelBufferView = nullptr; // Optional

            vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        }


    }

    void createCommandBuffers() {
        commandBuffers.resize(swapChainFramebuffers.size());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        // `VK_COMMAND_BUFFER_LEVEL_PRIMARY`: Can be submitted to a queue for execution, but cannot be called from other command buffers.
        // `VK_COMMAND_BUFFER_LEVEL_SECONDARY`: Cannot be submitted directly, but can be called from primary command buffers.
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        for (size_t i = 0; i < commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            // how we're going to use the command buffer.
            // `VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT`: The command buffer will be rerecorded right after executing it once.
            // `VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT`: This is a secondary command buffer that will be entirely within a single render pass.
            // `VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT`: The command buffer can be resubmitted while it is also already pending execution.
            //beginInfo.flags = 0; // Optional
            //beginInfo.pInheritanceInfo = nullptr; // Optional

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = swapChainFramebuffers[i];
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = swapChainExtent;

            VkClearValue clearColor = { {{1.0f, 1.0f, 1.0f, 1.0f}} };
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            // how the drawing commands within the render pass will be provided
            // `VK_SUBPASS_CONTENTS_INLINE`: The render pass commands will be embedded in the primary command buffer itselfand no secondary command buffers will be executed.
            // `VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS`: The render pass commands will be executed from secondary command buffers.
            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            VkBuffer vertexBuffers[] = { vertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);
            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

            //vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);
            //vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
            vkCmdDraw(commandBuffers[i], 6, 1, 0, 0);

            // commandBuffer
            // vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
            // instanceCount : Used for instanced rendering, use 1 if you're not doing that.
            // firstVertex : Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
            // firstInstance : Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
            //vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

            vkCmdEndRenderPass(commandBuffers[i]);

            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
    }

    void createSyncObjects() {
        // semaphonre for GPU-GPU synchronized
        // fence for CPU-GPU synchronized
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        // default: UNSIGNALED
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    void drawFrame() {
        // until inFilghtFences[currentFrame] is signaled.
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        // Acquire an image from the swap chain
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        // `VK_ERROR_OUT_OF_DATE_KHR`: The swap chain has become incompatible with the surface 
        //      and can no longer be used for rendering. Usually happens after a window resize.
        // `VK_SUBOPTIMAL_KHR`: The swap chain can still be used to successfully present to the surface
        //      , but the surface properties are no longer matched exactly.
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        // Check if a previous frame is using this image (i.e. there is its fence to wait on)
        if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }

        // Mark the image as now being in use by this frame
        imagesInFlight[imageIndex] = inFlightFences[currentFrame];

        updateUniformBuffer(imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        // (async) Execute the command buffer with that image as attachment in the framebuffer
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
        //if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        //    throw std::runtime_error("failed to submit draw command buffer!");
        //}

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        // It's not necessary if you're only using a single swap chain
        presentInfo.pResults = nullptr; // Optional

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        // be likely not optimally using the GPU in this way
        // because the whole graphics pipeline is only used for one frame at a time right now
        // vkQueueWaitIdle(presentQueue);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

private:


    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        // memoryTypes
        // memoryHeaps
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        // 逐个比特复制的操作
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        }
        else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        // some details about the queue family, 
        // including the type of operations that are supported 
        // and the number of queues that can be created based on that family. 
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    std::vector<const char*> getRequiredExtensions() {
        int supported = glfwVulkanSupported();
        if (supported == GLFW_TRUE) {
            // do nothing
        }
        else {
            std::cout << "vulkan is minimally available\n";
        }

        // If **successful**,the list will always contain `VK_KHR_surface`
        // If Vulkan is **not available** on the machine, this function returns `NULL`.
        // Call glfwVulkanSupported to check whether Vulkan is at least minimally available.
        // you can see https://www.glfw.org/docs/latest/group__vulkan.html#ga99ad342d82f4a3421e2864978cb6d1d6
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        // macOS: GLFW currently supports both the `VK_MVK_macos_surface` and the newer `VK_EXT_metal_surface` extensions.
        // windows: VK_KHR_win32_surface
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // if you require DEBUG.
        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    VkFormat findDepthFormat() {
        return findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    void createBuffer(VkDeviceSize size
        , VkBufferUsageFlags usage
        , VkMemoryPropertyFlags properties
        , VkBuffer& buffer
        , VkDeviceMemory& bufferMemory)
    {

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        // bytes size
        bufferInfo.size = size;
        // indicates for which purposes the data in the buffer is going to be used
        bufferInfo.usage = usage;
        // buffer will only be used from **graphics** queue
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.flags = 0;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        // size: 内存大小 (bytes), may differ from bufferInfo.size.
        // alignment : 缓冲区在已分配的内存区域中开始的bytes offset, depends on bufferInfo.usageand bufferInfo.flags.
        // memoryTypeBits : Bit field of the memory types that are suitable for the buffer.
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        // memoryOffset: 因为这个内存是专门为这个顶点缓冲区分配的，所以偏移量就是0。
        //      如果偏移量非零，那么它必须能被memRequirements.alignment整除
        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        //VkCommandBufferAllocateInfo allocInfo{};
        //allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        //allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        //allocInfo.commandPool = commandPool;
        //allocInfo.commandBufferCount = 1;

        //VkCommandBuffer commandBuffer;
        //vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        //VkCommandBufferBeginInfo beginInfo{};
        //beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        //// only use the command buffer once
        //beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        //vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        {
            VkBufferCopy copyRegion{};
            copyRegion.size = size;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        }
        endSingleTimeCommands(commandBuffer);
        //vkEndCommandBuffer(commandBuffer);

        //VkSubmitInfo submitInfo{};
        //submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        //submitInfo.commandBufferCount = 1;
        //submitInfo.pCommandBuffers = &commandBuffer;

        //vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        //// wait for the transfer queue to become idle
        //vkQueueWaitIdle(graphicsQueue);

        //vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }
    void createImage(uint32_t width
        , uint32_t height
        , VkFormat format
        , VkImageTiling tiling
        , VkImageUsageFlags usage
        , VkMemoryPropertyFlags properties
        , VkImage& image
        , VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        // same format for the texels as the pixels in the buffer, 
        // otherwise the copy operation will fail.
        imageInfo.format = format;
        // `VK_IMAGE_TILING_LINEAR`: Texels are laid out in row-major order like our pixels array
        // `VK_IMAGE_TILING_OPTIMAL`: Texels are laid out in an implementation defined order for optimal access
        imageInfo.tiling = tiling;
        // `VK_IMAGE_LAYOUT_UNDEFINED`: Not usable by the GPU and the very first transition will discard the texels.
        // `VK_IMAGE_LAYOUT_PREINITIALIZED`: Not usable by the GPU, but the first transition will preserve the texels.
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        // The samples flag is related to multisampling. 
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        // related to sparse images
        imageInfo.flags = 0; // Optional

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
        // in Vulakn, the origin point is on left-top and y-axis direction is from top to bottom.
        ubo.proj[1][1] *= -1;
        glm::mat4 mvp = ubo.proj * ubo.view * ubo.model;

        //rasterizer->updateTransform(mvp);
        //void* outputIndices = nullptr;
        //vkMapMemory(device, outputIndexBufferMemory, 0, 6 * sizeof(float4), 0, &outputIndices);
        //rasterizer->genOutputIndices(device, outputIndexBufferMemory, 6);
        //vkUnmapMemory(device, outputIndexBufferMemory);

        void* data;
        vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(device, uniformBuffersMemory[currentImage]);
    }

    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        // It is possible to use VK_IMAGE_LAYOUT_UNDEFINED as oldLayout 
        // if you don't care about the existing contents of the image.
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0; // TODO
        barrier.dstAccessMask = 0; // TODO

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            //0 /* TODO */, 0 /* TODO */,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        endSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = {
            width,
            height,
            1
        };
        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );
        endSingleTimeCommands(commandBuffer);
    }

private:
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

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity
        , VkDebugUtilsMessageTypeFlagsEXT messageType
        , const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
        , void* pUserData) {
        // pCallbackData->pMessage; The debug message as a null-terminated string
        // pCallbackData->pObjects; Array of Vulkan object handles related to the message
        // pCallbackData->objectCount; Number of objects in array
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        // If the callback returns true, then the call is aborted with the VK_ERROR_VALIDATION_FAILED_EXT error.
        // so you should always return `VK_FALSE`
        return VK_FALSE;
    }
};

int main() {
    HelloPathRenderingApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}