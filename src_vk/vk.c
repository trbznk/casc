// NEXT: https://vulkan-tutorial.com/Uniform_buffers/Descriptor_layout_and_buffer

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const char* validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};
const size_t validation_layers_count = sizeof(validation_layers)/sizeof(validation_layers[0]);

#ifdef NDEBUG
    const bool enable_validation_layers = false;
#else
    const bool enable_validation_layers = true;
#endif

const char *device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
const size_t device_extensions_count = sizeof(device_extensions)/sizeof(device_extensions[0]);

typedef struct {
    VkInstance instance;
    GLFWwindow* window;

    VkPhysicalDevice physical_device;
    VkDevice device;

    VkQueue graphics_queue;
    VkQueue present_queue;

    VkSurfaceKHR surface;

    VkSwapchainKHR swap_chain;
    
    VkImage *swap_chain_images;
    uint32_t swap_chain_images_count;
    
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;

    VkImageView *swap_chain_image_views;
    uint32_t swap_chain_image_views_count;

    VkRenderPass render_pass;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    VkFramebuffer *swap_chain_framebuffers;
    uint32_t swap_chain_framebuffers_count;

    VkCommandPool command_pool;
    VkCommandBuffer *command_buffers;
    size_t command_buffers_count;

    VkSemaphore *image_available_semaphores;
    size_t image_available_semaphores_count;

    VkSemaphore *render_finished_semaphores;
    size_t render_finished_semaphores_count;

    VkFence *in_flight_fences;
    size_t in_flight_fences_count;

    uint32_t current_frame;

    bool framebuffer_resized;

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;
} VkContext;

VkContext *CONTEXT;

typedef struct {

    uint32_t graphics_family;
    uint32_t present_family;

    bool graphics_family_has_value;
    bool present_family_has_value;

} QueueFamilyIndices;

typedef struct {

    VkSurfaceCapabilitiesKHR capabilities;

    VkSurfaceFormatKHR *formats;
    uint32_t formats_count;
    
    VkPresentModeKHR *present_modes;
    uint32_t present_modes_count;

} SwapChainSupportDetails;

typedef struct {
    size_t size;
    uint8_t *bytes;
} ShaderCode;

typedef struct {
    float x, y;
} v2f;

typedef struct {
    float x, y, z;
} v3f;

typedef struct {
    v2f pos;
    v3f color;
} Vertex;

typedef struct {
    VkVertexInputAttributeDescription pos_description;
    VkVertexInputAttributeDescription color_description;
} VertexInputAttributeDescription;

// const Vertex vertices[] = {
//     {{0.0f, -0.5f}, {0.5f, 0.5f, 0.0f}},
//     {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
//     {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
// };
const Vertex vertices[] = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};
size_t vertices_count = sizeof(vertices)/sizeof(vertices[0]);
const uint16_t indices[] = {
    0, 1, 2, 2, 3, 0
};
size_t indices_count = sizeof(indices)/sizeof(indices[0]);

VkVertexInputBindingDescription vk_vertex_get_binding_description() {
    VkVertexInputBindingDescription binding_description = {0};
    binding_description.binding = 0;
    binding_description.stride = sizeof(Vertex);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return binding_description;
}

VertexInputAttributeDescription vk_vertext_get_attribute_descriptions() {
    VertexInputAttributeDescription description = {0};

    description.pos_description.binding = 0;
    description.pos_description.location = 0;
    description.pos_description.format = VK_FORMAT_R32G32_SFLOAT;
    description.pos_description.offset = offsetof(Vertex, pos);

    description.color_description.binding = 0;
    description.color_description.location = 1;
    description.color_description.format = VK_FORMAT_R32G32B32_SFLOAT;
    description.color_description.offset = offsetof(Vertex, color);

    return description;
}

