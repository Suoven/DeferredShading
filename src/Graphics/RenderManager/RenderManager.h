#pragma once
/*!
******************************************************************************
\*file   RenderManager.h
\author  Inigo Fernadez
\par     DP email: arenas.f@digipen.edu
\par     Course: CS250
\par     Assignment 2
\date	 10-2-2020

\brief
	this file contains the declaration of the render manager class which is used
	to manage the render of the objects, the image, the window and the texture of the sfml api

*******************************************************************************/

//#include "../Camera/Camera.h"	
#include <GL/glew.h>
#include <GL/GL.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <array>
#include <vector>
#include <unordered_map>

#include "../Camera/Camera.h"
#include "../../GameObject/GameObject.h"


class GLSLProgram;
struct ImGuiIO;

namespace tinygltf
{
	struct Texture;
}


class RenderManager
{
public:
	// -----------------------------------------------------------------------
	//static RenderManager & Instance() - static function used to use the RenderManager as a Singleton
	static RenderManager& Instance()
	{
		static RenderManager render_manager;
		return render_manager;
	}

	//load scene objects and default models
	void LoadDefaultModels();
	bool LoadScene();

	//functions used to intialize, update and shutdown the render manager
	void Initialize(const unsigned WindowWidth, const unsigned WindowHeight);

	//this function will update the render manager and draw all the renderables 
	void Update();

	//edit function that holds imgui editor paramters
	void Edit();

	//this function will be called at the end of the program to delete and finish everything
	void ShutDown();
	void UnLoadModels();
	void UnLoadModel(Model* model);
	void DestroyObjects();

	//function used to add a new renderable object to the render manager so it will get drawn
	void AddRenderable(GameObject* _renderable);

	//function used to add a new renderable object to the render manager so it will get drawn
	void DeleteRenderable(GameObject* _renderable);

	//function used to restart the shader programs
	void RestartShader();

	//load a model in gltf format providing the path to it
	Model* LoadModel(std::string path);

	//create a new light and returns it
	Light* CreateLight(Light::LIGTH_TYPE type, const glm::vec3& pos, const glm::vec3& scale, const glm::vec3& rot,
		const glm::vec3& color, float radius, const glm::vec3& dir = glm::vec3(1), bool visible = true);

	//remove the light provided from the lights list and renderable list if needed, it frees the memory
	void RemoveLight(Light* light);

	//take a screen shot of the scene
	void TakeScreenShot();

	// ------------------------------------------------------------------------
#pragma region // GETTERS & SETTERS

//set delete and get camera
	void AddCamera(Camera* cam) { mCamera = cam; }
	void DeleteCamera() { if (mCamera) delete mCamera; }
	Camera* GetCamera() { return mCamera; }

	SDL_Window* get_window() { return  window; }
	GLSLProgram* get_shader_program() { return  shaderProgram; }
	SDL_GLContext get_context() { return  context_; }
	int get_texture_handler(tinygltf::Texture texture, std::string name);

	void GetWindowSize(int& width, int& height) { SDL_GetWindowSize(window, &width, &height); }
#pragma endregion

	//file path to load and save
	std::string		load_path = { "./data/scenes/sceneAO.json" };
	std::string		screenshot_path = { "./data/screenshots/screenshot.png" };
	bool			takescreenshot = false;
	bool			draw_lights_model = false;
	int				texture_mode = 0;
	float           extended_view_size = 1.5f;
private:
	//camera of the level	
	Camera* mCamera = nullptr;

	//shaders
	GLSLProgram* shaderProgram = nullptr;
	GLSLProgram* ambientShaderProgram = nullptr;
	GLSLProgram* differedPointShadingProgram = nullptr;
	GLSLProgram* differedDirShadingProgram = nullptr;
	GLSLProgram* lightsShaderProgram = nullptr;
	GLSLProgram* BloomSceneProgram = nullptr;
	GLSLProgram* BlurSceneProgram = nullptr;
	GLSLProgram* BrightShaderProgram = nullptr;
	GLSLProgram* ShadowMapShaderProgram = nullptr;
	GLSLProgram* shadowRenderProg = nullptr;
	GLSLProgram* decalsShaderProgram = nullptr;
	GLSLProgram* AmbientOcclusionProgram = nullptr;
	GLSLProgram* BilateralBlurProgram = nullptr;

	//sdl imgui objects
	SDL_Window* window = nullptr;
	SDL_GLContext	context_ = nullptr;
	ImGuiIO* io = nullptr;

	//vector to store the models and renderables of the render manager
	Light* mDirectionalLight = nullptr;
	std::vector<GameObject*>				renderables;
	std::vector<Decal*>						decals;
	std::vector<Light*>						mLights;
	std::unordered_map<std::string, Model*>	mModels;
	std::vector<Texture>					mTextures;

