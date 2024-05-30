// NEXT https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Image_views

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;

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
    VkFormat swap_chain_image_format;
    VkExtent2D swap_chain_extent;
} VkContext;

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

bool vk_queue_family_indices_is_complete(QueueFamilyIndices *indices) {
    return indices->graphics_family_has_value && indices->present_family_has_value;
}

void vk_init_window(VkContext *context) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    context->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", NULL, NULL);
}

void vk_create_instance(VkContext *context) {
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
    create_info.enabledLayerCount = 0;

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

// TODO: move this into casc core.c
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

    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
    if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = context->surface;
    create_info.minImageCount = image_count;
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

    vkGetSwapchainImagesKHR(context->device, context->swap_chain, &image_count, NULL);
    context->swap_chain_images = malloc(sizeof(VkImage)*image_count);
    vkGetSwapchainImagesKHR(context->device, context->swap_chain, &image_count, context->swap_chain_images);

    context->swap_chain_image_format = surface_format.format;
    context->swap_chain_extent = extent;
}

void vk_init_vulkan(VkContext *context) {
    vk_create_instance(context);
    // @skip chapter 'Validation Layers' - https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers
    // setupDebugMessenger();
    vk_create_surface(context);
    vk_pick_physical_device(context);
    vk_create_logical_device(context);
    vk_create_swap_chain(context);
}

void vk_main_loop(VkContext *context) {
    while (!glfwWindowShouldClose(context->window)) {
        glfwPollEvents();
    }
}

void vk_cleanup(VkContext *context) {
    vkDestroySwapchainKHR(context->device, context->swap_chain, NULL);
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

    vk_run(&context);

    return 0;
}