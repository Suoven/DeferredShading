#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "../../Serializer/tiny_gltf.h"
#include "../../OpenGL/GLSLProgram.h"
#include "RenderManager.h"
#include <iostream>

//take a screen shot
void RenderManager::TakeScreenShot()
{
	//take a screenshot
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	int x = viewport[0];
	int y = viewport[1];
	int width = viewport[2];
	int height = viewport[3];

	std::vector<unsigned char> imageData(width * height * 4, 0);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadBuffer(GL_FRONT);
	glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());
	stbi_flip_vertically_on_write(true);
	stbi_write_png(screenshot_path.c_str(), width, height, 4, imageData.data(), 0);
}


//load default models
void RenderManager::LoadDefaultModels()
{
	//---------------------------------LOAD QUAD--------------------------
	Model* quad = new Model();
	Mesh quad_mesh;
	GLuint quadVAO, quadVBO;

	float quadVertices[] = {
		// positions        // texture Coords
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};
	// gen vao and vb
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);

	//Provide the data for the positions and texture coordinates
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

	//set the attributes location for use in the vertex shader
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	//add handlers to the model
	quad_mesh.VAOs.push_back(quadVAO);
	quad_mesh.VBOs.push_back(quadVBO);
	quad->mMeshes.push_back(quad_mesh);

	//add the quad to the list of models
	mModels["Quad"] = quad;
	mQuad = quad;

	//--------------------------LOAD SPHERE------------------------------
	//load the sphere
	mSphere = LoadModel("./data/gltf/Sphere.gltf");

	//--------------------------LOAD CUBE------------------------------
	//load the cube
	mCube = LoadModel("./data/gltf/Cube.gltf");
}

