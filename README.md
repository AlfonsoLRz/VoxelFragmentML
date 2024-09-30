# Voxel fragmentation

![c++](https://img.shields.io/github/languages/top/AlfonsoLRz/VesselFracturer) 
![opengl](https://img.shields.io/badge/opengl-4.5-red.svg) 
![glfw3](https://img.shields.io/badge/glfw3-3.3.7-purple.svg) 
![glew](https://img.shields.io/badge/glew-2.2.0-yellow.svg) 
![glew](https://img.shields.io/badge/glm-0.9.9.8-green.svg) 
![assimp](https://img.shields.io/badge/assimp-5.2.4-orange.svg) 
![license](https://img.shields.io/badge/license-MIT-blue.svg)

GPU-based fragmentation of voxelizations using OpenGL compute shaders. This project is aimed at generating datasets for training fragment assembly models.

 <p align="center" >
    <img src="Assets/teaser.png"/></br>
    <em>Assembled fragments of Iberian vessels.</em>
</p>

## Dependencies

The code in this repository has some dependencies which are following listed:

- assimp 5.2.4.
- glew 2.2.0.
- opengl 4.5.
- glfw3 3.3.7.
- glm 0.9.9.8.
- simplify (https://github.com/sp4cerat/Fast-Quadric-Mesh-Simplification).
- MagicaVoxel File Writer (https://github.com/aiekick/MagicaVoxel_File_Writer).

**_*The last two dependencies were already included as part of this repo, under the folder `VesselFracturer/Libraries/`._**

## Windows

The project is primarily intended to be used in Windows. The Microsoft Visual Studio project files are uploaded to the repo and therefore it should be trivial to open it (regardless of changing the development platform kit). The project was configured as follows:

- Development platform kit: `v143`.
- Language standard: `C++ 17`.
- Integration with `vcpkg`. After cloning `vcpkg` and launching the main `.bat`, it can be integrated with MSVC by executing `vcpkg integrate install` in the command line (note that, `vcpkg` must be registered in the system path).

## Linux

TODO
