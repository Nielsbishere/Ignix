#include "graphics/command/commands.hpp"
#include "helpers/common.hpp"
#include "graphics/enums.hpp"
#include "graphics/graphics.hpp"
#include "system/viewport_manager.hpp"
#include "system/viewport_interface.hpp"
#include "system/local_file_system.hpp"
#include "gui/gui.hpp"
#include "gui/window.hpp"
#include "gui/struct_inspector.hpp"
#include "types/mat.hpp"
#include "input/keyboard.hpp"
#include "input/mouse.hpp"
#include "utils/math.hpp"

using namespace igx::ui;
using namespace igx;
using namespace oic;

oicExposedEnum(BiomeType, u8, Large_biome, Small_biome);
oicExposedEnum(Difficulty, u8, Easy, Normal, Hard, Hardcore);

struct TestViewportInterface : public ViewportInterface {

	Graphics &g;
	GUI *gui;

	//Resources

	SwapchainRef swapchain;
	FramebufferRef intermediate;
	CommandListRef cl;
	PrimitiveBufferRef mesh;
	DescriptorsRef descriptors, computeDescriptors;
	PipelineLayoutRef pipelineLayout, computePipelineLayout;
	PipelineRef pipeline, computePipeline;
	ShaderBufferRef uniforms;
	UploadBufferRef uploadBuffer;
	TextureRef tex2D, computeOutput;
	SamplerRef samp;

	Vec2u32 res;
	Vec3f32 eye{ 3, 3, 7 };
	f64 speed = 2;

	Vec3f32 cubePosition = { 2, 2, 2 }, cubeRotation, cubeScale = { 1, 0.5f, 1 };

	static constexpr u8 msaa = 8;

	//TODO: Demonstrate multiple windows
	//TODO: Use render targets

	//Data on the GPU (test shader)
	struct UniformBuffer {
		Mat4x4f32 pvw;
		Vec3f32 mask;
	};

	//Testing sliders and everything
	
	struct TestStruct0 {

		inline void click() const {
			oic::System::log()->debug("Play");
		}

		RadioButtons<Difficulty> difficulty;
		bool silver, bronze, gold = true;
		Dropdown<BiomeType> biomeType = BiomeType::Small_biome;

		//MinMaxSliderBase<f32, 0, 1> Volume = .5f;
		//MinMaxProgress<f32, 0, 100, 0.1> Progress;

		InflectWithName(
			{ "Play", "Difficulty", "Silver", "Bronze", "Gold", "Biome type" }, 
			Button<TestStruct0, &TestStruct0::click>{},
			difficulty,
			silver,
			bronze,
			gold,
			biomeType
		);
	};

	struct TestStruct1 {

		//Progress<u8, 4> Compressed_chunk_width, Compressed_chunk_height;

		bool Optimize_size, Optimize_quality;

		Dropdown<Key> Keyboard_shortcut;
		Dropdown<MouseButton> Mouse_shortcut;

		oic::FileSystem *Files = oic::System::files();

		List<TestStruct0> Games{ {}, {} };

		HashMap<String, u32> Testing {
			{ "Testing", 123 },
			{ "Something else", 000 }
		};

		u8 Test = 11;
		u8 Test_u8 = u8_MAX;
		i8 Test_i8 = i8_MIN;
		u16 Test_u16 = u16_MAX;
		i16 Test_i16 = i16_MIN;
		u32 Test_u32 = u32_MAX;
		i32 Test_i32 = i32_MIN;
		u64 Test_u64 = u64_MAX;
		i64 Test_i64 = i64_MIN;

		Inflect(
			/*Compressed_chunk_width, Compressed_chunk_height,*/ 
			Optimize_size, Optimize_quality, 
			Files,
			Test, Test_u8, Test_i8, Test_u16, Test_i16,
			Test_u32, Test_i32, Test_u64, Test_i64,
			Keyboard_shortcut, Mouse_shortcut,
			Games, Testing
		);
	};

	//TODO: Fix leak!
	//TODO: Some crash
	//TODO: Format names

	using StructInspector0 = StructInspector<TestStruct0>;
	using StructInspector1 = StructInspector<TestStruct1>;

	StructInspector0 structInspector0;
	StructInspector1 structInspector1;

	//Create resources

