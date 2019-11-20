#include "graphics/command/commands.hpp"
#include "graphics/surface/swapchain.hpp"
#include "graphics/surface/framebuffer.hpp"
#include "graphics/memory/primitive_buffer.hpp"
#include "graphics/memory/shader_buffer.hpp"
#include "graphics/memory/texture.hpp"
#include "graphics/shader/sampler.hpp"
#include "graphics/shader/pipeline.hpp"
#include "graphics/shader/descriptors.hpp"
#include "graphics/enums.hpp"
#include "graphics/graphics.hpp"
#include "system/viewport_manager.hpp"
#include "system/viewport_interface.hpp"
#include "system/local_file_system.hpp"
#include "utils/hash.hpp"

using namespace ignis;
using namespace oic;
using namespace cmd;

struct TestViewportInterface : public ViewportInterface {

	//Construct graphics instance

	Graphics g;

	//Resources

	HashMap<const ViewportInfo*, Swapchain*> swapchains{};
	Framebuffer *intermediate{};
	CommandList *cl{};
	PrimitiveBuffer *mesh{};
	Descriptors *descriptors{}, *computeDescriptors{};
	Pipeline *pipeline{}, *computePipeline{};
	GPUBuffer *uniforms{};
	Texture *tex2D{}, *computeOutput{};
	Sampler *samp{};

	//TODO: Demonstrate multiple windows
	//TODO: Compute and use render targets

	//Create resources

	TestViewportInterface() {
		
		intermediate = new Framebuffer(
			g, NAME("Framebuffer"),
			Surface::Info(
				{ GPUFormat::RGBA8 }, DepthFormat::NONE, false, 8
			)
		);

		//Create primitive buffer

		List<BufferAttributes> attrib{ { GPUFormat::RG32f } };

		const List<Vec2f> vboBuffer{
			{ 0.5, -0.5 }, { -1, -1 },
			{ -1, 1 }, { 0.5, 0.5 }
		};

		const List<u8> iboBuffer{
			0,3,2, 2,1,0
		};

		mesh = new PrimitiveBuffer(g, NAME("Test mesh"),
			PrimitiveBuffer::Info(
				BufferLayout(vboBuffer, attrib[0]), 
				BufferLayout(iboBuffer, GPUFormat::R8u)
			)
		);

		//Create uniform buffer

		const Vec3f mask = { 1, 1, 1 };

		uniforms = new ShaderBuffer(
			g, NAME("Test pipeline uniform buffer"),
			ShaderBuffer::Info(
				HashMap<String, ShaderBuffer::Layout>{ { NAME("mask"), { 0, mask } } },
				GPUBufferType::UNIFORM, GPUMemoryUsage::LOCAL
			)
		);

		//Create texture

		const u32 rgba0[4][2] = {

			{ 0xFFFF00FF, 0xFF00FFFF },	//1,0,1, 0,1,1
			{ 0xFFFFFF00, 0xFFFFFFFF },	//1,1,0, 1,1,1

			{ 0xFF7F007F, 0xFF7F0000 },	//.5,0,.5, .5,0,0
			{ 0xFF7F7F7F, 0xFF007F00 }	//.5,.5,.5, 0,.5,0
		};

		const u32 rgba1[2][1] = {
			{ 0xFFBFBFBF },				//0.75,0.75,0.75
			{ 0xFF5F7F7F }				//0.375,0.5,0.5
		};

		tex2D = new Texture(
			g, NAME("Test texture"),
			Texture::Info(
				List<Grid2D<u32>>{
					rgba0, rgba1
				}, 
				GPUFormat::RGBA8, GPUMemoryUsage::LOCAL, 2
			)
		);

		//Create sampler

		samp = new Sampler(
			g, NAME("Test sampler"), Sampler::Info()
		);

		//Create compute target

		computeOutput = new Texture(
			g, NAME("Compute output"),
			Texture::Info(
				Vec2u{ 512, 512 }, GPUFormat::RGBA16f, GPUMemoryUsage::LOCAL, 1, 1
			)
		);

		//Load shader code
		//(Compute output 512x512)

		Buffer comp;
		bool success = oic::System::files()->read("./shaders/test.comp.spv", comp);

		if (!success)
			oic::System::log()->fatal("Couldn't find compute shader");

		//Create layout for compute

		PipelineLayout pipelineLayout(
			RegisterLayout(
				NAME("Output"), 0, TextureType::TEXTURE_2D, 0, ACCESS_COMPUTE, true
			)
		);

		auto descriptorsInfo = Descriptors::Info(pipelineLayout, {});
		descriptorsInfo.resources[0] = { computeOutput, TextureType::TEXTURE_2D };

		computeDescriptors = new Descriptors(
			g, NAME("Compute descriptors"), 
			descriptorsInfo
		);

		//Create pipeline (shader and render states)

		computePipeline = new Pipeline(
			g, NAME("Compute pipeline"),
			Pipeline::Info(
				Pipeline::Flag::OPTIMIZE,
				comp,
				pipelineLayout,
				Vec3u{ 16, 16, 1 }
			)
		);


		//Load shader code
		//(Mask shader)

		Buffer vert, frag;
		success =	oic::System::files()->read("./shaders/test.vert.spv", vert);
		success		&=	oic::System::files()->read("./shaders/test.frag.spv", frag);

		if (!success)
			oic::System::log()->fatal("Couldn't find shaders");

		//Create descriptors that should be bound

		pipelineLayout = {

			RegisterLayout(
				NAME("Test"), 0, GPUBufferType::UNIFORM, 0,
				ACCESS_FRAGMENT, uniforms->size()
			),

			RegisterLayout(
				NAME("test"), 1, SamplerType::SAMPLER_2D, 0,
				ACCESS_FRAGMENT
			)
		};

		descriptorsInfo = Descriptors::Info(pipelineLayout, {});
		descriptorsInfo.resources[0] = uniforms;

		descriptorsInfo.resources[1] = GPUSubresource(
			samp, computeOutput, TextureType::TEXTURE_2D
		);

		descriptors = new Descriptors(
			g, NAME("Test descriptors"), 
			descriptorsInfo
		);

		//Create pipeline (shader and render states)

		pipeline = new Pipeline(
			g, NAME("Test pipeline"),

			Pipeline::Info(

				Pipeline::Flag::OPTIMIZE,

				attrib,

				{ 
					{ ShaderStage::VERTEX, vert }, 
					{ ShaderStage::FRAGMENT, frag } 
				},

				pipelineLayout,
				Pipeline::MSAA(intermediate->getInfo().samples)

				//TODO:
				//DepthStencil()
				//BlendState()
				//RenderPass
				//Parent pipeline / allow parenting (optional)
			)
		);

		//Create command list and store our commands

		cl = new CommandList(g, NAME("Command list"), CommandList::Info(1_KiB));

		cl->add(

			//Render to compute shader

			BindPipeline(computePipeline),
			BindDescriptors(computeDescriptors),
			Dispatch(computeOutput->getInfo().dimensions),
			
			//Clear and bind MSAA

			SetClearColor(Vec4f { 0.586f, 0.129f, 0.949f, 1.0f }),
			BeginFramebuffer(intermediate),

			//TODO: BeginRenderPass instead of BeginFramebuffer

			//Draw primitive

			BindPipeline(pipeline),
			BindDescriptors(descriptors),
			BindPrimitiveBuffer(mesh),
			DrawInstanced(mesh->elements()),

			//Present to surface

			EndFramebuffer()
		);

		//Pre-render and store rendered result.

		g.pause();
	}

