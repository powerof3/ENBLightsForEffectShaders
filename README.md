# ENB Lights For Effect Shaders

SKSE plugin that adds support for ENB Light for all effect shaders.

## Requirements
* [CMake](https://cmake.org/)
	* Add this to your `PATH`
* [PowerShell](https://github.com/PowerShell/PowerShell/releases/latest)
* [Vcpkg](https://github.com/microsoft/vcpkg)
	* Add the environment variable `VCPKG_ROOT` with the value as the path to the folder containing vcpkg
* [Visual Studio Community 2019](https://visualstudio.microsoft.com/)
	* Desktop development with C++
* [CommonLibSSE](https://github.com/powerof3/CommonLibSSE/tree/dev)
	* You need to build from the powerof3/dev branch
	* Add this as as an environment variable `CommonLibSSEPath`
* [ColorSpace](https://github.com/berendeanicolae/ColorSpace)
	* C++ library for converting between color spaces and comparing colors.

## Register Visual Studio as a Generator
* Open `x64 Native Tools Command Prompt`
* Run `cmake`
* Close the cmd window

## Building
```
git clone https://github.com/powerof3/ENBLightsForEffectShaders.git
cd ENBLightsForEffectShaders
cmake -B build -S .
```
## License
[MIT](LICENSE)
