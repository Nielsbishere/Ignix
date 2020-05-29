#pragma once
#include "helpers/graphics_object_ref.hpp"
#include "utils/hash.hpp"
#include "window_container.hpp"
#include "system/viewport_manager.hpp"

namespace oic {
	class InputDevice;
	using InputHandle = u32;
}

namespace igx::ui {

	//All info needed on the GPU
	struct GUIInfo {
		Vec2i32 res, pos;
		Vec2f32 whiteTexel;		//uv position of a pure "white" texel (so no subpixel filtering is applied)
	};

	//Renders GUI into a framebuffer
	//You create the GUI through this interface and pass this final UI to the present
	//
	class GUI : public WindowContainer {

	public:

		enum Flags : u8 {
			NONE = 0,							//Clears framebuffer and will only redraw when required
			OWNS_FRAMEBUFFER = 1,				//Whether the UI owns the framebuffer; determines if it should clear and resize itself
			OWNS_COMMAND_LIST = 2,				//Whether the UI owns the command list; (if it should be cleared 
			DISABLE_SUBPIXEL_RENDERING = 4		//Whether subpixel rendering should be disabled
		};

		static constexpr u32 msaa = 4;

	private:

		GUIInfo info;

		List<oic::Monitor> monitors;

		BufferAttributes vertexLayout = { 0, GPUFormat::RG32f, GPUFormat::RG32f, GPUFormat::RGBA8 };	//vec2 pos, vec2 uv, vec4un8 color

		PipelineLayoutRef pipelineLayout;

		SetClearColor clearColor{};

		List<TextureRef> textures;

		DescriptorsRef descriptors;
		FramebufferRef target;

		CommandListRef commands;
		SamplerRef sampler;

		PipelineRef uiShader;
		GPUBufferRef guiDataBuffer, guiMonitorBuffer;
		UploadBufferRef uploadBuffer;

		Graphics *graphics;

		struct Data;
		Data *data{};						//Implementation dependent data
		usz commandListSize{};

		Flags flags;

	protected:

		bool needsBufferUpdate = true, requestedUpdate = true;

		void bakePrimitives(Graphics &g);	//Fills vertex/index buffer
		bool prepareDrawData();				//Returns true if it should bake primitive data
		void renderWindows(List<Window*> &windows);
		void draw();						//Called to fill the command list

		void initData(Graphics &g);			//Init implementation dependent data and descriptors/textures

		void beginDraw();					//Prepares for draw start
		void endDraw();						//Prepares for draw end

	private:

		void init(Graphics &g);

	public:

		GUI(const GUI&) = delete;
		GUI(GUI&&) = delete;
		GUI &operator=(const GUI&) = delete;
		GUI &operator=(GUI&&) = delete;

		~GUI();

		//Create UI with its own framebuffer (Flags::NONE)
		GUI(Graphics &g, const SetClearColor &clearColor = Vec4f32(), usz commandListSize = 4_MiB);

		//Creates UI with a managed framebuffer
		GUI(Graphics &g, const FramebufferRef &fb, usz commandListSize = 4_MiB);

		//Creates UI with a managed command list
		GUI(Graphics &g, const CommandListRef &cl);

		//Creates UI with a managed framebuffer and command list
		GUI(Graphics &g, const FramebufferRef &fb, const CommandListRef &cl);

		//Resize the UI's framebuffer (required, even for managed framebuffers)
		void resize(const Vec2u32 &target);

		//Requests the UI to redraw next draw
		void requestUpdate();

		//Sends input information to the UI
		//returns true if the input was accepted
		bool onInputUpdate(const oic::InputDevice*, oic::InputHandle, bool isActive);

		//Should be called per frame, but doesn't necessarily always render
		//"offset" is the 
		void render(Graphics &g, const Vec2i32 &offset, const List<oic::Monitor> &monitors);

		//Get the command list (only needed if it has its own)
		inline const CommandListRef &getCommands() const { return commands; }

		//Get the framebuffer (only needed if it has its own)
		inline const FramebufferRef &getFramebuffer() const { return target; }

		//Get the UI flags
		inline Flags getFlags() const { return flags; }

	};

}