#include "graphics/command/commands.hpp"
#include "helpers/graphics_object_ref.hpp"
#include "graphics/enums.hpp"
#include "graphics/graphics.hpp"
#include "system/viewport_manager.hpp"
#include "system/viewport_interface.hpp"
#include "system/local_file_system.hpp"
#include "gui/gui.hpp"
#include "gui/ui_window.hpp"
#include "types/mat.hpp"
#include "input/keyboard.hpp"
#include "input/mouse.hpp"
#include "utils/math.hpp"

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
	Vec3f32 eye{ 3, 3, 7 };
	f64 speed = 2;

	Vec3f32 cubePosition, cubeRotation, cubeScale;

	//TODO: Demonstrate multiple windows
	//TODO: Use render targets

	//Data on the GPU (test shader)
	struct UniformBuffer {
		Mat4x4f32 proj, view, world;
		Vec3f32 mask;
	};

	//Create resources

	TestViewportInterface(Graphics &g): g(g) {

		intermediate = {
			g, NAME("Framebuffer"),
			Framebuffer::Info(
				{ GPUFormat::RGBA8 }, DepthFormat::D32, false, 8
			)
		};

		//Create primitive buffer

		List<BufferAttributes> attrib{ { 0, GPUFormat::RGB32f }, { 1, GPUFormat::RG32f } };

		const List<Vec3f32> positionBuffer{

			//Bottom
			{ -1, -1, -1 }, { 1, -1, -1 },
			{ 1, -1, 1 },	{ -1, -1, 1 },

			//Top
			{ -1, 1, -1 },  { 1, 1, -1 },
			{ 1, 1, 1 },	{ -1, 1, 1 },

			//Back
			{ -1, -1, -1 }, { -1, 1, -1 },
			{ 1, 1, -1 },	{ 1, -1, -1 },

			//Front
			{ -1, -1, 1 },  { -1, 1, 1 },
			{ 1, 1, 1 },	{ 1, -1, 1 },

			//Left
			{ -1, -1, -1 }, { -1, -1, 1 },
			{ -1, 1, 1 },	{ -1, 1, -1 },

			//Right
			{ 1, -1, -1 },  { 1, -1, 1 },
			{ 1, 1, 1 },	{ 1, 1, -1 },
		};

		const List<Vec2f32> uvBuffer{

			//Bottom
			{ 0, 0 }, { 1, 0 },
			{ 1, 1 }, { 0, 1 },

			//Top
			{ 0, 1 }, { 1, 1 },
			{ 1, 0 }, { 0, 0 },

			//Back
			{ 0, 0 }, { 1, 0 },
			{ 1, 1 }, { 0, 1 },

			//Front
			{ 0, 1 }, { 1, 1 },
			{ 1, 0 }, { 0, 0 },

			//Left
			{ 0, 0 }, { 1, 0 },
			{ 1, 1 }, { 0, 1 },

			//Right
			{ 0, 1 }, { 1, 1 },
			{ 1, 0 }, { 0, 0 }
		};

		const List<u8> iboBuffer{
			0,3,2, 2,1,0,			//Bottom
			4,5,6, 6,7,4,			//Top
			8,11,10, 10,9,8,		//Back
			12,13,14, 14,15,12,		//Front
			16,19,18, 18,17,16,		//Left
			20,21,22, 22,23,20		//Right
		};

		mesh = {
			g, NAME("Test mesh"),
			PrimitiveBuffer::Info(
				{
					BufferLayout(positionBuffer, attrib[0]),
					BufferLayout(uvBuffer, attrib[1])
				},
				BufferLayout(iboBuffer, { 0, GPUFormat::R8u })
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
			BeginFramebuffer(intermediate),
			SetViewportAndScissor(),
			ClearFramebuffer(intermediate),

			//TODO: BeginRenderPass instead of BeginFramebuffer

			//Draw primitive

			BindPipeline(pipeline),
			BindDescriptors(descriptors),
			BindPrimitiveBuffer(mesh),
			DrawInstanced(mesh->elements()),

			//Present to surface

			EndFramebuffer()
		);

		//Setup GUI

		gui = new GUI(g, intermediate);
		gui->addWindow(new UIWindow("Test", 0, { 50, 50 }, { 200, 350 }, { 200, 350 }));
		gui->addWindow(new UIWindow("Test", 1, { 250, 250 }, { 200, 350 }, { 200, 350 }));

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

	void updateUniforms() {

		UniformBuffer buffer = {
			Mat4x4f32::perspective(f32(70_deg), res.cast<Vec2f32>().aspect(), 0.1f),
			Mat4x4f32::lookDirection(eye, { 0, 0, -1 }, { 0, 1, 0 }),
			Mat4x4f32::rotateZ(cubeRotation.x),								//TODO: Use transform instead
			{ 1, 1, 1 }
		};

		memcpy(uniforms->getBuffer(), &buffer, sizeof(UniformBuffer));
		uniforms->flush(0, sizeof(UniformBuffer));
	}

	//Update size of surfaces

	void resize(const ViewportInfo*, const Vec2u32 &size) final override {
		res = size;
		updateUniforms();
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

				if (dvc->isDown(Key::KEY_W)) d += Vec3f32(0, 0, -1);
				if (dvc->isDown(Key::KEY_S)) d += Vec3f32(0, 0, 1);

				if (dvc->isDown(Key::KEY_Q)) d += Vec3f32(0, -1, 0);
				if (dvc->isDown(Key::KEY_E)) d += Vec3f32(0, 1, 0);

				if (dvc->isDown(Key::KEY_D)) d += Vec3f32(1, 0, 0);
				if (dvc->isDown(Key::KEY_A)) d += Vec3f32(-1, 0, 0);

			} else if (dvc->isType(InputDevice::MOUSE)) {

				f64 delta = dvc->getCurrentAxis(MouseAxis::AXIS_WHEEL);

				if(delta)
					speed = oic::Math::clamp(speed * 1 + (delta / 1024), 0.5, 5.0);
			}

		if (d.any())
			eye += d * f32(dt * speed);

		cubeRotation += f32(5_deg * dt);
		
		updateUniforms();
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