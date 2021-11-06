#ifndef SHADER_READFILE_CPP
#define SHADER_READFILE_CPP

#include <cstdio>
#include <cstdlib>
#include <vulkan\vulkan.h>
#include "types.hpp"

VkResult vulkeng_readspirv_createshadermodule(const char * shaderfilename, VkShaderModule * ad_shadermodule, VkDevice logicaldevice )
{
    VkResult result;
    FILE* shaderfile;
    shaderfile = fopen(shaderfilename, "rb");
    if(shaderfile == NULL)
    {
        result = VK_INCOMPLETE;
        return(result);
    }
    fseek(shaderfile, 0L, SEEK_END);
    i32 shaderfilesize = ftell(shaderfile);
    u8* buffer = (u8*) malloc(shaderfilesize);
    rewind(shaderfile);
    i32 readsize = (i32)fread((void*) buffer, 1, shaderfilesize, shaderfile);
    if(readsize != shaderfilesize)
    {
        result = VK_INCOMPLETE;
        return(result);
    }
    VkShaderModuleCreateInfo vsmci; 
    vsmci.codeSize = shaderfilesize;
    vsmci.flags     = 0;
    vsmci.pCode     = (u32*) buffer;
    vsmci.pNext     = nullptr;
    vsmci.sType     = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    result = vkCreateShaderModule(logicaldevice, &vsmci, nullptr, ad_shadermodule);
    return(result);
}

#endif /*SHADER_READFILE_CPP*/

