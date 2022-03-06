1. How to use your program: 
	LShift: move faster with WASD
	WS: move the camera forward/backward along its forward vector.
	AD: move the camera left/right along its side vector.
	QE: move the camera up/down along the global up vector (0,1,0).
	Click + Drag: the mouse should tilt the camera around its yaw and pitch.
	Up/Down: rotate the camera around the pitch.
	Left/right: rotate the camera around the yaw.
	T: change to different texture modes of the gBuffer or the final image
	Ctrl+R: Reload scene
	F5:  reload shaders
	The rest of the controls are controlled by imgui within the application

2. Important parts of the code: 
	- HBAO.cpp : in this file we have the implementation of the ambient occlusion pass

	-RenderManager.cpp/.h : In this file is where every important part from diferred shading is located, in functions like GenerateGBuffer where the
				gBuffer is created and the Update function where the all the Rendering passes such as the light pass, ambient pass, 
				bloom pass and other small passes are called. You can also find the load function where the gltf models are loaded 
				with the corresponding textures and vao. 
	
	-AmbientOcclusion.fs: shader that performs all the computations to compute the ambient occlusion for each pixel
	-BilateralGBlur.fs: shader performs a bilateral gaussian blur with 2 passes
	-gBuffer.vs/.fs : shader that fills the gBuffer
	-deferred_dierectional_shading.vs/.fs: shaders that compute the final color of the scene using taking into account the shadow with different parameters
	-Blur.fs : shader that given a texture blurries it into the current framebuffer texture
	-BloomScene.fs : shader that performs the final pass with the bloom texture and the scene texture to combine them, it also applies if needed gamma correction and 
			exposure.
3. Known issues and problems: 
