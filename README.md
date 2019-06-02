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
***
```cpp
Texture()
```
Default constructor. Does not init anything.
***
```cpp
Texture(const Texture& txt)
```
Constructor from an existing texture. Creates texture, similar to given.
***
```cpp
Texture(Texture&& txt)
```
Constructor from an existing texture. Moves given texture to from which it was called.
***
```cpp
Texture(const uint32_t cWidth, const uint32_t cHeight, ImageFormat cFormat, uint32_t cSetIndex, uint32_t cBinding)
```
Constructor from parameters. Width and height of texture (in **texels**) are specified by first and second parameter respectively, cFormat is an image format, cSetIndex and cBinding are the set index and binding, with which the texture can be fetched in shader.
***
```cpp
void create(const uint32_t cWidth, const uint32_t cHeight, ImageFormat cFormat, uint32_t cSetIndex, uint32_t cBinding)
```
Creation function. Must be called only once and only if the texture was created using default constructor.
***
```cpp
Texture& operator=(const Texture& rTexture)
Texture& operator=(Texture& rTexture)
```
Copy functions. Each deletes old texture content (if such existed) and creates new texture, similar to rTexture.
***
```cpp
Texture& operator=(Texture&& rTexture)
```
Move function. Deletes old texture content (if such existed) and moves rTexture to the current texture.
***
```cpp
void resetSetIndex(const uint32_t newIndex)
```
Resets texture set index.
***
```cpp
void resetBinding(const uint32_t newBinding)
```
Resets texture binding.
***
```cpp
~Texture()
```
Destructor.
***

