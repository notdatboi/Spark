LIBS= -lvulkan -lglfw -lassimp
CC=g++ -std=c++17
BIN=a.out
SOURCES=$(wildcard src/*.cpp)
OBJS=$(patsubst src/%.cpp,obj/%.o,$(SOURCES))

all: $(OBJS)

obj/Executives.o: src/Executives.cpp \
	include/Executives.hpp \
	include/SparkIncludeBase.hpp \
	include/System.hpp
	$(CC) -c $< -o $@ -g

obj/MemoryManager.o: src/MemoryManager.cpp \
	include/MemoryManager.hpp \
	include/SparkIncludeBase.hpp \
	include/System.hpp 
	$(CC) -c $< -o $@ -g

obj/ResourceSet.o: src/ResourceSet.cpp \
	include/ResourceSet.hpp \
	include/Texture.hpp \
	include/System.hpp \
	include/Executives.hpp \
	include/MemoryManager.hpp \
	include/UniformBuffer.hpp \
	include/SparkIncludeBase.hpp \
	include/Image.hpp \
	include/Buffer.hpp \
	include/ImageView.hpp
	$(CC) -c $< -o $@ -g

obj/System.o: src/System.cpp \
	include/System.hpp \
	include/Executives.hpp \
	include/SparkIncludeBase.hpp
	$(CC) -c $< -o $@ -g

obj/Texture.o: src/Texture.cpp \
	include/Texture.hpp \
	include/System.hpp \
	include/MemoryManager.hpp \
	include/Executives.hpp \
	include/SparkIncludeBase.hpp \
	include/Image.hpp \
	include/Buffer.hpp \
	include/ImageView.hpp
	$(CC) -c $< -o $@ -g

obj/UniformBuffer.o: src/UniformBuffer.cpp \
	include/UniformBuffer.hpp \
	include/System.hpp \
	include/MemoryManager.hpp \
	include/Executives.hpp \
	include/Buffer.hpp \
	include/SparkIncludeBase.hpp
	$(CC) -c $< -o $@ -g
	
obj/VertexBuffer.o: src/VertexBuffer.cpp \
	include/VertexBuffer.hpp \
	include/Executives.hpp \
	include/System.hpp \
	include/MemoryManager.hpp \
	include/Buffer.hpp \
	include/SparkIncludeBase.hpp
	$(CC) -c $< -o $@ -g

obj/Window.o: src/Window.cpp \
	include/Window.hpp \
	include/System.hpp \
	include/ResourceSet.hpp  \
	include/ShaderSet.hpp \
	include/Buffer.hpp \
	include/SparkIncludeBase.hpp
	$(CC) -c $< -o $@ -g

obj/ShaderSet.o: src/ShaderSet.cpp \
	include/ShaderSet.hpp \
	include/System.hpp \
	include/SparkIncludeBase.hpp
	$(CC) -c $< -o $@ -g

obj/Image.o: src/Image.cpp \
	include/Image.hpp \
	include/System.hpp \
	include/Executives.hpp \
	include/MemoryManager.hpp \
	include/SparkIncludeBase.hpp
	$(CC) -c $< -o $@ -g

obj/ImageView.o: src/ImageView.cpp \
	include/ImageView.hpp \
	include/System.hpp \
	include/SparkIncludeBase.hpp
	$(CC) -c $< -o $@ -g

obj/Buffer.o: src/Buffer.cpp \
	include/Buffer.hpp \
	include/System.hpp \
	include/Executives.hpp \
	include/MemoryManager.hpp \
	include/SparkIncludeBase.hpp
	$(CC) -c $< -o $@ -g