bool vk_check_validation_layer_support() {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);

    VkLayerProperties available_layers[layer_count];
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

    for (size_t i = 0; i < validation_layers_count; i++) {
        bool layer_found = false;

        for (size_t j = 0; j < layer_count; j++) {
            if (strcmp(validation_layers[i], available_layers[j].layerName) == 0) {
                layer_found = true;
                break;
            }
        }

        if (!layer_found) {
            return false;
        }
    }

    return true;
}

bool vk_queue_family_indices_is_complete(QueueFamilyIndices *indices) {
    return indices->graphics_family_has_value && indices->present_family_has_value;
}

static void vk_framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    (void)width;
    (void)height;
    CONTEXT->framebuffer_resized = true;
}

void vk_init_window(VkContext *context) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    context->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", NULL, NULL);
    // glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(context->window, vk_framebuffer_resize_callback);
}

uint32_t vk_find_memory_type(VkContext *context, uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(context->physical_device, &mem_properties);

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    // failed to find suitable memory type!
    assert(false);
}

void vk_copy_buffer(VkContext *context, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = context->command_pool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(context->device, &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    VkBufferCopy copy_region = {0};
    copy_region.srcOffset = 0; // Optional
    copy_region.dstOffset = 0; // Optional
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(context->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(context->graphics_queue);

    vkFreeCommandBuffers(context->device, context->command_pool, 1, &command_buffer);
}

void vk_create_buffer(VkContext *context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *buffer_memory) {
    VkBufferCreateInfo buffer_info = {0};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(context->device, &buffer_info, NULL, buffer);
    if (result != VK_SUCCESS) {
        // failed to create buffer!
        assert(false);
    }

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(context->device, *buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = vk_find_memory_type(context, mem_requirements.memoryTypeBits, properties);

    result = vkAllocateMemory(context->device, &alloc_info, NULL, buffer_memory);
    if (result != VK_SUCCESS) {
        // failed to allocate buffer memory!
        assert(false);
    }

    vkBindBufferMemory(context->device, *buffer, *buffer_memory, 0);
}

void vk_create_vertex_buffer(VkContext *context) {
    VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices_count;

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    vk_create_buffer(
        context,
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &staging_buffer,
        &staging_buffer_memory
    );

    void* data;
    vkMapMemory(context->device, staging_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, vertices, (size_t) buffer_size);
    vkUnmapMemory(context->device, staging_buffer_memory);

    vk_create_buffer(
        context,
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &context->vertex_buffer,
        &context->vertex_buffer_memory
    );
    
    vk_copy_buffer(context, staging_buffer, context->vertex_buffer, buffer_size);

    vkDestroyBuffer(context->device, staging_buffer, NULL);
    vkFreeMemory(context->device, staging_buffer_memory, NULL);
}

void vk_create_index_buffer(VkContext *context) {
    VkDeviceSize buffer_size = sizeof(indices[0]) * indices_count;

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    vk_create_buffer(
        context,
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &staging_buffer,
        &staging_buffer_memory
    );

    void* data;
    vkMapMemory(context->device, staging_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, indices, (size_t) buffer_size);
    vkUnmapMemory(context->device, staging_buffer_memory);

    vk_create_buffer(
        context,
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &context->index_buffer,
        &context->index_buffer_memory
    );

    vk_copy_buffer(context, staging_buffer, context->index_buffer, buffer_size);

    vkDestroyBuffer(context->device, staging_buffer, NULL);
    vkFreeMemory(context->device, staging_buffer_memory, NULL);
}

void vk_create_instance(VkContext *context) {
    if (enable_validation_layers && !vk_check_validation_layer_support()) {
        // validation layers requested, but not available!
        assert(false);
    }

    // app info
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    // create info
    VkInstanceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    uint32_t glfw_extension_count = 0;
    // const char** glfw_extensions;
    const char** glfw_extensions;

    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    // printf("glfw_extension_count=%d\n", glfw_extension_count);

    // we need to add an extra extensions for macos support
    const char* required_extensions[glfw_extension_count+1];
    // const char** required_extensions = malloc((glfw_extension_count + 1) * sizeof(char*));
    for (size_t i = 0; i < glfw_extension_count; i++) {
        required_extensions[i] = glfw_extensions[i];
        // printf("required_extensions[i]=%s\n", required_extensions[i]);
    }
    required_extensions[glfw_extension_count] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    // printf("required_extensions[%d]=%s\n", glfw_extension_count, required_extensions[glfw_extension_count]);

    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    create_info.enabledExtensionCount = glfw_extension_count+1;
    create_info.ppEnabledExtensionNames = required_extensions;
    if (enable_validation_layers) {
        create_info.enabledLayerCount = validation_layers_count;
        create_info.ppEnabledLayerNames = validation_layers;
    } else {
        create_info.enabledLayerCount = 0;
    }

    // create instance
    VkResult result = vkCreateInstance(&create_info, NULL, &context->instance);
    if (result != VK_SUCCESS) {
        printf("failed to create instance with error code %d\n", result);
        assert(false);
    }
};

QueueFamilyIndices vk_find_queue_families(VkContext *context, VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    indices.graphics_family_has_value = false;
    indices.present_family_has_value = false;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

    for (size_t i = 0; i < queue_family_count; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
            indices.graphics_family_has_value = true;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, context->surface, &present_support);
        if (present_support) {
            indices.present_family = i;
            indices.present_family_has_value = true;
        }
    }
    // the queue family can differ

    return indices;
}

bool vk_check_device_extension_support(VkPhysicalDevice device) {
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);

    VkExtensionProperties available_extensions[extension_count];
    vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, available_extensions);

    uint32_t seen = 0;
    for (size_t i = 0; i < device_extensions_count; i++) {
        for (size_t j = 0; j < extension_count; j++) {
            if (!strcmp(device_extensions[i], available_extensions[j].extensionName)) {
                seen += 1;
            }
        } 
    }

    return seen == device_extensions_count;
}

