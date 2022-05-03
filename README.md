# Differed Shading Pipeline
Deferred shading pipeline that allows for multiple lights, and enhanced with different post processing effects like Bloom, Decals, Horizontal Ambient Occlusion, HDR.

![This is a alt text.](/OctreeImplementation/resources/octree.gif "Octree in action")

# How to use program: 
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
