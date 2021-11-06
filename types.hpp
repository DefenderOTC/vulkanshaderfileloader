#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vector>

// queue family info struct

//us int types
typedef     uint8_t     u8;
typedef     uint16_t    u16;
typedef     uint32_t    u32;
typedef     uint64_t    u64;
//sig int types
typedef     int8_t     i8;
typedef     int16_t    i16;
typedef     int32_t    i32;
typedef     int64_t    i64;   

//float
typedef     float       f32;
typedef     double      f64;

//b and c 
typedef     int32_t     b32;
typedef     int8_t      b8;

//wide char
typedef     wchar_t     wchar;

struct vulkeng_queue_family_indexes
{
    i32     graphicsqueuefamily;
    i32     presentationqueuefamily;
};
struct vulkeng_device
{
    VkPhysicalDevice physicaldevice;
    VkDevice         logicaldevice;
    VkQueue          graphicsqueue;
    VkQueue          presentationqueue;
};
struct swapchaindetails 
{
    VkSurfaceCapabilitiesKHR surfacecapabilities;
    VkSurfaceFormatKHR*       surfaceformats;
    VkPresentModeKHR*         presentmodes;
    u32     presentmodecount;
    u32     surfaceformatcount;
};

struct swapchainimage
{
    VkImage image;
    VkImageView imageview;
};

struct vertex
{
    glm::vec3 pos;
    glm::vec3 col;
};
struct transfer_q
{
    VkQueue transfer_queue;
    VkCommandPool transfer_pool;
};
struct mesh
{
    u32 mesh_vertex_count;
    VkPhysicalDevice mesh_physical_device;
    VkDevice    mesh_logical_device;
    VkBuffer    mesh_vertex_buffer;
    VkDeviceMemory mesh_device_memory;
    u32 indexcount; 
    VkBuffer indexbuffer;
    VkDeviceMemory indexbuffermemory;
};



