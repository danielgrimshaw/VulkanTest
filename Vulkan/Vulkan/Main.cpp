/* Copyright (C) 2016 Daniel Grimshaw
*
* Main.cpp | The starting point of an exploration into the Vulkan API
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Renderer.h"
#include "Window.h"
#include "util.h"

const std::vector<Vertex> vertices = {
	{ { 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
	{ { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },
	{ { -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } }
};

int main(void) {
	Renderer r;

	r.openWindow(800, 600, "Vulkan Test");

	VkCommandPool command_pool;

	VkCommandPoolCreateInfo command_pool_create_info {};
	command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.queueFamilyIndex = r.getGraphicsFamilyIndex();
	command_pool_create_info.flags = 0;

	ErrorCheck(vkCreateCommandPool(r.getDevice(), &command_pool_create_info, nullptr, &command_pool));
	
	// Create Vertex Buffer
	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_buffer_memory;
	VkDeviceSize vertex_buffer_size = sizeof(vertices[0]) * vertices.size();

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;

	r.createBuffer(vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

	void * data;
	ErrorCheck(vkMapMemory(r.getDevice(), staging_buffer_memory, 0, vertex_buffer_size, 0, &data));
	memcpy(data, vertices.data(), (size_t)vertex_buffer_size);
	vkUnmapMemory(r.getDevice(), staging_buffer_memory);

	r.createBuffer(vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer, vertex_buffer_memory);

	// Copy staging buffer into vertex buffer
	r.copyBuffer(command_pool, staging_buffer, vertex_buffer, vertex_buffer_size);

	// Create Command Buffers
	std::vector<VkCommandBuffer> command_buffers(r.getSwapchainFramebuffers().size());

	VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.commandPool = command_pool;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount = (uint32_t)command_buffers.size();

	ErrorCheck(vkAllocateCommandBuffers(r.getDevice(), &command_buffer_allocate_info, command_buffers.data()));

	for (size_t i = 0; i < command_buffers.size(); i++) {
		VkCommandBufferBeginInfo command_buffer_begin_info = {};
		command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		command_buffer_begin_info.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(command_buffers[i], &command_buffer_begin_info);

		VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };

		VkRenderPassBeginInfo render_pass_begin_info{};
		render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin_info.renderPass = r.getRenderPass();
		render_pass_begin_info.framebuffer = r.getSwapchainFramebuffers()[i];
		render_pass_begin_info.renderArea.offset = { 0, 0 };
		render_pass_begin_info.renderArea.extent.width = r.getWindow()->getWidth();
		render_pass_begin_info.renderArea.extent.height = r.getWindow()->getHeight();
		render_pass_begin_info.clearValueCount = 1;
		render_pass_begin_info.pClearValues = &clear_color;

		vkCmdBeginRenderPass(command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, r.getGraphicsPipeline());

		VkBuffer vertex_buffers[] = { vertex_buffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(command_buffers[i], 0, 1, vertex_buffers, offsets);

		vkCmdDraw(command_buffers[i], 3, 1, 0, 0);
		vkCmdEndRenderPass(command_buffers[i]);

		ErrorCheck(vkEndCommandBuffer(command_buffers[i]));
	}

	VkSemaphore image_available;
	VkSemaphore render_finished;

	VkSemaphoreCreateInfo semaphore_create_info {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	ErrorCheck(vkCreateSemaphore(r.getDevice(), &semaphore_create_info, nullptr, &image_available));
	ErrorCheck(vkCreateSemaphore(r.getDevice(), &semaphore_create_info, nullptr, &render_finished));
	
	while (r.run()) { // main loop
		uint32_t image_index;
		vkAcquireNextImageKHR(r.getDevice(), r.getWindow()->getSwapchain(), UINT64_MAX, image_available, VK_NULL_HANDLE, &image_index);

		VkSemaphore wait_semaphores[] = { image_available };
		VkSemaphore signal_semaphores[] = { render_finished };

		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSubmitInfo submit_info {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = wait_semaphores;
		submit_info.pWaitDstStageMask = wait_stages;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffers[image_index];
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = signal_semaphores;
		
		ErrorCheck(vkQueueSubmit(r.getQueue(), 1, &submit_info, VK_NULL_HANDLE));

		VkSubpassDependency subpass_dependency {};
		subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpass_dependency.dstSubpass = 0;
		subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpass_dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo render_pass_create_info {};
		render_pass_create_info.dependencyCount = 1;
		render_pass_create_info.pDependencies = &subpass_dependency;

		VkSwapchainKHR swapchains[] = { r.getWindow()->getSwapchain() };

		VkPresentInfoKHR present_info {};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = signal_semaphores;
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swapchains;
		present_info.pImageIndices = &image_index;
		present_info.pResults = nullptr;

		ErrorCheck(vkQueuePresentKHR(r.getQueue(), &present_info));
	}

	vkQueueWaitIdle(r.getQueue());

	vkDestroySemaphore(r.getDevice(), image_available, nullptr);
	image_available = nullptr;
	vkDestroySemaphore(r.getDevice(), render_finished, nullptr);
	render_finished = nullptr;
	vkFreeCommandBuffers(r.getDevice(), command_pool, (uint32_t) command_buffers.size(), command_buffers.data());
	vkFreeMemory(r.getDevice(), vertex_buffer_memory, nullptr);
	vertex_buffer_memory = nullptr;
	vkFreeMemory(r.getDevice(), staging_buffer_memory, nullptr);
	staging_buffer_memory = nullptr;
	vkDestroyBuffer(r.getDevice(), vertex_buffer, nullptr);
	vertex_buffer = nullptr;
	vkDestroyBuffer(r.getDevice(), staging_buffer, nullptr);
	staging_buffer = nullptr;
	vkDestroyCommandPool(r.getDevice(), command_pool, nullptr);
	command_pool = nullptr;

	return 0;
}