	//default models
	Model* mQuad;
	Model* mSphere;
	Model* mCube;

	//Handlers
	GLuint						gBuffer, gPosition, gNormal, gDiffuse, gDepth, gDepthText;	//handlers to store gbuffer textures 
	GLuint						hdrFBO, hdrTexture, hdrDepth;								//handlers for the hdr texture 
	GLuint						blurFBO[2], blurTextures[2];								//handlers for the blur texture
	GLuint						brightFBO, brightTexture;									//handlers for the bright texture
	GLuint						decalsFBO, decalsDepth;										//handlers for the decals pass
	GLuint						AOFBO, AOText;												//handlers for the ambient oclusion
	std::vector<GLuint>			shadowmapsFBO, shadowmapsText;								//handlers for the cascaded shadow maps
	

	//lights
	bool	move_lights = true;
	float	lights_velocity = 0.1f;
	float   lights_intensity = 1.0f;
	float	ambient_intensity = 0.1f;
	float	lights_radius = 30.0f;
	int		lights_count = 0;

	//bloom
	bool	mb_use_bloom = false;
	int     blur_samples = 10;
	float   bright_threshold = 1.0f;

	//hdr
	bool mb_use_gamma = true;
	bool mb_use_exposure = true;
	float exposure = 2.0f;
	float gamma = 1.3f;
	float depth_contrast = 0.02f;

	//shadows
	int pcf_samples = 0;
	float occluder_max_distance = 50.0f;
	float shadow_bias = 0.00f;
	float cascade_linearity = 0.5f;
	float blend_distance = 0.0f;
	bool draw_shadow_maps = false;
	bool draw_cascade_levels = false;
	int	cascade_levels = 3;
	glm::ivec2 near_shadowmap_resolution = { 2048, 2048 };
	std::vector<float>			far_planes;
	std::vector<glm::mat4x4>	lightMtx;

	//decals
	float min_angle = 80.0f;
	int   decals_mode = 0;
	bool  draw_decals = true;

	//AO 
	bool draw_AO_texture = false;
	int dir_count = 5;
	int step_count = 10;
	float angle_bias = 30.0f;
	float ao_radious = 2.7f;
	float ao_scale = 4.0f;
	float ao_attenuation = 1.0f;
	float blur_threshold = 0.65f;
	int bilateral_blur_samples = 10;
	bool use_ambient_occlusion = true;
	int AO_texture_mode = 0;
	bool use_surface_normal = false;
	bool tangent_method = false;

	void UpdateLightsCount();
	void UpdateLightsMovement();
private:
	//private constructors to make the render a singleton
	RenderManager() = default;
	RenderManager(const RenderManager&) = default;
	
	//this function will generate the depth buffer
	void CreateDecalsBuffer();
	void GenerateGBuffer();
	void CreateHDRBrightTexture();
	void CreateBlurBuffers();
	void CreateShadowMaps();
	void DestroyShadowMaps();
	void CreateAOBuffer();

	//different passes in the 
	void DrawScene(GLSLProgram* shaderProg, bool use_custom_matrix = false, bool use_textures = true);
	void RenderQuad(GLSLProgram* program);

	//different passes in the pipeline
	void DrawShadowMaps();
	void AmbientPass();
	void PointLightPass();
	void DirLightPass();
	void BloomPass();
	void ComputeBrightness();
	void DrawLightsModels();
	void DrawDebugShadowMaps();
	void DecalsDrawPass();
	void AmbientOclusionPass();

	//function for shadows computation
	void ComputeShadowsMtx();
	std::vector<glm::vec4> ComputeFrustrumPoints(const glm::mat4& proj);
	glm::mat4 ComputeLightProjMtx(const float nearPlane, const float farPlane);

	//function called in the initalize to initialize the SDL window
	void InitWindow(const unsigned WindowWidth, const unsigned WindowHeight);

	//functions to set the uniforms of the lights properties and object materials
	void SetUniformLight(GLSLProgram* shader, const Light* light);
};

#define RenderMgr (RenderManager::Instance())



struct  Color
{
	glm::vec3 white		= { 1.0f, 1.0f, 1.0f };
	glm::vec3 red		= { 1.0f, 0.0f, 0.0f };
	glm::vec3 green		= { 0.0f, 1.0f, 0.0f };
	glm::vec3 blue		= { 0.0f, 0.0f, 1.0f };
	glm::vec3 yellow	= { 1.0f, 1.0f, 0.0f };
	glm::vec3 cyan		= { 0.0f, 1.0f, 1.0f };
	glm::vec3 pink		= { 1.0f, 0.0f, 1.0f };
	glm::vec3 colors[7] = { white, red, green, blue, yellow, cyan, pink };
};