	TestViewportInterface(Graphics &g): g(g) {

		intermediate = {
			g, NAME("Framebuffer"),
			Framebuffer::Info(
				{ GPUFormat::srgba8 }, DepthFormat::D32, false, msaa
			)
		};

		//Create primitive buffer

		List<BufferAttributes> attrib{ { 0, GPUFormat::rgb32f }, { 1, GPUFormat::rg32f } };

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

		const List<u16> iboBuffer{
			0,1,2, 2,3,0,			//Bottom
			4,7,6, 6,5,4,			//Top
			8,9,10, 10,11,8,		//Back
			12,15,14, 14,13,12,		//Front
			16,17,18, 18,19,16,		//Left
			20,23,22, 22,21,20		//Right
		};

		mesh = {
			g, NAME("Test mesh"),
			PrimitiveBuffer::Info(
				{
					BufferLayout(positionBuffer, attrib[0]),
					BufferLayout(uvBuffer, attrib[1])
				},
				BufferLayout(iboBuffer, { 0, GPUFormat::r16u })
			)
		};

		//Create uniform buffer

		uniforms = {
			g, NAME("Test pipeline uniform buffer"),
			ShaderBuffer::Info(
				GPUBufferType::UNIFORM, GPUMemoryUsage::SHARED | GPUMemoryUsage::CPU_WRITE,
				{ { NAME("mask"), ShaderBuffer::Layout(0, Buffer(sizeof(UniformBuffer))) } }
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
				GPUFormat::rgba8, GPUMemoryUsage::LOCAL, 2
			)
		};

		//Create sampler

		samp = { g, NAME("Test sampler"), Sampler::Info() };

		//Create compute target

		computeOutput = {
			g, NAME("Compute output"),
			Texture::Info(
				Vec2u16{ 512, 512 }, GPUFormat::rgba16f, GPUMemoryUsage::GPU_WRITE, 1, 1
			)
		};

		//Load shader code
		//(Compute output 512x512)

		//Create layout for compute

		computePipelineLayout = {
			g, NAME("Compute pipeline layout"),
			PipelineLayout::Info(
				RegisterLayout(
					NAME("Output"), 0,
					TextureType::TEXTURE_2D, 0, 0,
					ShaderAccess::COMPUTE, GPUFormat::rgba16f, true
				)
			)
		};

		auto descriptorsInfo = Descriptors::Info(computePipelineLayout, 0, {});
		descriptorsInfo.resources[0] = { computeOutput, TextureType::TEXTURE_2D };

		computeDescriptors = {
			g, NAME("Compute descriptors"),
			descriptorsInfo
		};

		//Create pipeline (shader and render states)

		computePipeline = {
			g, NAME("Compute pipeline"),
			Pipeline::Info(
				Pipeline::Flag::NONE,
				VIRTUAL_FILE("shaders/test.comp.spv"),
				{},
				computePipelineLayout,
				Vec3u32{ 16, 16, 1 }
			)
		};

		//Create descriptors that should be bound

		pipelineLayout = {

			g, NAME("Graphics pipeline layout"),

			PipelineLayout::Info(

				RegisterLayout(
					NAME("Test"), 0,
					GPUBufferType::UNIFORM, 0, 0,
					ShaderAccess::VERTEX_FRAGMENT, uniforms->size()
				),

				RegisterLayout(
					NAME("test"), 1,
					SamplerType::SAMPLER_2D, 0, 0,
					ShaderAccess::FRAGMENT
				)
			)
		};

		descriptorsInfo = Descriptors::Info(pipelineLayout, 0, {});
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

				Pipeline::Flag::NONE,

				attrib,

				{},

				HashMap<ShaderStage, Pair<String, String>>{
					{ ShaderStage::VERTEX, { VIRTUAL_FILE("shaders/test.vert.spv"), "main" } },
					{ ShaderStage::FRAGMENT, { VIRTUAL_FILE("shaders/test.frag.spv"), "main" } }
				},

				pipelineLayout,
				MSAA(intermediate->getInfo().samples, .2f),
				DepthStencil::depth()
			)
		};

		//Submit our resources to the GPU

		uploadBuffer = {
			g, NAME("Upload buffer"),
			UploadBuffer::Info(64_KiB, 0, 0)		//Only use 64_KiB, don't allow shrinking/growing
		};
		
		cl = { g, NAME("Command list"), CommandList::Info(2_KiB) };

		//Create command list and store our commands

		cl->add(

			//Upload mesh and texture if changes occur (or init time)
			FlushImage(tex2D, uploadBuffer),
			FlushBuffer(mesh, uploadBuffer),

			//Update uniforms
			FlushBuffer(uniforms, nullptr),		//Tell when we want our CPU data to be copied

			//Render to compute shader

			BindPipeline(computePipeline),
			BindDescriptors(computeDescriptors),
			Dispatch(computeOutput->getInfo().dimensions.cast<Vec3u32>()),
			
			//Clear and bind MSAA

			SetClearColor(Vec4f32(0.586f, 0.129f, 0.949f, 1.0f)),
			BeginFramebuffer(intermediate),
			SetViewportAndScissor(),
			ClearFramebuffer(),

			//TODO: BeginRenderPass instead of BeginFramebuffer

			//Draw primitive

			BindPipeline(pipeline),
			BindDescriptors(descriptors),
			BindPrimitiveBuffer(mesh),
			DrawInstanced::indexed(mesh->elements()),

			//Present to surface

			EndFramebuffer()
		);

		//Setup GUI

		gui = new GUI(g, intermediate);
		
		gui->addWindow(Window("", 0, {}, { 200, 350 }, &structInspector0, Window::STATIC_NO_MENU));
		gui->addWindow(Window("Test", 1, { 50, 50 }, { 200, 350 }, &structInspector1));

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

		g.resume();		//This thread can now interact with graphics

		swapchain = {
			g, NAME("Swapchain"),
			Swapchain::Info{ vp, false }
		};
	}

	//Delete viewport resources

	void release(const ViewportInfo*) final override {
		swapchain.release();
	}

	//TODO: Fix flickering on resize
	//TODO: Fix crash on exit
	//TODO: Every execution should copy that command buffer, to ensure resources aren't removed
	//TODO: Remove the name restriction, since resources can be destroyed on one thread but created on another

	//Update size of surfaces

	void resize(const ViewportInfo*, const Vec2u32 &size) final override {

		res = size;

		intermediate->onResize(size);
		gui->resize(size);
		swapchain->onResize(size);
	}

	//Update input

	void onInputUpdate(
		ViewportInfo*, const InputDevice *dvc, InputHandle ih, bool isActive
	) final override {

		gui->onInputUpdate(dvc, ih, isActive);
	}

	//Execute commandList

	void render(const ViewportInfo *vi) final override {
		gui->render(g, vi->offset, vi->monitors);
		g.present(intermediate, swapchain, cl, gui->getCommands());
	}

	//Update eye
	void update(const ViewportInfo *vi, f64 dt) final override {
	
		Vec3f32 d;

		for(auto *dvc : vi->devices)
			if (dvc->isType(InputDevice::KEYBOARD)) {

				if (dvc->isDown(Key::Key_w)) d += Vec3f32(0, 0, -1);
				if (dvc->isDown(Key::Key_s)) d += Vec3f32(0, 0, 1);

				if (dvc->isDown(Key::Key_q)) d += Vec3f32(0, -1, 0);
				if (dvc->isDown(Key::Key_e)) d += Vec3f32(0, 1, 0);

				if (dvc->isDown(Key::Key_d)) d += Vec3f32(1, 0, 0);
				if (dvc->isDown(Key::Key_a)) d += Vec3f32(-1, 0, 0);

			} else if (dvc->isType(InputDevice::MOUSE)) {

				f64 delta = dvc->getCurrentAxis(MouseAxis::Axis_wheel_y);

				if(delta)
					speed = oic::Math::clamp(speed * 1 + (delta / 1024), 0.5, 5.0);
			}

		if (d.any())
			eye += d * f32(dt * speed);

		cubeRotation += f32(5_deg * dt);
		
		if (res.neq(0).all()) {

			auto p = Mat4x4f32::perspective(f32(70_deg), res.cast<Vec2f32>().aspect(), 0.1f, 100.f);
			auto v = Mat4x4f32::lookDirection(eye, { 0, 0, -1 }, { 0, 1, 0 });
			auto w = Mat4x4f32::transform(cubePosition, cubeRotation, cubeScale);

			UniformBuffer buffer = {
				p * v * w,
				{ 1, 1, 1 }
			};

			memcpy(uniforms->getBuffer(), &buffer, sizeof(UniformBuffer));
			uniforms->flush(0, sizeof(UniformBuffer));		//Notify driver that we need an update
		}
	}
};

//Create window and wait for exit

int main() {

	const String appName = "Igx test window";
	constexpr u32 appVersion = 1;

	Graphics g(
		appName,
		appVersion,
		"Igx",
		1
	);

	TestViewportInterface viewportInterface(g);

	g.pause();

	System::viewportManager()->create(
		ViewportInfo(
			appName, {}, {}, 0, &viewportInterface, ViewportInfo::HANDLE_INPUT
		)
	);

	System::viewportManager()->waitForExit();

	return 0;
}