VkSurfaceFormatKHR vk_choose_swap_surface_format(const VkSurfaceFormatKHR *available_formats, size_t count) {
    
    for (size_t i = 0; i < count; i++) {
        if (
            available_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        ) {
            return available_formats[i];
        }
    }

    return available_formats[0];
}

VkPresentModeKHR vk_choose_swap_present_mode(const VkPresentModeKHR *available_present_modes, size_t count) {

    for (size_t i = 0; i < count; i++) {
        if (available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_present_modes[i];
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

// move this into casc core.c @move
uint32_t clamp(uint32_t v, uint32_t low, uint32_t high) {
    if (low <= v && v <= high) {
        return v;
    } else if (v < low) {
        return low;
    } else if (high < v) {
        return high;
    }

    // unreachable
    assert(false);
    return 0;
}

VkExtent2D vk_choose_swap_extent(VkContext *context, const VkSurfaceCapabilitiesKHR *capabilities) {
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(context->window, &width, &height);

        VkExtent2D actual_extent = {
            (uint32_t)width,
            (uint32_t)height
        };

        actual_extent.width = clamp(actual_extent.width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width);
        actual_extent.height = clamp(actual_extent.height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height);

        return actual_extent;
    }
}

SwapChainSupportDetails vk_query_swap_chain_support(VkContext *context, VkPhysicalDevice device) {
    SwapChainSupportDetails details = {0};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, context->surface, &details.capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, context->surface, &details.formats_count, NULL);
    if (details.formats_count != 0) {
        details.formats = malloc(sizeof(VkSurfaceFormatKHR)*details.formats_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, context->surface, &details.formats_count, details.formats);
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(device, context->surface, &details.present_modes_count, NULL);
    if (details.present_modes_count != 0) {
        details.present_modes = malloc(sizeof(VkPresentModeKHR)*details.present_modes_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, context->surface, &details.present_modes_count, details.present_modes);
    }

    return details;
}

bool vk_is_device_suitable(VkContext *context, VkPhysicalDevice device) {
    // VkPhysicalDeviceProperties device_properties;
    // vkGetPhysicalDeviceProperties(device, &device_properties);

    // VkPhysicalDeviceFeatures device_features;
    // vkGetPhysicalDeviceFeatures(device, &device_features);

    // @skip further device checking and scoring - https://vulkan-tutorial.com/en/Drawing_a_triangle/Setup/Physical_devices_and_queue_families

    QueueFamilyIndices indices = vk_find_queue_families(context, device);

    bool extensions_supported = vk_check_device_extension_support(device);

    bool swap_chain_adequate = false;
    if (extensions_supported) {
        SwapChainSupportDetails swap_chain_support = vk_query_swap_chain_support(context, device);
        swap_chain_adequate = swap_chain_support.formats_count > 0 && swap_chain_support.present_modes_count > 0;
    }

    return vk_queue_family_indices_is_complete(&indices) && extensions_supported && swap_chain_adequate;
}

void vk_pick_physical_device(VkContext *context) {
    context->physical_device = VK_NULL_HANDLE;
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(context->instance, &device_count, NULL);

    if (device_count == 0) {
        // failed to find GPUs with Vulkan support
        assert(false);
    }

    VkPhysicalDevice devices[device_count];
    vkEnumeratePhysicalDevices(context->instance, &device_count, devices);

    for (size_t i = 0; i < device_count; i++) {
        if (vk_is_device_suitable(context, devices[i])) {
            context->physical_device = devices[i];
            break;
        }
    }

    if (context->physical_device == VK_NULL_HANDLE) {
        // failed to find a suitable GPU!
        assert(false);
    }
}

void vk_create_logical_device(VkContext *context) {
    QueueFamilyIndices indices = vk_find_queue_families(context, context->physical_device);
    
    float queue_priority = 1.0f;
    // queue family graphics family
    VkDeviceQueueCreateInfo queue_create_info_graphics_family = {0};
    queue_create_info_graphics_family.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info_graphics_family.queueFamilyIndex = indices.graphics_family;
    queue_create_info_graphics_family.queueCount = 1;
    queue_create_info_graphics_family.pQueuePriorities = &queue_priority;
    // queue family present family
    VkDeviceQueueCreateInfo queue_create_info_present_family = {0};
    queue_create_info_present_family.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info_present_family.queueFamilyIndex = indices.present_family;
    queue_create_info_present_family.queueCount = 1;
    queue_create_info_present_family.pQueuePriorities = &queue_priority;

    VkDeviceQueueCreateInfo queue_create_infos[] = {
        queue_create_info_graphics_family,
        queue_create_info_present_family
    };

    VkPhysicalDeviceFeatures device_features = {0};
    VkDeviceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos = queue_create_infos;
    create_info.queueCreateInfoCount = sizeof(queue_create_infos)/sizeof(queue_create_infos[0]);
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = device_extensions_count;
    create_info.ppEnabledExtensionNames = device_extensions;

    // belongs here and cannot be used until validation layers are not implemented - https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Logical_device_and_queues
    // create_info.enabled_extension_count = 0;
    // if (enable_validation_layers) {
    //     createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    //     createInfo.ppEnabledLayerNames = validationLayers.data();
    // } else {
    //     createInfo.enabledLayerCount = 0;
    // }

    VkResult result = vkCreateDevice(context->physical_device, &create_info, NULL, &context->device);
    if (result != VK_SUCCESS) {
        // failed to create logical device!
        assert(false);
    }

    vkGetDeviceQueue(context->device, indices.graphics_family, 0, &context->graphics_queue);
    vkGetDeviceQueue(context->device, indices.present_family, 0, &context->present_queue);
}

void vk_create_surface(VkContext *context) {
    VkResult result = glfwCreateWindowSurface(context->instance, context->window, NULL, &context->surface);
    if (result != VK_SUCCESS) {
        // failed to create window surface!
        assert(false);
    }
}

void vk_create_swap_chain(VkContext *context) {
    SwapChainSupportDetails swap_chain_support = vk_query_swap_chain_support(context, context->physical_device);

    VkSurfaceFormatKHR surface_format = vk_choose_swap_surface_format(swap_chain_support.formats, swap_chain_support.formats_count);
    VkPresentModeKHR present_mode = vk_choose_swap_present_mode(swap_chain_support.present_modes, swap_chain_support.present_modes_count);
    VkExtent2D extent = vk_choose_swap_extent(context, &swap_chain_support.capabilities);

    context->swap_chain_images_count = swap_chain_support.capabilities.minImageCount + 1;
    if (swap_chain_support.capabilities.maxImageCount > 0 && context->swap_chain_images_count > swap_chain_support.capabilities.maxImageCount) {
        context->swap_chain_images_count = swap_chain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = context->surface;
    create_info.minImageCount = context->swap_chain_images_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = vk_find_queue_families(context, context->physical_device);
    uint32_t queue_family_indices[] = {indices.graphics_family, indices.present_family};

    if (indices.graphics_family != indices.present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0; // Optional
        create_info.pQueueFamilyIndices = NULL; // Optional
    }

    create_info.preTransform = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(context->device, &create_info, NULL, &context->swap_chain);
    if (result != VK_SUCCESS) {
        // failed to create swap chain!
        assert(false);
    }

    vkGetSwapchainImagesKHR(context->device, context->swap_chain, &context->swap_chain_images_count, NULL);
    context->swap_chain_images = malloc(sizeof(VkImage)*context->swap_chain_images_count);
    vkGetSwapchainImagesKHR(context->device, context->swap_chain, &context->swap_chain_images_count, context->swap_chain_images);

    context->swap_chain_image_format = surface_format.format;
    context->swap_chain_extent = extent;
}

void vk_create_image_views(VkContext* context) {
    context->swap_chain_image_views_count = context->swap_chain_images_count;
    context->swap_chain_image_views = malloc(sizeof(VkImageView)*context->swap_chain_image_views_count);
    for (size_t i = 0; i < context->swap_chain_images_count; i++) {
        VkImageViewCreateInfo create_info = {0};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = context->swap_chain_images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = context->swap_chain_image_format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(context->device, &create_info, NULL, &context->swap_chain_image_views[i]);
        if (result != VK_SUCCESS) {
            // failed to create image views!
            assert(false);
        }
    }

}

// @move this function into core.c
ShaderCode vk_read_shader_code_file(const char* path) {
    printf("read shader code %s\n", path);
    ShaderCode code;

    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        // error opening file
        assert(false);
    }

    fseek(f, 0, SEEK_END);
    code.size = ftell(f);
    printf("code.size=%zu\n", code.size);
    rewind(f);

    code.bytes = malloc(code.size*sizeof(uint8_t));
    fread(code.bytes, sizeof(uint8_t), code.size, f);
    
    fclose(f);

    return code;
}

VkShaderModule vk_create_shader_module(VkContext *context, ShaderCode code) {
    VkShaderModuleCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size;
    // create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    create_info.pCode = (const uint32_t*)code.bytes;

    VkShaderModule shader_module;
    VkResult result = vkCreateShaderModule(context->device, &create_info, NULL, &shader_module);
    if (result != VK_SUCCESS) {
        // failed to create shader module!
        assert(false);
    }

    return shader_module;
}

void vk_create_render_pass(VkContext *context) {
    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = context->swap_chain_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {0};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(context->device, &render_pass_info, NULL, &context->render_pass);
    if (result != VK_SUCCESS) {
        // failed to create render pass!
        assert(false);
    }
}

void vk_create_graphics_pipeline(VkContext *context) {
    ShaderCode vert_shader_code = vk_read_shader_code_file("/Users/alex/repos/casc/shaders/vert.spv");
    ShaderCode frag_shader_code = vk_read_shader_code_file("/Users/alex/repos/casc/shaders/frag.spv");

    VkShaderModule vert_shader_module = vk_create_shader_module(context, vert_shader_code);
    VkShaderModule frag_shader_module = vk_create_shader_module(context, frag_shader_code);

    VkPipelineShaderStageCreateInfo vert_shader_stage_info = {0};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info = {0};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

    VkVertexInputBindingDescription binding_description = vk_vertex_get_binding_description();
    VkVertexInputAttributeDescription attribute_descriptions[] = {
        vk_vertext_get_attribute_descriptions().pos_description,
        vk_vertext_get_attribute_descriptions().color_description
    };
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.vertexAttributeDescriptionCount = 2;
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions;
    
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {0};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) context->swap_chain_extent.width;
    viewport.height = (float) context->swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {0};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = context->swap_chain_extent;

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic_state = {0};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = sizeof(dynamic_states)/sizeof(dynamic_states[0]);
    dynamic_state.pDynamicStates = dynamic_states;

    VkPipelineViewportStateCreateInfo viewport_state = {0};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = NULL; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    // example from the tutorial to do blending
    // colorBlendAttachment.blendEnable = VK_TRUE;
    // colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    // colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    // colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    // colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    // colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending = {0};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY; // Optional
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0f; // Optional
    color_blending.blendConstants[1] = 0.0f; // Optional
    color_blending.blendConstants[2] = 0.0f; // Optional
    color_blending.blendConstants[3] = 0.0f; // Optional

    VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0; // Optional
    pipeline_layout_info.pSetLayouts = NULL; // Optional
    pipeline_layout_info.pushConstantRangeCount = 0; // Optional
    pipeline_layout_info.pPushConstantRanges = NULL; // Optional

    {
        VkResult result = vkCreatePipelineLayout(context->device, &pipeline_layout_info, NULL, &context->pipeline_layout);
        if (result != VK_SUCCESS) {
            // failed to create pipeline layout!
            assert(false);
        }
    }

    VkGraphicsPipelineCreateInfo pipeline_info = {0};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = NULL; // Optional
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = context->pipeline_layout;
    pipeline_info.renderPass = context->render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipeline_info.basePipelineIndex = -1; // Optional

    {
        VkResult result = vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &context->graphics_pipeline);
        if (result != VK_SUCCESS) {
            // failed to create graphics pipeline!
            assert(false);
        }
    }

    vkDestroyShaderModule(context->device, frag_shader_module, NULL);
    vkDestroyShaderModule(context->device, vert_shader_module, NULL);
}

