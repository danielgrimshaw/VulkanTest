D:/VulkanSDK/1.0.13.0/Bin/glslangValidator.exe -V Shader.vert
echo "Is Model On?"
set /p input= yes or no: 

if %input%==yes D:/VulkanSDK/1.0.13.0/Bin/glslangValidator.exe -V Shader.frag
if %input%==no D:/VulkanSDK/1.0.13.0/Bin/glslangValidator.exe -V Fractal.frag

COPY vert.spv D:\Programming\Github\Graphics\Vulkan\VulkanTesting\Vulkan\x64\Debug\vert.spv
COPY frag.spv D:\Programming\Github\Graphics\Vulkan\VulkanTesting\Vulkan\x64\Debug\frag.spv
COPY vert.spv D:\Programming\Github\Graphics\Vulkan\VulkanTesting\Vulkan\Vulkan\vert.spv
COPY frag.spv D:\Programming\Github\Graphics\Vulkan\VulkanTesting\Vulkan\Vulkan\frag.spv
pause