	//Cleanup resources

	~TestViewportInterface() {
		destroy(
			samp, tex2D, uniforms, cl, mesh, computeOutput,
			computePipeline, computeDescriptors,
			pipeline, descriptors, intermediate
		);
	}

	//Create viewport resources

	void init(ViewportInfo *vp) final override {

		if (swapchains.size())
			oic::System::log()->fatal("Currently only supporting 1 viewport");

		//Create MSAA render target and window swapchain

		swapchains[vp] = new Swapchain(
			g, NAME("Swapchain"), 
			Swapchain::Info{ vp, false, DepthFormat::NONE }
		);
	}

	//Delete viewport resources

	void release(const ViewportInfo *vp) final override {
		delete swapchains[vp];
		swapchains.erase(vp);
	}

	//Update size of surfaces

	void resize(const ViewportInfo *vp, const Vec2u &size) final override {
		intermediate->onResize(size);
		swapchains[vp]->onResize(size);
	}

	//Execute commandList

	void render(const ViewportInfo *vp) final override {

		//Copy pre-rendered result to viewports

		g.present(intermediate, swapchains[vp], cl);
	}

};

//Create window and wait for exit

int main() {

	TestViewportInterface viewportInterface;

	System::viewportManager()->create(
		ViewportInfo("Test window", {}, {}, 0, &viewportInterface)
	);

	//TODO: Better way of stalling than this; like interrupt
	while (System::viewportManager()->size())
		System::wait(250_ms);

	return 0;
}