void vk_create_framebuffers(VkContext *context) {
    context->swap_chain_framebuffers_count = context->swap_chain_image_views_count;
    context->swap_chain_framebuffers = malloc(sizeof(VkFramebuffer)*context->swap_chain_framebuffers_count);

    for (size_t i = 0; i < context->swap_chain_image_views_count; i++) {
        VkImageView attachments[] = {
            context->swap_chain_image_views[i]
        };

        VkFramebufferCreateInfo framebuffer_info = {0};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = context->render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = context->swap_chain_extent.width;
        framebuffer_info.height = context->swap_chain_extent.height;
        framebuffer_info.layers = 1;

        VkResult result = vkCreateFramebuffer(context->device, &framebuffer_info, NULL, &context->swap_chain_framebuffers[i]);
        if (result != VK_SUCCESS) {
            // failed to create framebuffer!
            assert(false);
        }
    }
}

void vk_create_command_pool(VkContext *context) {
    QueueFamilyIndices queue_family_indices = vk_find_queue_families(context, context->physical_device);

    VkCommandPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = queue_family_indices.graphics_family;

    VkResult result = vkCreateCommandPool(context->device, &pool_info, NULL, &context->command_pool);
    if (result != VK_SUCCESS) {
        // failed to create command pool!
        assert(false);
    }
}

