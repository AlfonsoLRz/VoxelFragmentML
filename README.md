# Voxel fragmentation

![c++](https://img.shields.io/github/languages/top/AlfonsoLRz/VoxelFragmentML) 
![opengl](https://img.shields.io/badge/opengl-4.6-red.svg) 
![glfw3](https://img.shields.io/badge/glfw3-3.3.7-purple.svg) 
![glew](https://img.shields.io/badge/glew-2.2.0-yellow.svg) 
![glew](https://img.shields.io/badge/glm-0.9.9.8-green.svg) 
![assimp](https://img.shields.io/badge/assimp-5.2.4-orange.svg) 
![license](https://img.shields.io/badge/license-MIT-blue.svg)

<a href="https://doi.org/10.1016/j.cag.2024.104104" target="_blank">[Paper]</a>
<a href="https://zenodo.org/records/13899699" target="_blank">[Voxel data (Zenodo, 3GB)]</a>
<a href="https://s5-ceatic.ujaen.es/fragment-dataset-uja/" target="_blank">[Complete dataset (450 GB)]</a>
<a href="https://s5-ceatic.ujaen.es/fragment-dataset-uja/" target="_blank">[Triangle meshes and point clouds (partial, 27 GB)]</a>

GPU-based fragmentation of voxelizations using OpenGL compute shaders. This project is aimed at generating datasets for training fragment assembly models. While this fragmentation method can be applied over any mesh, this work specifically focus on archaeological artefacts such as those depicted in *Figure 1*.

 <p align="center">
    <img src="docs/data/dataset.png"/></br>
    <em>Figure 1. Assembled fragments of Iberian vessels.</em>
</p>

## Dependencies

The code in this repository has some dependencies which are following listed:

- assimp 5.2.4.
- glew 2.2.0.
- opengl 4.6.
- glfw3 3.3.7.
- glm 0.9.9.8.
- [simplify](https://github.com/sp4cerat/Fast-Quadric-Mesh-Simplification).
- [MagicaVoxel File Writer](https://github.com/aiekick/MagicaVoxel_File_Writer).

**_*The last two dependencies were already included as part of this repo, under the folder `MeshFragments/Libraries/`._**

## Windows

The project is primarily intended to be used in Windows. The Microsoft Visual Studio project files are uploaded to the repo and therefore it should be trivial to open it (regardless of changing the development platform kit). The project was configured as follows:

- Development platform kit: `v143`.
- Language standard: `C++ 23`.
- Integration with `vcpkg`. After cloning `vcpkg` and launching the main `.bat`, it can be integrated with MSVC by executing `vcpkg integrate install` in the command line (note that `vcpkg` can be registered in the system path for easier usage).

## Data download

The whole fragment data is available at <a href="https://s5-ceatic.ujaen.es/fragment-dataset-uja/">our research institute's page</a>. However, two lighter versions have been released since the complete dataset is too heavy (450 GB). Moreover, we encourage the readers to primarily use the Zenodo dataset if your work is centred on implicit data/voxels. In summary, these are the available datasets:

- A <a href="https://zenodo.org/records/13899699" target="_blank">3GB dataset</a> composed only of voxel data, published in Zenodo.

- The <a href="https://s5-ceatic.ujaen.es/fragment-dataset-uja/" target="_blank">whole fragment dataset</a>, split into eight files of ~50GB (totalling 450GB) with compressed voxel data, point clouds and triangle meshes.

- A <a href="https://s5-ceatic.ujaen.es/fragment-dataset-uja/" target="_blank">lighter version</a> of uncompressed triangle meshes and point clouds (`vessels_200_obj_ply_no_zipped.zip`; 27 GB). This is mainly intended for testing the dataset since it only contains decimated fragments of 200 models, with no individual zipping. However, note that these are provided as triangle meshes and point clouds derived from marching cubes, and may have more geometric inaccuracies. 

## Decompress binary data

The scripts to decompress binary grids, meshes and point clouds are available at `docs/decompress`. Point clouds are decompressed in C++ using the Point Cloud Library (PCL), whereas the other formats are decompressed using Python.

<p align="center">
    <img src="docs/data/decompress_binaries.png">
    <em>Figure 2. Rendering uncompressed data.</em>
</p>

## Citation

    @article{LopezGenerating2024,
        title = {Generating implicit object fragment datasets for machine learning},
        journal = {Computers & Graphics},
        pages = {104104},
        year = {2024},
        issn = {0097-8493},
        doi = {https://doi.org/10.1016/j.cag.2024.104104},
        url = {https://www.sciencedirect.com/science/article/pii/S0097849324002395},
        author = {Alfonso López and Antonio J. Rueda and Rafael J. Segura and Carlos J. Ogayar and Pablo Navarro and José M. Fuertes},
        keywords = {Voxel, Fragmentation, Fracture dataset, Voronoi, GPU programming}
    }
