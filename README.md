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
Constructor from parameters. Width and height of texture are specified by first and second parameter respectively, rawData parameter is a pointer to 4-channel RGBA image data, cSetIndex and cBinding are the set index and binding, with which the texture can be fetched from shader.
```cpp
void create(const uint32_t width, const uint32_t height, const void* rawData, uint32_t cSetIndex, uint32_t cBinding)
```
Creation function. Must be called only once and only if the texture was created using default constructor.
```cpp
Texture& operator=(const Texture& rTexture)
Texture& operator=(Texture& rTexture)
```
Copy function. Deletes old texture content (if such existed) and creates new texture, similar to rTexture.
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

