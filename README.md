![top_banner](./.github/banner.jpg)

---

### TKVulkan
![workflow_badge](https://img.shields.io/github/actions/workflow/status/RPGtk/tk-vulkan/build.yml?label=Build%20Test)
![loc_badge](https://github.com/R-TK1/tk-vulkan/blob/badges/loc.svg)
![latest_release_badge](https://img.shields.io/github/v/release/RPGtk/tk-vulkan?label=Experimental&include_preleases=true)

A small, very specialized [Vulkan](https://www.vulkan.org/) wrapper to make it easier to carry out certain operations like initialization and [windowing system integration](https://docs.vulkan.org/guide/latest/wsi.html). This is not a full-fledged [graphics library](https://en.wikipedia.org/wiki/Graphics_library); it's simply a shallow Vulkan wrapper. The only logic not directly related to Vulkan is the [SPIR-V](https://en.wikipedia.org/wiki/Standard_Portable_Intermediate_Representation) shader [compilation](https://github.com/KhronosGroup/glslang) interface, provided as a standalone executable (for the purpose of deletion once its tasks are complete).

---

#### Dependencies
TKVulkan requires very few dependencies by design.

- [Vulkan](https://vulkan.lunarg.com/): Vulkan is a cross-platform, performant graphics API for modern and older systems alike. It provides an understandable but explicit API for handling graphical tasks.
- [GLSLang](https://github.com/KhronosGroup/glslang): A frontend for many shader languages, including GLSL and ESSL, and a SPIR-V bytecode generator. We only truly use the SPIR-V bytecode generator.

---

![bottom_banner](./.github/banner.jpg)