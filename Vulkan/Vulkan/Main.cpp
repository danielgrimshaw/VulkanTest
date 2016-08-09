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

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <iostream>

const std::vector<Vertex> vertices = {
	{ { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
	{ { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
	{ { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } },
	{ { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f } }
};

const std::vector<uint16_t> indices = {
	0, 1, 2,
	2, 3, 0
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

	VkBuffer vertex_staging_buffer;
	VkDeviceMemory vertex_staging_buffer_memory;

	r.createBuffer(vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertex_staging_buffer, vertex_staging_buffer_memory);

	void * data;
	ErrorCheck(vkMapMemory(r.getDevice(), vertex_staging_buffer_memory, 0, vertex_buffer_size, 0, &data));
	memcpy(data, vertices.data(), (size_t)vertex_buffer_size);
	vkUnmapMemory(r.getDevice(), vertex_staging_buffer_memory);

	r.createBuffer(vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer, vertex_buffer_memory);

	// Copy staging buffer into vertex buffer
	r.copyBuffer(command_pool, vertex_staging_buffer, vertex_buffer, vertex_buffer_size);

	// Create index buffer
	VkBuffer index_buffer;
	VkDeviceMemory index_buffer_memory;
	VkDeviceSize index_buffer_size = sizeof(indices[0]) * indices.size();

	VkBuffer index_staging_buffer;
	VkDeviceMemory index_staging_buffer_memory;

	r.createBuffer(index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, index_staging_buffer, index_staging_buffer_memory);
	
	ErrorCheck(vkMapMemory(r.getDevice(), index_staging_buffer_memory, 0, index_buffer_size, 0, &data));
	memcpy(data, indices.data(), (size_t)index_buffer_size);
	vkUnmapMemory(r.getDevice(), index_staging_buffer_memory);

	r.createBuffer(index_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer, index_buffer_memory);

	// Copy staging buffer into index buffer
	r.copyBuffer(command_pool, index_staging_buffer, index_buffer, index_buffer_size);

	// Create uniform buffer
	VkBuffer uniform_staging_buffer;
	VkDeviceMemory uniform_staging_buffer_memory;
	VkBuffer uniform_buffer;
	VkDeviceMemory uniform_buffer_memory;

	VkDeviceSize uniform_buffer_size = sizeof(UniformBufferObject);

	r.createBuffer(uniform_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_staging_buffer, uniform_staging_buffer_memory);
	r.createBuffer(uniform_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uniform_buffer, uniform_buffer_memory);

	// Create descriptor set
	VkDescriptorSet descriptor_set;

	VkDescriptorSetLayout layouts[] = { r.getDescriptorSetLayout() };
	VkDescriptorSetAllocateInfo descriptor_set_allocate_info {};
	descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.descriptorPool = r.getDescriptorPool();
	descriptor_set_allocate_info.descriptorSetCount = 1;
	descriptor_set_allocate_info.pSetLayouts = layouts;

	ErrorCheck(vkAllocateDescriptorSets(r.getDevice(), &descriptor_set_allocate_info, &descriptor_set));

	// Configure descriptors
	VkDescriptorBufferInfo buffer_info {};
	buffer_info.buffer = uniform_buffer;
	buffer_info.offset = 0;
	buffer_info.range = sizeof(UniformBufferObject);

	// Info for writing descriptor
	VkWriteDescriptorSet descriptor_write {};
	descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_write.dstSet = descriptor_set;
	descriptor_write.dstBinding = 0;
	descriptor_write.dstArrayElement = 0;
	descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_write.descriptorCount = 1;
	descriptor_write.pBufferInfo = &buffer_info; // Used if descriptor is buffer data
	descriptor_write.pImageInfo = nullptr; // Used if descriptor is image data
	descriptor_write.pTexelBufferView = nullptr; // Used if descriptor is buffer views

	vkUpdateDescriptorSets(r.getDevice(), 1, &descriptor_write, 0, nullptr);

	// Create command buffers
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

		vkCmdBindIndexBuffer(command_buffers[i], index_buffer, 0, VK_INDEX_TYPE_UINT16);

		vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, r.getPipelineLayout(), 0, 1, &descriptor_set, 0, nullptr);

		vkCmdDrawIndexed(command_buffers[i], indices.size(), 1, 0, 0, 0);
		vkCmdEndRenderPass(command_buffers[i]);

		ErrorCheck(vkEndCommandBuffer(command_buffers[i]));
	}

	VkSemaphore image_available;
	VkSemaphore render_finished;

	VkSemaphoreCreateInfo semaphore_create_info {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	ErrorCheck(vkCreateSemaphore(r.getDevice(), &semaphore_create_info, nullptr, &image_available));
	ErrorCheck(vkCreateSemaphore(r.getDevice(), &semaphore_create_info, nullptr, &render_finished));
	
	auto start_time = std::chrono::high_resolution_clock::now();

	while (r.run()) { // main loop
		// Update Uniform Buffer
		auto current_time = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count() / 1000.0f;
		
		UniformBufferObject ubo {};
		ubo.model = glm::rotate(glm::mat4(), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.projection = glm::perspective(glm::radians(45.0f), (float)(r.getWindow()->getSurfaceCapabilities().currentExtent.width) / (float)(r.getWindow()->getSurfaceCapabilities().currentExtent.height), 0.1f, 10.0f);

		ubo.projection[1][1] *= -1.0f; // GLM is for OpenGL, the Y-axis needs to be flipped for Vulkan

		void * data;
		vkMapMemory(r.getDevice(), uniform_staging_buffer_memory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(r.getDevice(), uniform_staging_buffer_memory);

		r.copyBuffer(command_pool, uniform_staging_buffer, uniform_buffer, sizeof(ubo));

		// Main Draw
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
	vkFreeMemory(r.getDevice(), uniform_buffer_memory, nullptr);
	uniform_buffer_memory = nullptr;
	vkFreeMemory(r.getDevice(), uniform_staging_buffer_memory, nullptr);
	uniform_staging_buffer_memory = nullptr;
	vkFreeMemory(r.getDevice(), index_buffer_memory, nullptr);
	index_buffer_memory = nullptr;
	vkFreeMemory(r.getDevice(), index_staging_buffer_memory, nullptr);
	index_staging_buffer_memory = nullptr;
	vkFreeMemory(r.getDevice(), vertex_buffer_memory, nullptr);
	vertex_buffer_memory = nullptr;
	vkFreeMemory(r.getDevice(), vertex_staging_buffer_memory, nullptr);
	vertex_staging_buffer_memory = nullptr;
	vkDestroyBuffer(r.getDevice(), uniform_buffer, nullptr);
	uniform_buffer = nullptr;
	vkDestroyBuffer(r.getDevice(), uniform_staging_buffer, nullptr);
	uniform_staging_buffer = nullptr;
	vkDestroyBuffer(r.getDevice(), index_buffer, nullptr);
	index_buffer = nullptr;
	vkDestroyBuffer(r.getDevice(), index_staging_buffer, nullptr);
	index_staging_buffer = nullptr;
	vkDestroyBuffer(r.getDevice(), vertex_buffer, nullptr);
	vertex_buffer = nullptr;
	vkDestroyBuffer(r.getDevice(), vertex_staging_buffer, nullptr);
	vertex_staging_buffer = nullptr;
	vkDestroyCommandPool(r.getDevice(), command_pool, nullptr);
	command_pool = nullptr;

	return 0;
}