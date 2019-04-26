# Spark
Graphics library, based on Vulkan
## Status
In development
## Class list
```spk::System``` - class, that holds ```vk::Instance```, ```vk::PhysicalDevice``` and ```vk::Device```.

```spk::WindowSystem``` - class, that holds GLFW3 window handle and ```vk::Surface``` to draw to.

```spk::MemoryManager``` - class, that handles ```vk::DeviceMemory``` management operation and holds array of ```vk::DeviceMemory```structures; supports lazy allocations to reduce memory allocation count.
