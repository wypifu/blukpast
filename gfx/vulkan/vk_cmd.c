#include <gfx/vulkan/include/vk_cmd.h>

void bkpCmdBeginRendering(VkCommandBuffer cmd, const VkRenderingInfo * info)
{
    vkCmdBeginRendering(cmd, info);
}

void bkpCmdEndRendering(VkCommandBuffer cmd)
{
    vkCmdEndRendering(cmd);
}

void bkpCmdSetViewport(VkCommandBuffer cmd, const VkViewport * vp)
{
    vkCmdSetViewport(cmd, 0, 1, vp);
}

void bkpCmdSetScissor(VkCommandBuffer cmd, const VkRect2D * sc)
{
    vkCmdSetScissor(cmd, 0, 1, sc);
}

void bkpCmdBindPipeline(VkCommandBuffer cmd, VkPipeline pipeline)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void bkpCmdBindDescriptorSets(VkCommandBuffer cmd, VkPipelineLayout layout,
                               uint32_t firstSet, uint32_t count, const VkDescriptorSet * sets)
{
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, firstSet, count, sets, 0, NULL);
}

void bkpCmdBindVertexBuffer(VkCommandBuffer cmd, uint32_t binding,
                             VkBuffer buffer, VkDeviceSize offset)
{
    vkCmdBindVertexBuffers(cmd, binding, 1, &buffer, &offset);
}

void bkpCmdBindIndexBuffer(VkCommandBuffer cmd, VkBuffer buffer,
                            VkDeviceSize offset, VkIndexType indexType)
{
    vkCmdBindIndexBuffer(cmd, buffer, offset, indexType);
}

void bkpCmdPushConstants(VkCommandBuffer cmd, VkPipelineLayout layout,
                          VkShaderStageFlags stages, uint32_t offset,
                          uint32_t size, const void * data)
{
    vkCmdPushConstants(cmd, layout, stages, offset, size, data);
}

void bkpCmdDraw(VkCommandBuffer cmd, uint32_t vertexCount, uint32_t firstVertex)
{
    vkCmdDraw(cmd, vertexCount, 1, firstVertex, 0);
}

void bkpCmdDrawIndexed(VkCommandBuffer cmd, uint32_t indexCount,
                        uint32_t firstIndex, int32_t vertexOffset)
{
    vkCmdDrawIndexed(cmd, indexCount, 1, firstIndex, vertexOffset, 0);
}

void bkpCmdMemoryBarrier(VkCommandBuffer cmd,
                         VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                         VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)
{
    VkMemoryBarrier b   = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    b.srcAccessMask     = srcAccess;
    b.dstAccessMask     = dstAccess;
    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 1, &b, 0, NULL, 0, NULL);
}

void bkpCmdBarrierImages(VkCommandBuffer cmd,
                          const BkpImageBarrierInfo * barriers,
                          uint32_t count)
{
    VkImageMemoryBarrier2 b[count];
    for(uint32_t i = 0; i < count; ++i)
    {
        b[i]                            = (VkImageMemoryBarrier2){};
        b[i].sType                      = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        b[i].srcStageMask               = barriers[i].srcStage;
        b[i].srcAccessMask              = barriers[i].srcAccess;
        b[i].dstStageMask               = barriers[i].dstStage;
        b[i].dstAccessMask              = barriers[i].dstAccess;
        b[i].oldLayout                  = barriers[i].oldLayout;
        b[i].newLayout                  = barriers[i].newLayout;
        b[i].srcQueueFamilyIndex        = VK_QUEUE_FAMILY_IGNORED;
        b[i].dstQueueFamilyIndex        = VK_QUEUE_FAMILY_IGNORED;
        b[i].image                      = barriers[i].image;
        b[i].subresourceRange.aspectMask     = barriers[i].aspect;
        b[i].subresourceRange.baseMipLevel   = barriers[i].baseMip;
        b[i].subresourceRange.levelCount     = barriers[i].mipCount;
        b[i].subresourceRange.baseArrayLayer = barriers[i].baseLayer;
        b[i].subresourceRange.layerCount     = barriers[i].layerCount;
    }
    VkDependencyInfo dep = {};
    dep.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dep.imageMemoryBarrierCount  = count;
    dep.pImageMemoryBarriers     = b;
    vkCmdPipelineBarrier2(cmd, &dep);
}