#### Uniform buffer class
```cpp
spk::UniformBuffer
```
**Public member functions**
***
```cpp
UniformBuffer()
```
Default constructor. Does not init anything.
***
```cpp
UniformBuffer(const UniformBuffer& ub)
```
Constructor from an existing uniform buffer. Creates uniform buffer, similar to given.
***
```cpp
UniformBuffer(UniformBuffer&& ub)
```
Constructor from an existing uniform buffer. Moves ub to the current uniform buffer.
***
```cpp
UniformBuffer(const uint32_t cSize, const uint32_t cSetIndex, const uint32_t cBinding)
```
Constructor. Size of the buffer (in **bytes**) is specified by cSize parameter, cSetIndex and cBinding are the set index and binding, with which the buffer can be fetched in shader.
***
```cpp
void create(const uint32_t cSize, const uint32_t cSetIndex, const uint32_t cBinding)
```
Creation function. Must be called only once and only if the buffer was created using default constructor.
***
```cpp
UniformBuffer& operator=(const UniformBuffer& rBuffer)
UniformBuffer& operator=(UniformBuffer& rBuffer)
```
Copy functions. Each deletes old uniform buffer content (if such existed) and creates new uniform buffer, similar to rBuffer.
***
```cpp
UniformBuffer& operator=(UniformBuffer&& rBuffer)
```
Move function. Deletes old uniform buffer content (if such existed) and moves rBuffer to the current uniform buffer object.
***
```cpp
void resetSetIndex(const uint32_t newIndex)
```
Resets uniform buffer set index.
***
```cpp
void resetBinding(const uint32_t newBinding)
```
Resets uniform buffer binding.
***
```cpp
~UniformBuffer();
```
Destructor.
***
#### Resource Set Class
```cpp
spk::ResourceSet
```
**Public member functions**
***
```cpp
ResourceSet()
```
Default constructor. Does not init anything.
***
```cpp
ResourceSet(std::vector<Texture>& cTextures, std::vector<UniformBuffer>& cUniformBuffers)
```
Constructor. Creates resource set from given textures and uniform buffers.
***
```cpp
ResourceSet(const ResourceSet& set)
```
Constructor. Copies the contents of a given set.
***
```cpp
ResourceSet& operator=(const ResourceSet& set)
```
Copy function. Deletes old contents of a current resource set and inits it with parameters from given set
***
```cpp
void create(std::vector<Texture>& cTextures, std::vector<UniformBuffer>& cUniformBuffers)
```
Creates resource set from given textures and uniform buffers. Must be called only once and only if the resource set was created using default constructor.
***
```cpp
void update(const uint32_t set, const uint32_t binding, const void* data)
```
Updates texture or uniform buffer that have setIndex and binding equal to given, with given data.
***
```cpp
~ResourceSet()
```
Destructor.
***
#### Shader Set Class
```cpp
spk::ShaderSet
```
**Public member functions**
***
```cpp
ShaderSet()
```
Default constructor. Does not init anything.
***
```cpp
ShaderSet(const ShaderSet& set)
```
Constructor. Creates object using parameters from given set.
***
```cpp
ShaderSet(const std::vector<ShaderInfo>& shaders)
```
Constructor. Creates object using ```ShaderInfo``` structs.
***
```cpp
void create(const std::vector<ShaderInfo>& shaders)
```
Constructs shader set from given shader infos. Must be called only once and only if the shader set was created using default constructor.
***
```cpp
ShaderSet& operator=(const ShaderSet& set)
```
Copy function. Deletes old contents of a current shader set and inits it with parameters of a given set.
***
```cpp
~ShaderSet()
```
Destructor.
***
#### Vertex Buffer Class
```cpp
spk::VertexBuffer
```
**Public member functions**
***
```cpp
VertexBuffer()
```
Default constructor. Does not init anything.
***
```cpp
VertexBuffer(const VertexBuffer& vb)
```
Constructor. Creates vertex buffer from the other vertex buffer.
***
```cpp
VertexBuffer(VertexBuffer&& vb)
```
Constructor. Moves given buffer to current.
***
```cpp
VertexBuffer(const std::vector<VertexAlignmentInfo>& cAlignmentInfos, const std::vector<uint32_t>& cVertexBufferSizes, const uint32_t cIndexBufferSize = 0)
```
Constructor. Creates vertex buffer using given ```VertexAlignmentInfo```s, sizes of an each vertex buffer binding and size of an index buffer (both in **bytes**). For non-indexed draws do not specify cIndexBufferSize parameter or set it to 0.
***
```cpp
void create(const std::vector<VertexAlignmentInfo>& cAlignmentInfos, const std::vector<uint32_t>& cVertexBufferSizes, const uint32_t cIndexBufferSize = 0)
```
Creates vertex buffer using given ```VertexAlignmentInfo```s, sizes of an each vertex buffer binding and size of an index buffer. For non-indexed draws do not specify cIndexBufferSize parameter or set it to 0. Must be called only once and only if the object was created using default constructor.
***
```cpp
void setInstancingOptions(const uint32_t count, const uint32_t first)
```
Sets count of instances and the index of the first instance. Instances are used (often in combination with uniform buffers) for drawing lots of similar objects that have slightly different parameters. By default count is 1 and first index is 0.
***
```cpp
void updateVertexBuffer(const void* data, const uint32_t binding)
```
Writes given data to the specified vertex buffer binding.
***
```cpp
void updateIndexBuffer(const void* data)
```
Writes given data to the index buffer.
***
```cpp
VertexBuffer& operator=(const VertexBuffer& rBuffer)
VeretxBuffer& operator=(VertexBuffer& rBuffer)
```
Each of these function destroys current VertexBuffer content and creates new VertexBuffer using the data fetched from rBuffer.
***
```cpp
VertexBuffer& operator=(VertexBuffer&& rBuffer)
```
Deletes all current VertexBuffer contents and moves rBuffer contents to the current VertexBuffer.
***
```cpp
~VertexBuffer()
```
Destructor.
***
#### Window Class
```cpp
spk::Window
```
**Public member functions**
***
```cpp
Window()
```
Default constructor. Doesn't init anything.
***
```cpp
Window(const uint32_t cWidth, const uint32_t cHeight, const std::string cTitle, const DrawOptions cOptions)
```
Constructor. Creates window from given parameters: width, height, title and draw options of the window.
***
```cpp
void create(const uint32_t cWidth, const uint32_t cHeight, const std::string cTitle, const DrawOptions cOptions)
```
Creates window from given parameters: width, height, title and draw options of the window. Must be called only once and only if the object was created using default constructor.
***
```cpp
void draw(const ResourceSet* resources, const VertexBuffer* vertexBuffer, const ShaderSet* shaders)
```
Draws picture and presents it using given pointers: pointer to resource set for use in shaders, pointer to vertex buffer and pointer to shader set to load shaders.
***
```cpp
GLFWwindow* getGLFWWindow()
```
Gets created GLFW window pointer for you to handle.
***
```cpp
~Window()
```
Destructor.
***
### Enums and structs
```cpp
enum class ShaderType
{
  vertex, 
  fragment
}
```
This enumeration class is used to identify, which type of shader you are going to load.
***
```cpp
struct ShaderInfo
{
  ShaderType type;
  std::string filename;
}
```
This structure is used to get shader code from file and identify shader type. Shader file **must** be valid SPIR-V shader, which you can compile from GLSL-like shader code using glslangValidator.
***
```cpp
enum class FieldFormat
{
  int32,
  uint32,
  float32,
  double64,

  vec2i,
  vec2u,
  vec2f,
  vec2d,

  vec3i,
  vec3u,
  vec3f,
  vec3d,

  vec4i,
  vec4u,
  vec4f,
  vec4d
}
```
This enumeration class spescifies the type of vertex field, which can be understood as follows:
+ int32 = 32-bit signed integer
+ uint32 = 32-bit unsigned integer
+ float32 = 32-bit signed float
+ double64 = 64-bit double-precision signed float
+ vec2i = vector of 2 int32
+ vec2u = vector of 2 uint32
+ vec2f = vector of 2 float32
+ vec2d = vector of 2 double64
+ vec3i = vector of 3 int32
+ vec3u = vector of 3 uint32
+ vec3f = vector of 3 float32
+ vec3d = vector of 3 double64
+ vec4i = vector of 4 int32
+ vec4u = vector of 4 uint32
+ vec4f = vector of 4 float32
+ vec4d = vector of 4 double64
***
```cpp
struct StructFieldInfo
{
  uint32_t location;
  FieldFormat format;
  uint32_t offset;
}
```
StructFieldInfo specifies one field of structure you are going to use as vertex in vertex shader. ```location``` is a shader layout attribute, ```format``` is a format of a field and ```offset``` is a field offset (in **bytes**) from the befinning of the structure (usually obtained with a standard function offsetof()).
***
```cpp
struct VertexAlignmentInfo
{
  uint32_t binding;
  uint32_t structSize;
  std::vector<StructFieldInfo> fields;
}
```
VertexAlignmentInfo specifies how the vertex components are aligned. ```binding``` is a shader layout attribute, ```structSize``` is a size of one vertex (in **bytes**) and ```fields``` vector describes every field of vertex class or structure.
***
```cpp
enum class ImageFormat
{
  RGBA8,
  RGB8,
  BGR8,
  BGRA8,
  RGB16,
  RGBA16,
};
```
ImageFormat enumeration class specifes the format of image with following syntax: R, G, B and A letters indicate channel order and availability and number sets the number of bits per channel.
***
```cpp
enum class CullMode
{
  Clockwise,
  CounterClockwise,
  None
}
```
CullMode enumeration class specifies the way back face culling will be performed. If ```CullMode``` is set to None, no culling will be performed; if ```CullMode``` is set to Clockwise, all primitives with clockwise vertex order will be **discarded**; if ```CullMode``` is set to CounterClockwise, all primitives with counter-clockwise vertex order will be **discarded**.
***
```cpp
struct DrawOptions
{
  CullMode cullMode;
}
```
DrawOptions structure specifies additional window drawing options. For now, you can only specify the way of vertex culling.
***
