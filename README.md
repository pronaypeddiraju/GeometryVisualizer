# GeometryVisualizer
Geometry Visualization and raycast vs space

Instructions to get and build project:
- Visit the project repository at : https://github.com/pronaypeddiraju/GeometryVisualizer.git
- Download the files from the repository (clone)
- Compile the GeometryVisualizer2D.sln solution file to view and modify code
- Navigate to the Run folder and execute the GeometryVisualizer2D.exe file to run the project

Features:

- Ability to increase/Decrease the number of polygons in scene
- Ability to increase/Decrease the number of raycasts in the scene
- Functionality to enable/disable the mid-phase bit bucket optimization
- UI to manipulate all of the said functionality

Retrospectives:

What went well:
- Very quick to setup the project using Pachinko project as an initial base
- Utilized ImGUI to quickly manipulate values through a widget system
- Used my existing cursor framework to create a simple cursor and use some functions to determine cursor position and click logic

What went bad:
- Was unable to figure out reasons for slow down till the end of the project development
- Never considered that debug features could be causing slow down due to the need for DebuggerPrintf to flush the log on every function call
- Was unable to implement the scale and rotate functionality due to speed issues taking up too much time

What I learned:
- It's wise to consider debug information as potential slow down cause
- Profiler may not always pick up some information from code
- Never make any assumptions with regard to code. The assumption that debug code is "safe to use" is a bad assumption especially when considering speed