bool RenderManager::LoadScene()
{
	//open the scene json
	std::ifstream scene_file(load_path.c_str());
	if (!scene_file.is_open() || !scene_file.good()) return false;
	nlohmann::json j;

	//LOAD CAMERA
	scene_file >> j;
	if (j.find("camera") != j.end())
	{
		if (mCamera == nullptr)
			mCamera = new Camera();

		float far_plane = j["camera"]["far"];
		float near_plane = j["camera"]["near"];
		float fovy = j["camera"]["FOVy"];
		int width, height;
		SDL_GetWindowSize(window, &width, &height);
		mCamera->set_projection(fovy, glm::vec2(width, height), near_plane, far_plane);

		glm::vec3 pos;
		pos.x = j["camera"]["translate"]["x"];
		pos.y = j["camera"]["translate"]["y"];
		pos.z = j["camera"]["translate"]["z"];
		mCamera->set_position(pos);

		glm::vec3 rot;
		rot.x = j["camera"]["rotate"]["x"];
		rot.y = j["camera"]["rotate"]["y"];

		mCamera->rotate_around(glm::radians(rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec3 rightaxis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), mCamera->view_dir()));
		mCamera->rotate_around(glm::radians(rot.x), rightaxis);
	}

	//LOAD OBJECTS
	nlohmann::json& objects = *j.find("objects");
	for (auto it = objects.begin(); it != objects.end(); it++)
	{
		//get the path of the model
		nlohmann::json& val = *it;
		std::string path("./");
		std::string mesh_name = val["mesh"];
		path += mesh_name;

		//create a new object
		GameObject* new_object = new GameObject();
		new_object->mModel = LoadModel(path);

		glm::vec3 rot;
		rot.x = val["rotate"]["x"];
		rot.y = val["rotate"]["y"];
		rot.z = val["rotate"]["z"];

		new_object->RotateAroundVec(glm::vec3(0.0f, 0.0f, -1.0f), glm::radians(rot.z), glm::vec3(0));
		new_object->RotateAroundVec(glm::vec3(0.0f, 1.0f, 0.0f) , glm::radians(rot.y), glm::vec3(0));
		new_object->RotateAroundVec(glm::vec3(1.0f, 0.0f, 0.0f) , glm::radians(rot.x), glm::vec3(0));

		new_object->mPosition.x = val["translate"]["x"];
		new_object->mPosition.y = val["translate"]["y"];
		new_object->mPosition.z = val["translate"]["z"];

		new_object->mScale.x = val["scale"]["x"];
		new_object->mScale.y = val["scale"]["y"];
		new_object->mScale.z = val["scale"]["z"];

		AddRenderable(new_object);
	}

	//LOAD POINT LIGHTS
	lights_count = 200;
	UpdateLightsCount();
	
	//LOAD DIRECTIONAL LIGHT
	if (j.find("directional_light") != j.end())
	{
		glm::vec3 color;
		glm::vec3 dir;

		dir.x = j["directional_light"]["direction"]["x"];
		dir.y = j["directional_light"]["direction"]["y"];
		dir.z = j["directional_light"]["direction"]["z"];

		color.x = j["directional_light"]["color"]["x"];
		color.y = j["directional_light"]["color"]["y"];
		color.z = j["directional_light"]["color"]["z"];

		mDirectionalLight = new Light(Light::LIGTH_TYPE::DIRECTIONAL, glm::vec3(0), glm::vec3(1.0f), glm::vec3(0), dir);
		mDirectionalLight->mStats.m_color = color;
		mDirectionalLight->mModel = mSphere;
	}

	//LOAD DECALS
	if (j.find("decals") != j.end())
	{
		nlohmann::json& json_decals = *j.find("decals");
		for (auto it = json_decals.begin(); it != json_decals.end(); it++)
		{
			//get the path of the model
			nlohmann::json& val = *it;
			Decal* new_decal = new Decal;

			glm::vec3 rot;
			rot.x = val["rotate"]["x"];
			rot.y = val["rotate"]["y"];
			rot.z = val["rotate"]["z"];

			new_decal->RotateAroundVec(glm::vec3(0.0f, 0.0f, -1.0f), glm::radians(rot.z), glm::vec3(0));
			new_decal->RotateAroundVec(glm::vec3(0.0f, 1.0f, 0.0f), glm::radians(rot.y), glm::vec3(0));
			new_decal->RotateAroundVec(glm::vec3(1.0f, 0.0f, 0.0f), glm::radians(rot.x), glm::vec3(0));

			//set the world parameters of the decal
			new_decal->mPosition.x = val["translate"]["x"];
			new_decal->mPosition.y = val["translate"]["y"];
			new_decal->mPosition.z = val["translate"]["z"];

			new_decal->mScale.x = val["scale"]["x"];
			new_decal->mScale.y = val["scale"]["y"];
			new_decal->mScale.z = val["scale"]["z"];

			//set the texture values
			const unsigned texture_count = 3;
			std::string texture_names[texture_count] = { "diffuse", "normal", "metallic" };
			GLuint texture_handlers[texture_count];
			glGenTextures(3, texture_handlers);

			//iterate throgh the textures
			for (unsigned i = 0; i < texture_count; i++)
			{
				//bind the texture
				glBindTexture(GL_TEXTURE_2D, texture_handlers[i]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				//load the texture
				std::string substr = "./";
				std::string path = val[texture_names[i].c_str()];
				path = substr + path;
				SDL_Surface* surface = IMG_Load(path.c_str());
				if (!surface)
					continue;

				int pixelFormat = GL_RGB;
				if (surface->format->BytesPerPixel == 4)
					pixelFormat = GL_RGBA;

				// Give pixel data to opengl and free surface because OpenGL already has the data
				glTexImage2D(GL_TEXTURE_2D, 0, pixelFormat, surface->w, surface->h, 0, pixelFormat, GL_UNSIGNED_BYTE, surface->pixels);
				glGenerateMipmap(GL_TEXTURE_2D);
				SDL_FreeSurface(surface);

				//set the texture handler to the decal
				new_decal->textures[texture_names[i]] = texture_handlers[i];
			}
			//store the new decal
			decals.push_back(new_decal);
		}
	}

	//close file
	scene_file.close();
	return true;
}



