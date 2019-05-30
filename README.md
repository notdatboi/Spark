# Spark
Graphics library, based on Vulkan
## Documentation
### Global functions
```cpp
spk::system::init()
```
Initializes Spark. Must be called before any usage of the library.
```cpp
spk::system::deinit()
```
Deinitializes Spark. Must be called after the destruction of **all** Spark objects
### Classes
#### Texture class
```cpp
spk::Texture
```
**Public member functions**
```cpp
Texture()
```
Default constructor. Does not init anything.
```cpp
Texture(const Texture& txt)
```
Constructor from an existing texture. Creates texture, similar to given.
```cpp
Texture(Texture&& txt)
```
Constructor from an existing texture. Moves given texture to from which it was called.
```cpp
Texture(const uint32_t width, const uint32_t height, const void* rawData, uint32_t cSetIndex, uint32_t cBinding)
```
Constructor from parameters. Width and height of texture are specified by first and second parameter respectively, rawData parameter is a pointer to 4-channel RGBA image data, cSetIndex and cBinding are the set index and binding, with which the texture can be fetched in shader.
```cpp
void create(const uint32_t width, const uint32_t height, const void* rawData, uint32_t cSetIndex, uint32_t cBinding)
```
Creation function. Must be called only once and only if the texture was created using default constructor.
```cpp
Texture& operator=(const Texture& rTexture)
Texture& operator=(Texture& rTexture)
```
Copy functions. Each deletes old texture content (if such existed) and creates new texture, similar to rTexture.
```cpp
Texture& operator=(Texture&& rTexture)
```
Move function. Deletes old texture content (if such existed) and moves rTexture to the current texture.
```cpp
void resetSetIndex(const uint32_t newIndex)
```
Resets texture set index.
```cpp
void resetBinding(const uint32_t newBinding)
```
Resets texture binding.
```cpp
~Texture()
```
Destructor.

#### Uniform buffer class
```cpp
spk::UniformBuffer
```
**Public member functions**
```cpp
UniformBuffer()
```
Default constructor. Does not init anything.
```cpp
UniformBuffer(const UniformBuffer& ub)
```
Constructor from an existing uniform buffer. Creates uniform buffer, similar to given.
```cpp
UniformBuffer(UniformBuffer&& ub)
```
Constructor from an existing uniform buffer. Moves ub to the current uniform buffer.
```cpp
UniformBuffer(const uint32_t cSize, const uint32_t cSetIndex, const uint32_t cBinding)
```
Constructor. Size of the buffer is specified by cSize parameter, cSetIndex and cBinding are the set index and binding, with which the buffer can be fetched in shader.
```cpp
void create(const uint32_t cSize, const uint32_t cSetIndex, const uint32_t cBinding)
```
Creation function. Must be called only once and only if the buffer was created using default constructor.
```cpp
UniformBuffer& operator=(const UniformBuffer& rBuffer)
UniformBuffer& operator=(UniformBuffer& rBuffer)
```
Copy functions. Each deletes old uniform buffer content (if such existed) and creates new uniform buffer, similar to rBuffer.
```cpp
UniformBuffer& operator=(UniformBuffer&& rBuffer)
```
Move function. Deletes old uniform buffer content (if such existed) and moves rBuffer to the current uniform buffer object.
```cpp
void resetSetIndex(const uint32_t newIndex)
```
Resets uniform buffer set index.
```cpp
void resetBinding(const uint32_t newBinding)
```
Resets uniform buffer binding.
```cpp
~UniformBuffer();
```
Destructor.