u32     vulkeng_find_memory_type_index(u32 allowedtypes, VkMemoryPropertyFlags properties, VkPhysicalDevice physdevice)
{
    VkPhysicalDeviceMemoryProperties pdmp = {};
    vkGetPhysicalDeviceMemoryProperties(physdevice, &pdmp);

    for(u32 i = 0; i < pdmp.memoryTypeCount; i++)
    {
        if((allowedtypes & (1 << i)) 
        && (pdmp.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    return 0xFFFFFFFF;
}
static void vulkeng_copy_buffer(VkDevice logicaldevice,  VkQueue transferqueue, VkCommandPool transferpool, 
                                VkBuffer srcbuffer, VkBuffer dstbuffer, VkDeviceSize buffersize)
{
    VkCommandBuffer transfercommandbuffer;
    VkCommandBufferAllocateInfo cbai = {};

    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandPool = transferpool;
    cbai.commandBufferCount = 1;

    vkAllocateCommandBuffers(logicaldevice, &cbai, &transfercommandbuffer);
    
    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(transfercommandbuffer, &cbbi);

    VkBufferCopy bufferregion = {};
    bufferregion.srcOffset = 0;
    bufferregion.dstOffset = 0;
    bufferregion.size = buffersize;

    vkCmdCopyBuffer(transfercommandbuffer, srcbuffer, dstbuffer, 1, &bufferregion);


    vkEndCommandBuffer(transfercommandbuffer);

    VkSubmitInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &transfercommandbuffer;

    vkQueueSubmit(transferqueue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(transferqueue);

    //free temp command buffer
    vkFreeCommandBuffers(logicaldevice, transferpool, 1, &transfercommandbuffer);

    return;
}
void vulkeng_create_buffer(VkDevice logicaldevice, VkPhysicalDevice physdevice, VkDeviceSize buffersize, VkBufferUsageFlags bufferflags,
                             VkMemoryPropertyFlags memoryproperties, VkBuffer* buffer,VkDeviceMemory* buffermemory)
{
    VkBufferCreateInfo bci = {};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size = buffersize;
    bci.usage = bufferflags;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult result = vkCreateBuffer(logicaldevice, &bci, nullptr, buffer);
    if(result != VK_SUCCESS)
    {
        printf("Failed to create mesh vertex buffer.");
    }
    VkMemoryRequirements memreqs = {};
    vkGetBufferMemoryRequirements(logicaldevice, *buffer, &memreqs);

    VkMemoryAllocateInfo    mai = {};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = memreqs.size;
    mai.memoryTypeIndex = vulkeng_find_memory_type_index(memreqs.memoryTypeBits, 
                                                        memoryproperties, 
                                                        physdevice);
    
    
    result = vkAllocateMemory(logicaldevice, &mai, nullptr, buffermemory);
    if(result != VK_SUCCESS)
    {
        printf("Buffer allocate failed.");
    }
    vkBindBufferMemory(logicaldevice,*buffer, *buffermemory, 0);
    return;
}

void            vulkeng_create_mesh_vertex_buffer(vulkeng_device* vdevice, mesh* vmesh,VkQueue transferqueue, VkCommandPool transferpool,
                                                 std::vector<vertex> * vertices, VkDeviceMemory* devicememory)
{
    VkDeviceSize buffersize = sizeof(vertex) * vertices->size();

    VkBuffer stagingbuffer;
    VkDeviceMemory stagingmemory;

    vulkeng_create_buffer(vdevice->logicaldevice, vdevice->physicaldevice, buffersize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                                    &stagingbuffer, &stagingmemory);
    
    void* data;
    vkMapMemory(vmesh->mesh_logical_device, stagingmemory, 0, buffersize, 0, &data);
    memcpy(data, vertices->data(), (size_t)buffersize);
    vkUnmapMemory(vmesh->mesh_logical_device, stagingmemory);

    vulkeng_create_buffer(vdevice->logicaldevice, vdevice->physicaldevice, buffersize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vmesh->mesh_vertex_buffer, &vmesh->mesh_device_memory);
    
    vulkeng_copy_buffer(vmesh->mesh_logical_device, transferqueue, transferpool, stagingbuffer, vmesh->mesh_vertex_buffer, buffersize);

    vkDestroyBuffer(vmesh->mesh_logical_device, stagingbuffer, nullptr);
    vkFreeMemory(vmesh->mesh_logical_device, stagingmemory, nullptr);


    return;
}

void            vulkeng_create_mesh_index_buffer(vulkeng_device* vdevice, mesh* vmesh,VkQueue transferqueue, VkCommandPool transferpool,
                                                 std::vector<u32> * indexes, VkDeviceMemory* devicememory)
{
    VkDeviceSize buffersize = sizeof(u32) * indexes->size();
    
    VkBuffer stagingbuffer;
    VkDeviceMemory stagingmemory;

    vulkeng_create_buffer(vdevice->logicaldevice, vdevice->physicaldevice, buffersize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingbuffer, &stagingmemory);
    
    void* data;
    vkMapMemory(vmesh->mesh_logical_device, stagingmemory, 0, buffersize, 0, &data);
    memcpy(data, indexes->data(), (size_t)buffersize);
    vkUnmapMemory(vmesh->mesh_logical_device, stagingmemory);

    vulkeng_create_buffer(vmesh->mesh_logical_device, vmesh->mesh_physical_device, buffersize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vmesh->indexbuffer, &vmesh->indexbuffermemory);
    
    vulkeng_copy_buffer(vmesh->mesh_logical_device, transferqueue, transferpool, stagingbuffer, vmesh->indexbuffer, buffersize);
    vkDestroyBuffer(vmesh->mesh_logical_device, stagingbuffer, nullptr);
    vkFreeMemory(vmesh->mesh_logical_device, stagingmemory, nullptr);
                                       
    return;
}

void        vulkeng_create_mesh(mesh* vmesh,  vulkeng_device *vdevice, VkQueue transferqueue, VkCommandPool transferpool, 
                                std::vector<vertex>* vertices, std::vector<u32>* indices, VkDeviceMemory* devicememory)
{
    vmesh->indexcount = indices->size();
    vmesh->mesh_vertex_count = vertices->size();
    vmesh->mesh_physical_device = vdevice->physicaldevice;
    vmesh->mesh_logical_device = vdevice->logicaldevice;
    vmesh->mesh_device_memory = *devicememory;
    vulkeng_create_mesh_vertex_buffer(vdevice, vmesh, transferqueue, transferpool, vertices, devicememory);
    vulkeng_create_mesh_index_buffer(vdevice, vmesh, transferqueue, transferpool, indices, devicememory);


}
u32             vulkeng_get_mesh_vertex_count(mesh* vmesh)
{
    return vmesh->mesh_vertex_count;
}
VkBuffer        vulkeng_get_mesh_vertex_buffer(mesh* vmesh)
{
    return vmesh->mesh_vertex_buffer;
}
u32             vulkeng_get_mesh_index_count(mesh* vmesh)
{
    return vmesh->indexcount;
}
VkBuffer        vulkeng_get_mesh_index_buffer(mesh* vmesh)
{
    return vmesh->indexbuffer;
}
void            vulkeng_mesh_destroy_buffers(mesh* vmesh, VkDeviceMemory devicememory)
{
    vkDestroyBuffer(vmesh->mesh_logical_device, vmesh->mesh_vertex_buffer, nullptr); 
    vkFreeMemory(vmesh->mesh_logical_device, devicememory, nullptr);
    vkDestroyBuffer(vmesh->mesh_logical_device, vmesh->indexbuffer, nullptr);


}



#endif
