#include "graphics/command/commands.hpp"
#include "helpers/graphics_object_ref.hpp"
#include "graphics/enums.hpp"
#include "graphics/graphics.hpp"
#include "system/viewport_manager.hpp"
#include "system/viewport_interface.hpp"
#include "system/local_file_system.hpp"
#include "gui/gui.hpp"
#include "types/mat.hpp"
#include "input/keyboard.hpp"

using namespace igx;
using namespace oic;

struct TestViewportInterface : public ViewportInterface {

	Graphics &g;
	GUI *gui;

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

	Vec2u32 res;
	Vec3f32 eye{ 0, 0, 5 };

	//TODO: Demonstrate multiple windows
	//TODO: Compute and use render targets

	//Data on the GPU (test shader)
	struct UniformBuffer {

		Mat4x4f32 proj, view;
		Vec3f32 mask;

		static inline UniformBuffer lookAtProj(
			f32 aspect,
			const Vec3f32 &eye = { 0, 0, 5 },
			const Vec3f32 &center = { 0, 0, 0 },
			f32 fov = f32(70_deg), f32 near = .1f, f32 far = 100,
			const Vec3f32 &mask = { 1, 1, 1 },
			const Vec3f32 &up = { 0, 1, 0 }
		){
			return {
				Mat4x4f32::perspective(fov, aspect, near, far),
				Mat4x4f32::lookAt(eye, center, up),
				mask
			};
		}

		static inline UniformBuffer lookDirectionProj(
			f32 aspect,
			const Vec3f32 &eye = { 0, 0, 5 },
			const Vec3f32 &dir = { 0, 0, -1 },
			f32 fov = f32(70_deg), f32 near = .1f, f32 far = 100,
			const Vec3f32 &mask = { 1, 1, 1 },
			const Vec3f32 &up = { 0, 1, 0 }
		){
			return {
				Mat4x4f32::perspective(fov, aspect, near, far),
				Mat4x4f32::lookDirection(eye, dir, up),
				mask
			};
		}

	};

	//Create resources

	TestViewportInterface(Graphics &g): g(g) {

		intermediate = {
			g, NAME("Framebuffer"),
			Framebuffer::Info(
				{ GPUFormat::RGBA8 }, DepthFormat::D32, true, 8
			)
		};

		//Create primitive buffer

		List<BufferAttributes> attrib{ { GPUFormat::RGB32f } };

		const List<Vec3f32> vboBuffer{
			{ -1, -1, -1 }, { 1, -1, -1 },
			{ 1, 1, -1 },	{ -1, 1, -1 },
			{ -1, -1, 1 },	{ 1, -1, 1 },
			{ 1, 1, 1 },	{ -1, 1, 1 }
		};

		const List<u8> iboBuffer{
			0,1,2, 2,3,0,			//Bottom
			4,7,6, 6,5,4,			//Top
			0,4,7, 7,3,0,			//Left
			1,2,6, 6,5,1,			//Right
			7,3,2, 2,6,7,			//Front
			1,5,4, 4,0,1			//Back
		};

		mesh = {
			g, NAME("Test mesh"),
			PrimitiveBuffer::Info(
				BufferLayout(vboBuffer, attrib[0]),
				BufferLayout(iboBuffer, GPUFormat::R8u)
			)
		};

		//Create uniform buffer

		uniforms = {
			g, NAME("Test pipeline uniform buffer"),
			ShaderBuffer::Info(
				GPUBufferType::UNIFORM, GPUMemoryUsage::CPU_WRITE,
				{ { NAME("mask"), ShaderBufferLayout(0, Buffer(sizeof(UniformBuffer))) } }
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
				Vec2u32{ 512, 512 }, GPUFormat::RGBA16f, GPUMemoryUsage::LOCAL, 1, 1
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
				Vec3u32{ 16, 16, 1 }
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
				ShaderAccess::VERTEX_FRAGMENT, uniforms->size()
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

				HashMap<ShaderStage, Buffer>{
					{ ShaderStage::VERTEX, vert },
					{ ShaderStage::FRAGMENT, frag }
				},

				pipelineLayout,
				MSAA(intermediate->getInfo().samples, .2f),
				DepthStencil::depth()

				//TODO:
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

			SetClearColor(Vec4f32(0.586f, 0.129f, 0.949f, 1.0f)),
			ClearFramebuffer(intermediate),
			BeginFramebuffer(intermediate),
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

		gui = new GUI(g, intermediate);

		//Release the graphics instance for us until we need it again

		g.pause();
	}

	~TestViewportInterface() {
		destroy(gui);
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

	//Helper functions

	void updateCamera() {
		UniformBuffer buffer = UniformBuffer::lookDirectionProj(res.cast<Vec2f32>().aspect(), eye);
		memcpy(uniforms->getBuffer(), &buffer, sizeof(UniformBuffer));
		uniforms->flush(0, sizeof(UniformBuffer));
	}

	//Update size of surfaces

	void resize(const ViewportInfo*, const Vec2u32 &size) final override {
		res = size;
		updateCamera();
		intermediate->onResize(size);
		gui->resize(size);
		swapchain->onResize(size);
	}

	//Update input

	void onInputUpdate(
		const ViewportInfo*, const InputDevice *dvc, InputHandle ih, bool isActive
	) final override {

		gui->onInputUpdate(dvc, ih, isActive);
	}

	//Execute commandList

	void render(const ViewportInfo*) final override {
		gui->render(g);
		g.present(intermediate, swapchain, cl, gui->getCommands());
	}

	//Update eye
	void update(const ViewportInfo *vi, f64 dt) final override {
	
		Vec3f32 d;

		for(auto *dvc : vi->devices)
			if (dvc->isType(InputDevice::KEYBOARD)) {

				//TODO: Y is flipped?

				if (dvc->isDown(Key::KEY_W)) d += Vec3f32(0, 0, -1);
				if (dvc->isDown(Key::KEY_S)) d += Vec3f32(0, 0, 1);

				if (dvc->isDown(Key::KEY_E)) d += Vec3f32(0, -1, 0);
				if (dvc->isDown(Key::KEY_Q)) d += Vec3f32(0, 1, 0);

				if (dvc->isDown(Key::KEY_D)) d += Vec3f32(1, 0, 0);
				if (dvc->isDown(Key::KEY_A)) d += Vec3f32(-1, 0, 0);

			}

		if (d.any()) {
			eye += d * f32(dt);
			updateCamera();
		}
	}
};

//Create window and wait for exit

int main() {

	Graphics g;
	TestViewportInterface viewportInterface(g);

	g.pause();

	System::viewportManager()->create(
		ViewportInfo(
			"Test window", {}, {}, 0, &viewportInterface, ViewportInfo::HANDLE_INPUT
		)
	);

	//TODO: Better way of stalling than this; like interrupt
	while (System::viewportManager()->size())
		System::wait(250_ms);

	return 0;
}