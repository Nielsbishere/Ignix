#pragma once
#include "helpers/graphics_object_ref.hpp"
#include "utils/hash.hpp"
//#include "ui_window.hpp"

namespace oic {
	class InputDevice;
	using InputHandle = u32;
}

namespace igx {

	class UIWindow;

	//Renders GUI into a framebuffer
	//You create the GUI through this interface and pass this final UI to the present
	//
	class GUI {

	public:

		enum Flags : u8 {
			NONE = 0,						//Clears framebuffer and will only redraw when required
			OWNS_FRAMEBUFFER = 1,			//Whether the UI owns the framebuffer; determines if it should clear and resize itself
			OWNS_COMMAND_LIST = 2			//Whether the UI owns the command list; (if it should be cleared 
		};

		static constexpr u32 msaa = 8;

	private:

		BufferAttributes vertexLayout = { 0, GPUFormat::RG32f, GPUFormat::RG32f, GPUFormat::RGBA8 };	//vec2 pos, vec2 uv, vec4un8 color

		PipelineLayout pipelineLayout = {
			RegisterLayout(NAME("Input texture"), 0, SamplerType::SAMPLER_2D, 0, ShaderAccess::FRAGMENT),
			RegisterLayout(NAME("Res buffer"), 1, GPUBufferType::UNIFORM, 0, ShaderAccess::VERTEX, 8)
		};

		SetClearColor clearColor{};

		List<UIWindow*> windows;

		List<Texture> textures;

		Descriptors descriptors;
		Framebuffer target;

		CommandList commands;
		Sampler sampler;

		Pipeline uiShader;
		GPUBuffer resolution;

		struct Data;
		Data *data{};						//Implementation dependent data
		usz commandListSize{};

		Flags flags;
		bool
			couldRefresh = true,			//If it could refresh (e.g. mouse movement; recommend refresh)
			shouldRefresh = true;			//If it should refresh (e.g. resizes; force refresh)

	protected:

		void bakePrimitives(Graphics &g);	//Fills vertex/index buffer
		bool prepareDrawData();				//Returns true if it should bake primitive data
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
		GUI(Graphics &g, const Framebuffer &fb, usz commandListSize = 4_MiB);

		//Creates UI with a managed command list
		GUI(Graphics &g, const CommandList &cl);

		//Creates UI with a managed framebuffer and command list
		GUI(Graphics &g, const Framebuffer &fb, const CommandList &cl);

		//Resize the UI's framebuffer (required, even for managed framebuffers)
		void resize(const Vec2u32 &target);

		//Requests the UI to redraw next draw
		void requestUpdate();

		//Sends input information to the UI
		//returns true if the input was accepted
		bool onInputUpdate(const oic::InputDevice*, oic::InputHandle, bool isActive);

		//Should be called per frame, but doesn't necessarily always render
		void render(Graphics &g);

		//Get the command list (only needed if it has its own)
		inline const CommandList &getCommands() const { return commands; }

		//Get the framebuffer (only needed if it has its own)
		inline const Framebuffer &getFramebuffer() const { return target; }

		//Get the UI flags
		inline Flags getFlags() const { return flags; }

		//Windows

		//Relinquish ownership over window and give it to this UI
		inline bool addWindow(UIWindow *window) {

			auto it = std::find(windows.begin(), windows.end(), window);

			if (it != windows.end())
				return false;

			windows.push_back(window);
			return true;
		}

		//Reclaim ownership over window and remove it from the UI (no deletion)
		inline bool removeWindow(UIWindow *window) {
			
			auto it = std::find(windows.begin(), windows.end(), window);

			if (it == windows.end())
				return false;

			windows.erase(it);
			return true;
		}

		//Delete the window and remove it from the UI
		void deleteWindow(UIWindow *window);

		inline usz size() const { return windows.size(); }

		inline UIWindow **begin() { return windows.data(); }
		inline UIWindow **end() { return begin() + size(); }

		inline UIWindow *operator[](usz i) { return begin()[i]; }

	};

}