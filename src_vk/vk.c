// NEXT https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Swap_chain

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

const uint32_t WINDOW_WIDTH = 800;
const uint32_t WINDOW_HEIGHT = 600;

typedef struct {
    VkInstance instance;
    GLFWwindow* window;

    VkPhysicalDevice physical_device;
    VkDevice device;

    VkQueue graphics_queue;
    VkQueue present_queue;

    VkSurfaceKHR surface;
} VkContext;

typedef struct {
    uint32_t graphics_family;
    uint32_t present_family;

    bool graphics_family_has_value;
    bool present_family_has_value;
} QueueFamilyIndices;

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

bool vk_is_device_suitable(VkContext *context, VkPhysicalDevice device) {
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);

    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(device, &device_features);

    // @skip further device checking and scoring - https://vulkan-tutorial.com/en/Drawing_a_triangle/Setup/Physical_devices_and_queue_families

    QueueFamilyIndices indices = vk_find_queue_families(context, device);
    return indices.graphics_family_has_value && indices.present_family_has_value;
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

void vk_init_vulkan(VkContext *context) {
    vk_create_instance(context);
    // @skip chapter 'Validation Layers' - https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers
    // setupDebugMessenger();
    vk_create_surface(context);
    vk_pick_physical_device(context);
    vk_create_logical_device(context);
}

void vk_main_loop(VkContext *context) {
    while (!glfwWindowShouldClose(context->window)) {
        glfwPollEvents();
    }
}

void vk_cleanup(VkContext *context) {
    vkDestroySurfaceKHR(context->instance, context->surface, NULL);

    vkDestroyDevice(context->device, NULL);
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