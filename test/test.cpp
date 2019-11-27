#include "graphics/command/commands.hpp"
#include "helpers/graphics_object_ref.hpp"
#include "graphics/enums.hpp"
#include "graphics/graphics.hpp"
#include "system/viewport_manager.hpp"
#include "system/viewport_interface.hpp"
#include "system/local_file_system.hpp"
#include "utils/hash.hpp"

using namespace igx;
using namespace oic;

struct TestViewportInterface : public ViewportInterface {

	Graphics &g;

	//Resources

	Swapchain swapchain;
	Framebuffer intermediate;
	CommandList cl;
	PrimitiveBuffer mesh;
	Descriptors descriptors, computeDescriptors;
	Pipeline pipeline, computePipeline;
	ShaderBuffer uniforms;
	Texture tex2D, computeOutput;
	Sampler samp;

	//TODO: Demonstrate multiple windows
	//TODO: Compute and use render targets

	//Create resources

	TestViewportInterface(Graphics &g): g(g) {
		
		intermediate = {
			g, NAME("Framebuffer"),
			Framebuffer::Info(
				{ GPUFormat::RGBA8 }, DepthFormat::NONE, false, 8
			)
		};

		//Create primitive buffer

		List<BufferAttributes> attrib{ { GPUFormat::RG32f } };

		const List<Vec2f> vboBuffer{
			{ 0.5, -0.5 }, { -1, -1 },
			{ -1, 1 }, { 0.5, 0.5 }
		};

		const List<u8> iboBuffer{
			0,3,2, 2,1,0
		};

		mesh = {
			g, NAME("Test mesh"),
			PrimitiveBuffer::Info(
				BufferLayout(vboBuffer, attrib[0]),
				BufferLayout(iboBuffer, GPUFormat::R8u)
			)
		};

		//Create uniform buffer

		const Vec3f mask = { 1, 1, 1 };

		uniforms = {
			g, NAME("Test pipeline uniform buffer"),
			ShaderBuffer::Info(
				HashMap<String, ShaderBufferLayout>{ { NAME("mask"), { 0, mask } } },
				GPUBufferType::UNIFORM, GPUMemoryUsage::LOCAL
			)
		};

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

		tex2D = {
			g, NAME("Test texture"),
			Texture::Info(
				List<Grid2D<u32>>{
					rgba0, rgba1
				},
				GPUFormat::RGBA8, GPUMemoryUsage::LOCAL, 2
			)
		};

		//Create sampler

		samp = { g, NAME("Test sampler"), Sampler::Info() };

		//Create compute target

		computeOutput = {
			g, NAME("Compute output"),
			Texture::Info(
				Vec2u{ 512, 512 }, GPUFormat::RGBA16f, GPUMemoryUsage::LOCAL, 1, 1
			)
		};

		//Load shader code
		//(Compute output 512x512)

		Buffer comp;
		bool success = oic::System::files()->read("./shaders/test.comp.spv", comp);

		if (!success)
			oic::System::log()->fatal("Couldn't find compute shader");

		//Create layout for compute

		PipelineLayout pipelineLayout(
			RegisterLayout(
				NAME("Output"), 0, TextureType::TEXTURE_2D, 0, ShaderAccess::COMPUTE, true
			)
		);

		auto descriptorsInfo = Descriptors::Info(pipelineLayout, {});
		descriptorsInfo.resources[0] = { computeOutput, TextureType::TEXTURE_2D };

		computeDescriptors = {
			g, NAME("Compute descriptors"),
			descriptorsInfo
		};

		//Create pipeline (shader and render states)

		computePipeline = {
			g, NAME("Compute pipeline"),
			Pipeline::Info(
				PipelineFlag::OPTIMIZE,
				comp,
				pipelineLayout,
				Vec3u{ 16, 16, 1 }
			)
		};


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
				ShaderAccess::FRAGMENT, uniforms->size()
			),

			RegisterLayout(
				NAME("test"), 1, SamplerType::SAMPLER_2D, 0,
				ShaderAccess::FRAGMENT
			)
		};

		descriptorsInfo = Descriptors::Info(pipelineLayout, {});
		descriptorsInfo.resources[0] = GPUSubresource(uniforms, 0);

		descriptorsInfo.resources[1] = GPUSubresource(
			samp, computeOutput, TextureType::TEXTURE_2D
		);

		descriptors = {
			g, NAME("Test descriptors"),
			descriptorsInfo
		};

		//Create pipeline (shader and render states)

		pipeline = {
			g, NAME("Test pipeline"),

			Pipeline::Info(

				PipelineFlag::OPTIMIZE,

				attrib,

				{
					{ ShaderStage::VERTEX, vert },
					{ ShaderStage::FRAGMENT, frag }
				},

				pipelineLayout,
				PipelineMSAA(intermediate->getInfo().samples, .2f)

				//TODO:
				//DepthStencil()
				//RenderPass
				//Parent pipeline / allow parenting (optional)
			)
		};

		//Create command list and store our commands

		cl = { g, NAME("Command list"), CommandList::Info(1_KiB) };

		cl->add(

			//Render to compute shader

			BindPipeline(computePipeline),
			BindDescriptors(computeDescriptors),
			Dispatch(computeOutput->getInfo().dimensions),
			
			//Clear and bind MSAA

			SetClearColor(Vec4f{ 0.586f, 0.129f, 0.949f, 1.0f }),
			BeginFramebuffer(intermediate),
			ClearFramebuffer(intermediate),
			SetViewportAndScissor(),

			//TODO: BeginRenderPass instead of BeginFramebuffer

			//Draw primitive

			BindPipeline(pipeline),
			BindDescriptors(descriptors),
			BindPrimitiveBuffer(mesh),
			DrawInstanced(mesh->elements()),

			//Present to surface

			EndFramebuffer()
		);

		//Release the graphics instance for us until we need it again

		g.pause();
	}

	//Create viewport resources

	void init(ViewportInfo *vp) final override {

		if (swapchain.exists())
			oic::System::log()->fatal("Currently only supporting 1 viewport");

		//Create MSAA render target and window swapchain

		swapchain = {
			g, NAME("Swapchain"),
			Swapchain::Info{ vp, false, DepthFormat::NONE }
		};
	}

	//Delete viewport resources

	void release(const ViewportInfo*) final override {
		swapchain.release();
	}

	//Update size of surfaces

	void resize(const ViewportInfo*, const Vec2u &size) final override {
		intermediate->onResize(size);
		swapchain->onResize(size);
	}

	//Execute commandList

	void render(const ViewportInfo*) final override {

		//Copy pre-rendered result to viewports

		g.present(intermediate, swapchain, cl);
	}

};

//Create window and wait for exit

int notmain() {

	Graphics g;
	TestViewportInterface viewportInterface(g);

	System::viewportManager()->create(
		ViewportInfo("Test window", {}, {}, 0, &viewportInterface)
	);

	//TODO: Better way of stalling than this; like interrupt
	while (System::viewportManager()->size())
		System::wait(250_ms);

	return 0;
}