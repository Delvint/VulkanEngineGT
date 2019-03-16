/**
* The Vienna Vulkan Engine
*
* (c) bei Helmut Hlavacs, University of Vienna
*
*/


#include "VEInclude.h"


namespace ve {

	/**
	* \brief Initialize the subrenderer
	*
	* Create descriptor set layout, pipeline layout and the PSO
	*
	*/
	void VESubrenderC1::initSubrenderer() {
		VESubrender::initSubrenderer();

		//per object resources
		vh::vhRenderCreateDescriptorSetLayout(getRendererForwardPointer()->getDevice(),
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
			{ VK_SHADER_STAGE_VERTEX_BIT,	    },
			&m_descriptorSetLayout);

		vh::vhPipeCreateGraphicsPipelineLayout(getRendererForwardPointer()->getDevice(),
			{ m_descriptorSetLayout, getRendererForwardPointer()->getDescriptorSetLayoutPerFrame() },
			&m_pipelineLayout);

		vh::vhPipeCreateGraphicsPipeline(getRendererForwardPointer()->getDevice(),
			"shader/C1/vert.spv", "shader/C1/frag.spv",
			getRendererForwardPointer()->getSwapChainExtent(),
			m_pipelineLayout, getRendererForwardPointer()->getRenderPass(),
			&m_pipeline);

	}

	/**
	* \brief Add an entity to the subrenderer
	*
	* Create a UBO for this entity, a descriptor set per swapchain image, and update the descriptor sets
	*
	*/
	void VESubrenderC1::addEntity(VEEntity *pEntity) {
		VESubrender::addEntity(pEntity);

		vh::vhBufCreateUniformBuffers(getRendererForwardPointer()->getVmaAllocator(),
			(uint32_t)getRendererForwardPointer()->getSwapChainNumber(),
			(uint32_t)sizeof(vh::vhUBOPerObject),
			pEntity->m_uniformBuffers, pEntity->m_uniformBuffersAllocation);

		vh::vhRenderCreateDescriptorSets(getRendererForwardPointer()->getDevice(),
			(uint32_t)getRendererForwardPointer()->getSwapChainNumber(),
			getDescriptorSetLayout(),
			getRendererForwardPointer()->getDescriptorPool(),
			pEntity->m_descriptorSets);

		for (uint32_t i = 0; i < pEntity->m_descriptorSets.size(); i++) {
			vh::vhRenderUpdateDescriptorSet(getRendererForwardPointer()->getDevice(),
				pEntity->m_descriptorSets[i],
				{ pEntity->m_uniformBuffers[i] }, //UBOs
				{ sizeof(vh::vhUBOPerObject) },	//UBO sizes
				{ VK_NULL_HANDLE },	//textureImageViews
				{ VK_NULL_HANDLE }	//samplers
			);
		}
	}
}