void vk_create_command_buffers(VkContext *context) {
    context->command_buffers_count = MAX_FRAMES_IN_FLIGHT;
    context->command_buffers = malloc(sizeof(VkCommandBuffer)*context->command_buffers_count);

    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = context->command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = context->command_buffers_count;

    VkResult result = vkAllocateCommandBuffers(context->device, &alloc_info, context->command_buffers);
    if (result != VK_SUCCESS) {
        // failed to allocate command buffers!
        assert(false);
    }
}

void vk_record_command_buffer(VkContext *context, VkCommandBuffer command_buffer, uint32_t image_index) {
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0; // Optional
    begin_info.pInheritanceInfo = NULL; // Optional

    {
        VkResult result = vkBeginCommandBuffer(command_buffer, &begin_info);
        if (result != VK_SUCCESS) {
            // failed to begin recording command buffer!
            assert(false);
        }
    }

    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    VkRenderPassBeginInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = context->render_pass;
    render_pass_info.framebuffer = context->swap_chain_framebuffers[image_index];
    render_pass_info.renderArea.offset.x = 0;
    render_pass_info.renderArea.offset.y = 0;
    render_pass_info.renderArea.extent = context->swap_chain_extent;
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->graphics_pipeline);

    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)context->swap_chain_extent.width;
    viewport.height = (float)context->swap_chain_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {0};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = context->swap_chain_extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    VkBuffer vertex_buffers[] = {context->vertex_buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

    vkCmdBindIndexBuffer(command_buffer, context->index_buffer, 0, VK_INDEX_TYPE_UINT16);

    // printf("vertices_count = %u\n", (uint32_t)vertices_count);
    // vkCmdDraw(command_buffer, (uint32_t)vertices_count, 1, 0, 0);
    vkCmdDrawIndexed(command_buffer, (uint32_t)indices_count, 1, 0, 0, 0);
    vkCmdEndRenderPass(command_buffer);

    {
        VkResult result = vkEndCommandBuffer(command_buffer);
        if (result != VK_SUCCESS) {
            // failed to record command buffer!
            assert(false);
        }
    }
}

