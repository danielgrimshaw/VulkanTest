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
#include "BUILD_OPTIONS.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#if BUILD_ENABLE_MODEL
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#endif

#include <chrono>
#include <iostream>

#if BUILD_ENABLE_MODEL
#include <unordered_map>
#else

const std::vector<Vertex> vertices = {
	{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
	{ { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
	{ { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },

	{ { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
	{ { 0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
	{ { -0.5f, 0.5f, -0.5f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } }
};

const std::vector<uint32_t> indices = {
	0, 1, 2,
	2, 3, 0,

	4, 5, 6,
	6, 7, 4
};
#endif

#if BUILD_ENABLE_MODEL
const std::string MODEL_PATH = "models/chalet.obj";
const std::string TEXTURE_PATH = "textures/chalet.jpg";
#else
const std::string MODEL_PATH = "";
const std::string TEXTURE_PATH = "textures/texture.jpg";
#endif

int main(void) {
	Renderer r;

#if BUILD_ENABLE_MODEL
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
#endif
	r.openWindow(800, 600, "Vulkan Test");

	VkCommandPool command_pool;

	VkCommandPoolCreateInfo command_pool_create_info {};
	command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.queueFamilyIndex = r.getGraphicsFamilyIndex();
	command_pool_create_info.flags = 0;

	ErrorCheck(vkCreateCommandPool(r.getDevice(), &command_pool_create_info, nullptr, &command_pool));

	// Create depth resources
	VkImage depth_image;
	VkDeviceMemory depth_image_memory;
	VkImageView depth_image_view;

	VkFormat depth_format = r.findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);

	r.createImage(r.getWindow()->getWidth(), r.getWindow()->getHeight(), depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_image, depth_image_memory);
	r.createImageView(depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, depth_image_view);

	r.transitionImageLayout(command_pool, depth_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	r.makeFramebuffers(depth_image_view);

	// Create texture image
	int tex_width, tex_height, tex_channels;
	stbi_uc * pixels = stbi_load(TEXTURE_PATH.c_str(), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
	VkDeviceSize image_size = tex_width * tex_height * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	VkImage staging_image;
	VkDeviceMemory staging_image_memory;
	VkImage texture_image;
	VkDeviceMemory texture_image_memory;

	r.createImage(tex_width, tex_height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_image, staging_image_memory);

	void * data;
	ErrorCheck(vkMapMemory(r.getDevice(), staging_image_memory, 0, image_size, 0, &data));
	memcpy(data, pixels, (size_t)image_size);
	vkUnmapMemory(r.getDevice(), staging_image_memory);
	stbi_image_free(pixels);

	r.createImage(tex_width, tex_height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture_image, texture_image_memory);

	// Copy staging image to texture image
	r.transitionImageLayout(command_pool, staging_image, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	r.transitionImageLayout(command_pool, texture_image, VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	r.copyImage(command_pool, staging_image, texture_image, tex_width, tex_height);

	r.transitionImageLayout(command_pool, texture_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Create texture image view
	VkImageView texture_image_view;

	r.createImageView(texture_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, texture_image_view);

	// Create Texture Sampler
	VkSampler texture_sampler;

	VkSamplerCreateInfo sampler_info;
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.anisotropyEnable = VK_TRUE;
	sampler_info.maxAnisotropy = 16;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;
	sampler_info.pNext = NULL;
	sampler_info.flags = 0;

	ErrorCheck(vkCreateSampler(r.getDevice(), &sampler_info, nullptr, &texture_sampler));

#if BUILD_ENABLE_MODEL
	// Load Model
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	std::cout << "Loading Model" << std::endl;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, MODEL_PATH.c_str())) {
		throw std::runtime_error(err);
	}
	std::cout << "Done Loading Model" << std::endl;
	
	std::unordered_map<Vertex, int> unique_vertices = {};

	for (const auto & shape : shapes) {
		for (const auto & index : shape.mesh.indices) {
			Vertex vertex {};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			if (unique_vertices.count(vertex) == 0) {
				unique_vertices[vertex] = vertices.size();
				vertices.push_back(vertex);
			}
			
			indices.push_back(unique_vertices[vertex]);
		}
	}
#endif
	
	// Create Vertex Buffer
	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_buffer_memory;
	VkDeviceSize vertex_buffer_size = sizeof(vertices[0]) * vertices.size();

	VkBuffer vertex_staging_buffer;
	VkDeviceMemory vertex_staging_buffer_memory;

	r.createBuffer(vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertex_staging_buffer, vertex_staging_buffer_memory);

	// data is already defined and is now re-used
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

	VkDescriptorImageInfo image_info {};
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.imageView = texture_image_view;
	image_info.sampler = texture_sampler;

	// Info for writing descriptor
	std::array<VkWriteDescriptorSet, 2> descriptor_writes {};
	descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].dstSet = descriptor_set;
	descriptor_writes[0].dstBinding = 0;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pBufferInfo = &buffer_info; // Used if descriptor is buffer data
	descriptor_writes[0].pImageInfo = nullptr; // Used if descriptor is image data
	descriptor_writes[0].pTexelBufferView = nullptr; // Used if descriptor is buffer views

	descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[1].dstSet = descriptor_set;
	descriptor_writes[1].dstBinding = 1;
	descriptor_writes[1].dstArrayElement = 0;
	descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_writes[1].descriptorCount = 1;
	descriptor_writes[1].pBufferInfo = nullptr;
	descriptor_writes[1].pImageInfo = &image_info;
	descriptor_writes[1].pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(r.getDevice(), descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);

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

		std::array<VkClearValue, 2> clear_values = {};
		clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clear_values[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo render_pass_begin_info{};
		render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin_info.renderPass = r.getRenderPass();
		render_pass_begin_info.framebuffer = r.getSwapchainFramebuffers()[i];
		render_pass_begin_info.renderArea.offset = { 0, 0 };
		render_pass_begin_info.renderArea.extent.width = r.getWindow()->getWidth();
		render_pass_begin_info.renderArea.extent.height = r.getWindow()->getHeight();
		render_pass_begin_info.clearValueCount = clear_values.size();
		render_pass_begin_info.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, r.getGraphicsPipeline());

		VkBuffer vertex_buffers[] = { vertex_buffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(command_buffers[i], 0, 1, vertex_buffers, offsets);

		vkCmdBindIndexBuffer(command_buffers[i], index_buffer, 0, VK_INDEX_TYPE_UINT32);

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
	vkDestroySampler(r.getDevice(), texture_sampler, nullptr);
	texture_sampler = nullptr;
	vkDestroyImageView(r.getDevice(), texture_image_view, nullptr);
	texture_image_view = nullptr;
	vkFreeMemory(r.getDevice(), staging_image_memory, nullptr);
	staging_image_memory = nullptr;
	vkDestroyImage(r.getDevice(), staging_image, nullptr);
	staging_image = nullptr;
	vkFreeMemory(r.getDevice(), texture_image_memory, nullptr);
	texture_image_memory = nullptr;
	vkDestroyImage(r.getDevice(), texture_image, nullptr);
	texture_image = nullptr;
	vkDestroyImageView(r.getDevice(), depth_image_view, nullptr);
	depth_image_view = nullptr;
	vkFreeMemory(r.getDevice(), depth_image_memory, nullptr);
	depth_image_memory = nullptr;
	vkDestroyImage(r.getDevice(), depth_image, nullptr);
	depth_image = nullptr;
	vkDestroyCommandPool(r.getDevice(), command_pool, nullptr);
	command_pool = nullptr;

	return 0;
}