Model* RenderManager::LoadModel(std::string path)
{
	//check if the model is alredy loaded
	auto find = mModels.find(path);
	if (find != mModels.end())
		return find->second;

	//variables to load the model
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	//load the model
	bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);

	if (!warn.empty())
		printf("Warn: %s\n", warn.c_str());
	if (!err.empty())
		printf("Err: %s\n", err.c_str());
	if (!ret) {
		printf("Failed to parse glTF\n");
		return nullptr;
	}

	//load all the textures for the model
	for (auto& texture : model.textures)
	{
		GLuint texture_handler;
		tinygltf::Sampler mSampler = model.samplers[texture.sampler];
		tinygltf::Image mImage = model.images[texture.source];

		//check if the texture is already created
		if (get_texture_handler(texture, mImage.name) != -1) continue;

		//bind the current texture
		glGenTextures(1, &texture_handler);
		glBindTexture(GL_TEXTURE_2D, texture_handler);

		//set the paramters of the texture
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mSampler.magFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mSampler.minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mSampler.wrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mSampler.wrapT);

		// Give pixel data to opengl
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mImage.width, mImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mImage.image.data());
		glGenerateMipmap(GL_TEXTURE_2D);

		//add the texture handler to the vector
		mTextures.push_back(Texture{ mImage.name, texture.sampler , texture.source , texture_handler});
	}

	Model* new_model = new Model;
	const unsigned Attribute_count = 4;
	std::string attributes_names[Attribute_count] = { "POSITION" ,"NORMAL" ,"TANGENT" , "TEXCOORD_0" };
	for (auto& node : model.nodes)
	{
		//create a new mesh
		Mesh new_mesh;
		if (!node.scale.empty())
			new_mesh.mScale = { node.scale[0],  node.scale[1], node.scale[2] };
		if (!node.translation.empty())
			new_mesh.mPosition = { node.translation[0],  node.translation[1], node.translation[2] };

		glm::vec3 rot = glm::vec3(0);
		if (node.rotation.size() == 3)
			rot = glm::vec3(node.rotation[0], node.rotation[1], node.rotation[2]);
		else if (node.rotation.size() == 4)
		{
			glm::vec4 quat;
			quat.x = static_cast<float>(node.rotation[0]); 
			quat.y = static_cast<float>(node.rotation[1]); 
			quat.z = static_cast<float>(node.rotation[2]); 
			quat.w = static_cast<float>(node.rotation[3]); 

			glm::fquat Q(quat.w, quat.x, quat.y, quat.z);
			rot = glm::eulerAngles(Q) * 180.0f / PI;
		}

		new_mesh.RotateAroundVec(glm::vec3(0.0f, 0.0f, -1.0f), glm::radians(rot.z), glm::vec3(0));
		new_mesh.RotateAroundVec(glm::vec3(0.0f, 1.0f, 0.0f), glm::radians(rot.y), glm::vec3(0));
		new_mesh.RotateAroundVec(glm::vec3(1.0f, 0.0f, 0.0f), glm::radians(rot.x), glm::vec3(0));

		//security check
		if (node.mesh < 0) continue;

		//iterate through all the primitives of the mesh
		for (auto& primitive : model.meshes[node.mesh].primitives)
		{
			//create the handlers
			std::array<GLuint, Attribute_count> VBOs;
			GLuint new_VAO;
			GLuint idx_VBO;

			//generate and bind a vao
			glGenVertexArrays(1, &new_VAO);
			glBindVertexArray(new_VAO);

			//get the buffer view of the of the indices and add the number of indices to the model
			int indices_idx = primitive.indices;
			tinygltf::BufferView& indices_BVidx = model.bufferViews[model.accessors[indices_idx].bufferView];
			new_mesh.idx_counts.push_back(static_cast<unsigned>(model.accessors[indices_idx].count));
			new_mesh.idx_type.push_back(static_cast<unsigned>(model.accessors[indices_idx].componentType));

			// Generate the index buffer and fill it with data
			glGenBuffers(1, &idx_VBO);
			glBindBuffer(indices_BVidx.target, idx_VBO);
			glBufferData(indices_BVidx.target, indices_BVidx.byteLength, model.buffers[indices_BVidx.buffer].data.data() + indices_BVidx.byteOffset, GL_STATIC_DRAW);

			//generate the biffers for the attributes
			glGenBuffers(Attribute_count, VBOs.data());

			//generate buffers for all the atributtes of the vertex and fill them
			for (unsigned i = 0; i < Attribute_count; i++)
			{
				//get the index and the buffer view of the attribute
				int attr_idx = primitive.attributes.find(attributes_names[i])->second;
				tinygltf::BufferView& attrib_BV = model.bufferViews[model.accessors[attr_idx].bufferView];

				// Bind new VBO and Bind a buffer of vertices
				glBindBuffer(GL_ARRAY_BUFFER, VBOs[i]);

				// Give our vertices to OpenGL.
				glBufferData(attrib_BV.target, attrib_BV.byteLength, model.buffers[attrib_BV.buffer].data.data() + attrib_BV.byteOffset, GL_STATIC_DRAW);
				glEnableVertexAttribArray(i);
				glVertexAttribPointer(i, model.accessors[attr_idx].type, model.accessors[attr_idx].componentType, model.accessors[attr_idx].normalized, static_cast<GLsizei>(attrib_BV.byteStride), nullptr);

				//add the new vbo
				new_mesh.VBOs.push_back(VBOs[i]);
			}

			//Unbind buffer
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);

			//set the vao and vbos of the model
			new_mesh.VAOs.push_back(new_VAO);
			new_mesh.idx_VBOs.push_back(idx_VBO);

			//check if the model has materials
			if (primitive.material < 0) continue;

			//get the material 
			tinygltf::Material material = model.materials[primitive.material];

			//store the handler of the normal texture
			int idx = material.normalTexture.index;
			if(idx < 0)		new_mesh.normal_texture.push_back(idx);
			else			new_mesh.normal_texture.push_back(get_texture_handler(model.textures[idx], model.images[model.textures[idx].source].name));

			//store the handler of the diffuse color
			idx = material.pbrMetallicRoughness.baseColorTexture.index;
			if (idx < 0)	new_mesh.diffuse_texture.push_back(idx);
			else			new_mesh.diffuse_texture.push_back(get_texture_handler(model.textures[idx], model.images[model.textures[idx].source].name));

			//store the handler of the specular color
			idx = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
			if (idx < 0)	new_mesh.specular_texture.push_back(idx);
			else			new_mesh.specular_texture.push_back(get_texture_handler(model.textures[idx], model.images[model.textures[idx].source].name));

			//store the ambient color
			new_mesh.DiffuseColor.push_back(glm::vec4{ (float)material.pbrMetallicRoughness.baseColorFactor[0], (float)material.pbrMetallicRoughness.baseColorFactor[1],
												  (float)material.pbrMetallicRoughness.baseColorFactor[2], (float)material.pbrMetallicRoughness.baseColorFactor[3] });
		}
		new_model->mMeshes.push_back(new_mesh);
	}
	mModels[path] = (new_model);
	return new_model;
}

//unload all the vao and vbo
void RenderManager::UnLoadModel(Model* model)
{
	//iterate through the mesh
	for (auto& mesh : model->mMeshes)
	{
		//delete the vbos
		for (auto& vao : mesh.VAOs)
			glDeleteBuffers(1, &vao);

		//delete the vbos
		for (auto& vbo : mesh.VBOs)
			glDeleteBuffers(1, &vbo);

		//delete the idx buffers
		for (auto& idx_vbo : mesh.idx_VBOs)
			glDeleteBuffers(1, &idx_vbo);
	}
}

//get the texture form the textures already loaded
int RenderManager::get_texture_handler(tinygltf::Texture texture, std::string name)
{
	for (auto& mText : mTextures)
	{
		//check if the sampler and image is the same so its the same texture
		if (mText.sampler_idx == texture.sampler && mText.image_idx == texture.source && mText.name == name)
			return mText.texture_handler;
	}

	//return -1 if the texture is not found
	return -1;
}