void vk_cleanup_swap_chain(VkContext *context) {
    for (size_t i = 0; i < context->swap_chain_framebuffers_count; i++) {
        vkDestroyFramebuffer(context->device, context->swap_chain_framebuffers[i], NULL);
    }

    for (size_t i = 0; i < context->swap_chain_image_views_count; i++) {
        vkDestroyImageView(context->device, context->swap_chain_image_views[i], NULL);
    }

    vkDestroySwapchainKHR(context->device, context->swap_chain, NULL);
}

void vk_recreate_swap_chain(VkContext *context) {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(context->window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(context->window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(context->device);

    vk_cleanup_swap_chain(context);

    vk_create_swap_chain(context);
    vk_create_image_views(context);
    vk_create_framebuffers(context);
}

void vk_draw_frame(VkContext *context) {
    vkWaitForFences(context->device, 1, &context->in_flight_fences[context->current_frame], VK_TRUE, UINT64_MAX);

    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(
        context->device,
        context->swap_chain,
        UINT64_MAX,
        context->image_available_semaphores[context->current_frame],
        VK_NULL_HANDLE,
        &image_index
    );
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        vk_recreate_swap_chain(context);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        // failed to acquire swap chain image!
        assert(false);
    }

    vkResetFences(context->device, 1, &context->in_flight_fences[context->current_frame]);

    vkResetCommandBuffer(context->command_buffers[context->current_frame], 0);
    vk_record_command_buffer(context, context->command_buffers[context->current_frame], image_index);

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {context->image_available_semaphores[context->current_frame]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &context->command_buffers[context->current_frame];

    VkSemaphore signal_semaphores[] = {context->render_finished_semaphores[context->current_frame]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;


    result = vkQueueSubmit(context->graphics_queue, 1, &submit_info, context->in_flight_fences[context->current_frame]);
    if (result != VK_SUCCESS) {
        // failed to submit draw command buffer!
        assert(false);
    }

    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swap_chains[] = {context->swap_chain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = NULL; // Optional

    result = vkQueuePresentKHR(context->present_queue, &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || context->framebuffer_resized) {
        context->framebuffer_resized = false;
        vk_recreate_swap_chain(context);
    } else if (result != VK_SUCCESS) {
        // failed to present swap chain image!
        assert(false);
    }

    context->current_frame = (context->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void vk_create_sync_objects(VkContext *context) {
    context->image_available_semaphores_count = MAX_FRAMES_IN_FLIGHT;
    context->render_finished_semaphores_count = MAX_FRAMES_IN_FLIGHT;
    context->in_flight_fences_count = MAX_FRAMES_IN_FLIGHT;
    context->image_available_semaphores = malloc(sizeof(VkSemaphore)*context->image_available_semaphores_count);
    context->render_finished_semaphores = malloc(sizeof(VkSemaphore)*context->render_finished_semaphores_count);
    context->in_flight_fences = malloc(sizeof(VkFence)*context->in_flight_fences_count);

    VkSemaphoreCreateInfo semaphore_info = {0};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkResult r1 = vkCreateSemaphore(context->device, &semaphore_info, NULL, &context->image_available_semaphores[i]);
        VkResult r2 = vkCreateSemaphore(context->device, &semaphore_info, NULL, &context->render_finished_semaphores[i]);
        VkResult r3 = vkCreateFence(context->device, &fence_info, NULL, &context->in_flight_fences[i]);
        if (r1 != VK_SUCCESS || r2 != VK_SUCCESS || r3 != VK_SUCCESS) {
            // failed to create semaphores!
            assert(false);
        }
    }
}

void vk_init_vulkan(VkContext *context) {
    vk_create_instance(context);
    // @skip chapter 'Validation Layers' - https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers
    // setupDebugMessenger();
    vk_create_surface(context);
    vk_pick_physical_device(context);
    vk_create_logical_device(context);
    vk_create_swap_chain(context);
    vk_create_image_views(context);
    vk_create_render_pass(context);
    vk_create_graphics_pipeline(context);
    vk_create_framebuffers(context);
    vk_create_command_pool(context);
    vk_create_vertex_buffer(context);
    vk_create_index_buffer(context);
    vk_create_command_buffers(context);
    vk_create_sync_objects(context);
}

void vk_main_loop(VkContext *context) {
    while (!glfwWindowShouldClose(context->window)) {
        glfwPollEvents();
        vk_draw_frame(context);
    }
    vkDeviceWaitIdle(context->device);
}

void vk_cleanup(VkContext *context) {
    vk_cleanup_swap_chain(context);

    vkDestroyBuffer(context->device, context->index_buffer, NULL);
    vkFreeMemory(context->device, context->index_buffer_memory, NULL);

    vkDestroyBuffer(context->device, context->vertex_buffer, NULL);
    vkFreeMemory(context->device, context->vertex_buffer_memory, NULL);

    vkDestroyPipeline(context->device, context->graphics_pipeline, NULL);
    vkDestroyPipelineLayout(context->device, context->pipeline_layout, NULL);
    vkDestroyRenderPass(context->device, context->render_pass, NULL);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(context->device, context->image_available_semaphores[i], NULL);
        vkDestroySemaphore(context->device, context->render_finished_semaphores[i], NULL);
        vkDestroyFence(context->device, context->in_flight_fences[i], NULL);
    }

    vkDestroyCommandPool(context->device, context->command_pool, NULL);

    vkDestroyDevice(context->device, NULL);

    vkDestroySurfaceKHR(context->instance, context->surface, NULL);
    vkDestroyInstance(context->instance, NULL);

    glfwDestroyWindow(context->window);

    glfwTerminate();
}

void vk_run(VkContext *context) {
    vk_init_window(context);
    vk_init_vulkan(context);
    vk_main_loop(context);
    vk_cleanup(context);
}

int main() {
    VkContext context = {0};
    CONTEXT = &context;

    vk_run(&context);

    return 0;
}