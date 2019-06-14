LIBS= -lvulkan -lglfw -lassimp
CC=g++ -std=c++17
BIN=a.out
SOURCES=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,obj/%.o,$(SOURCES))

all: $(OBJS)

obj/Executives.o: Executives.cpp \
	Executives.hpp \
	SparkIncludeBase.hpp \
	System.hpp
	$(CC) -c $< -o $@ -g

obj/MemoryManager.o: MemoryManager.cpp \
	MemoryManager.hpp \
	SparkIncludeBase.hpp \
	System.hpp 
	$(CC) -c $< -o $@ -g

obj/ResourceSet.o: ResourceSet.cpp \
	ResourceSet.hpp \
	Texture.hpp \
	System.hpp \
	Executives.hpp \
	MemoryManager.hpp \
	UniformBuffer.hpp \
	SparkIncludeBase.hpp \
	Image.hpp \
	Buffer.hpp \
	ImageView.hpp
	$(CC) -c $< -o $@ -g

obj/System.o: System.cpp \
	System.hpp \
	Executives.hpp \
	SparkIncludeBase.hpp
	$(CC) -c $< -o $@ -g

obj/Texture.o: Texture.cpp \
	Texture.hpp \
	System.hpp \
	MemoryManager.hpp \
	Executives.hpp \
	SparkIncludeBase.hpp \
	Image.hpp \
	Buffer.hpp \
	ImageView.hpp
	$(CC) -c $< -o $@ -g

obj/UniformBuffer.o: UniformBuffer.cpp \
	UniformBuffer.hpp \
	System.hpp \
	MemoryManager.hpp \
	Executives.hpp \
	Buffer.hpp \
	SparkIncludeBase.hpp
	$(CC) -c $< -o $@ -g
	
obj/VertexBuffer.o: VertexBuffer.cpp \
	VertexBuffer.hpp \
	Executives.hpp \
	System.hpp \
	MemoryManager.hpp \
	Buffer.hpp \
	SparkIncludeBase.hpp
	$(CC) -c $< -o $@ -g

obj/Window.o: Window.cpp \
	Window.hpp \
	System.hpp \
	ResourceSet.hpp  \
	ShaderSet.hpp \
	Buffer.hpp \
	SparkIncludeBase.hpp
	$(CC) -c $< -o $@ -g

obj/ShaderSet.o: ShaderSet.cpp \
	ShaderSet.hpp \
	System.hpp \
	SparkIncludeBase.hpp
	$(CC) -c $< -o $@ -g

obj/Image.o: Image.cpp \
	Image.hpp \
	System.hpp \
	Executives.hpp \
	MemoryManager.hpp \
	SparkIncludeBase.hpp
	$(CC) -c $< -o $@ -g

obj/ImageView.o: ImageView.cpp \
	ImageView.hpp \
	System.hpp \
	SparkIncludeBase.hpp
	$(CC) -c $< -o $@ -g

obj/Buffer.o: Buffer.cpp \
	Buffer.hpp \
	System.hpp \
	Executives.hpp \
	MemoryManager.hpp \
	SparkIncludeBase.hpp
	$(CC) -c $< -o $@ -g