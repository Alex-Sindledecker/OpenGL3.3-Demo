#include <SFML/Graphics.hpp>
#include <random>
#include <time.h>
#include <GL/glew.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "CameraFP.h"

constexpr int GLEW_INIT_FAILURE = -1;
const GLfloat quadVertices[] = {
		1.f, 1.f, 0.f,
		0.f, 1.f, 0.f,
		0.f, 0.f, 0.f,
		0.f, 0.f, 0.f,
		1.f, 0.f, 0.f,
		1.f, 1.f, 0.f
};
sf::Image heightMap;

//Loads a .glsl file and compiles in into a shader
GLuint loadShader(const char* source, GLenum type)
{
	std::ifstream file;
	file.open(source);
	if (!file.is_open())
		return 0;
	std::stringstream ss;
	ss << file.rdbuf();
	std::string lines = ss.str();
	const char* raw_shader = lines.c_str();

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &raw_shader, NULL);
	glCompileShader(shader);

	char infoLog[512];
	int success;
	glGetShaderiv(type, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cout << infoLog << std::endl;
	}

	return shader;
}

//Creates a shader program
GLuint genShaderProgram(GLuint vertexShader, GLuint fragmentShader)
{
	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	glDetachShader(program, vertexShader);
	glDetachShader(program, fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	return program;
}

//Loads an image and generates a texture id
GLuint loadTexture(const char* source, GLenum colorSpace = GL_SRGB)
{
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	float aniso = 0.f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
	sf::Image image;
	if (!image.loadFromFile(source))
		return -3;
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexImage2D(GL_TEXTURE_2D, 0, colorSpace, image.getSize().x, image.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());
	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

//Generates an array vertex buffer object. It will store the passed data linearly in a buffer
GLuint genArrayVBO(GLsizeiptr size, const void* data)
{
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return vbo;
}

//Each vao has multiple "slots" and data can be inserted into each. This struct contains information about the data that is inserted
struct VAOslot
{
	GLuint vbo, index, vs;
	GLenum type;
	GLsizei stride;
	const void* offset;
};

//Generates a vertex array object (VAO)
GLuint genVAO(VAOslot* slots, size_t count)
{
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	for (int i = 0; i < count; i++)
	{
		glBindBuffer(GL_ARRAY_BUFFER, slots[i].vbo);
		glEnableVertexAttribArray(slots[i].index);
		glVertexAttribPointer(slots[i].index, slots[i].vs, slots[i].type, GL_FALSE, slots[i].stride, slots[i].offset);
	}
	glBindVertexArray(0);

	return vao;
}

//Container for buffers of obj datae
struct OBJ
{
	GLuint v_vbo, n_vbo, u_vbo, vao, vertexCount = 0;
};

//Vector 3 used to store either a 3D vertex or a normal vector
struct Vertex
{
	float x, y, z;
};
typedef Vertex Normal;

//Vector 2 for containing uv coords
struct UV
{
	float u, v;
};

//Loads an .obj file
void loadOBJ(const char* source, OBJ& obj)
{
	std::ifstream file;
	file.open(source);
	int lines = std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n') + 1;
	file.seekg(file.beg);
	int faces = std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), 'f');
	file.seekg(file.beg);
	std::vector<Vertex> vertices(lines / 3);
	std::vector<Normal> normals(lines / 3);
	std::vector<UV> uvs(lines / 3);
	std::vector<Vertex> nonIndexedVertices(faces * 3 * 3);
	std::vector<Normal> nonIndexedNormals(faces * 3);
	std::vector<UV> nonIndexedUvs(faces * 3);
	if (!file.is_open())
	{
		std::cout << "Failed to load obj: " << source << "! Exiting...";
		exit(-5);
	}

	std::stringstream stream;
	stream << file.rdbuf();

	int vIndex = 0, nIndex = 0, uvIndex = 0, niVIndex = 0, niNIndex = 0, niUVIndex = 0;
	std::string type;
	
	while (stream >> type)
	{
		if (type == "v")
		{
			Vertex vertex;
			stream >> vertex.x >> vertex.y >> vertex.z;
			vertices[vIndex] = vertex;
			vIndex++;
		}
		else if (type == "vn")
		{
			Normal normal;
			stream >> normal.x >> normal.y >> normal.z;
			normals[nIndex] = normal;
			nIndex++;
		}
		else if (type == "vt")
		{
			UV uv;
			stream >> uv.u >> uv.v;
			uvs[uvIndex] = uv;
			uvIndex++;
		}
		else if (type == "f")
		{
			char temp;
			int i1, i2, i3;

			for (int i = 0; i < 3; i++)
			{
				stream >> i1 >> temp >> i2 >> temp >> i3;
				nonIndexedVertices[niVIndex] = vertices[i1 - 1];
				nonIndexedNormals[niNIndex] = normals[i3 - 1];
				nonIndexedUvs[niUVIndex] = uvs[i2 - 1];
				niVIndex++;
				niNIndex++;
				niUVIndex++;
			}
			obj.vertexCount += 3;
		}
	}

	obj.v_vbo = genArrayVBO(nonIndexedVertices.size() * sizeof(Vertex), &nonIndexedVertices[0]);
	obj.n_vbo = genArrayVBO(nonIndexedNormals.size() * sizeof(Normal), &nonIndexedNormals[0]);
	obj.u_vbo = genArrayVBO(nonIndexedUvs.size() * sizeof(UV), &nonIndexedUvs[0].u);
}

//Container for terrain data buffers and properties
struct Terrain
{
	GLuint v_vbo, n_vbo, vao;
	int size, samples;
	std::vector<float> heights;
};

//Returns the height of the terrain at a given x and z pos
float getTerrainHeight(float xpos, float zpos)
{
	sf::Color pixel = heightMap.getPixel(xpos * (heightMap.getSize().x - 1), zpos * (heightMap.getSize().y - 1));
	float value = (pixel.r + pixel.g + pixel.b) / 256.f;
	return value * 3.f;
}

//Gets the height of a triangle at x, y within vertices p1, p2, and p3. I have no idea how this works at all
float getBaryCentricHeight(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float x, float z)
{
	float det = (p2.z - p3.z) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.z - p3.z);

	float l1 = ((p2.z - p3.z) * (x - p3.x) + (p3.x - p2.x) * (z - p3.z)) / det;
	float l2 = ((p3.z - p1.z) * (x - p3.x) + (p1.x - p3.x) * (z - p3.z)) / det;
	float l3 = 1.0f - l1 - l2;

	return l1 * p1.y + l2 * p2.y + l3 * p3.y;
}

//Returns the collision height of the terrain based on the worldX and worldZ
float getTerrainCollisionHeight(Terrain& terrain, float worldX, float worldZ)
{
	if (worldX > terrain.size || worldZ > terrain.size)
		return INT_MIN;

	float localX = worldX / terrain.size;
	float localZ = worldZ / terrain.size;
	float gridSquareSize = 1.f / terrain.samples;
	int gridX = std::floor(localX / gridSquareSize);
	int gridZ = std::floor(localZ / gridSquareSize);
	float xCoord = fmod(localX, gridSquareSize) / gridSquareSize;
	float zCoord = fmod(localZ, gridSquareSize) / gridSquareSize;
	if (xCoord < 1 - zCoord)
	{
		return getBaryCentricHeight(
			glm::vec3(0, terrain.heights[gridZ * terrain.samples + gridX], 0),
			glm::vec3(1, terrain.heights[gridZ * terrain.samples + (gridX + 1)], 0),
			glm::vec3(0, terrain.heights[(gridZ + 1) * terrain.samples + gridX], 1),
			xCoord, zCoord
		);
	}
	return getBaryCentricHeight(
		glm::vec3(1, terrain.heights[(gridZ) * terrain.samples + (gridX + 1)], 0),
		glm::vec3(1, terrain.heights[(gridZ + 1) * terrain.samples + (gridX + 1)], 1),
		glm::vec3(0, terrain.heights[(gridZ + 1) * terrain.samples + (gridX)], 1),
		xCoord, zCoord
	);
}

//Container for a 3 float vector
struct Vector3
{
	float x, y, z;

	Vector3 operator+(const Vector3& vector)
	{
		return { x + vector.x, y + vector.y, z + vector.z };
	}

	template<class T>
	Vector3 operator/(const T& t)
	{
		return { x / t, y / t, z / t };
	}
};

//Returns the normal vector of a triangle
glm::vec3 getTriangleNormal(Vector3 v1, Vector3 v2, Vector3 v3)
{
	glm::vec3 normalizedTriVectors1[2] = {
		glm::vec3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z),
		glm::vec3(v2.x - v3.x, v2.y - v3.y, v2.z - v3.z)
	};
	return glm::cross(normalizedTriVectors1[1], normalizedTriVectors1[0]);
}

//Retunns the vertices of a terrain quad based on the terrain x and y positions
void getQuadVertices(Vector3 vectors[4], float xpos, float zpos, float d)
{
	vectors[0] = { xpos + d, getTerrainHeight(xpos + d, zpos + d), zpos + d };
	vectors[1] = { xpos, getTerrainHeight(xpos, zpos + d), zpos + d };
	vectors[2] = { xpos, getTerrainHeight(xpos, zpos), zpos };
	vectors[3] = { xpos + d, getTerrainHeight(xpos + d, zpos), zpos };
}

//Generates a terrain -- TODO: Implement normal smoothing
void generateTerrain(Terrain& terrain, int size, int samples, bool smoothNormals)
{
	terrain.size = size;
	terrain.samples = samples;
	terrain.heights.resize(samples * samples);

	std::vector<Vector3> vertices(samples * samples * 6);
	std::vector<Vector3> normals(samples * samples * 6);

	float xpos = 0, ypos = 0, zpos = 0, d = 1 / (float)samples;
	int index = 0;
	for (int z = 0; z < samples; z++)
	{
		for (int x = 0; x < samples; x++)
		{
			Vector3 quad[4];
			getQuadVertices(quad, xpos, zpos, d);

			vertices[index] = quad[0];
			vertices[index + 1] = quad[1];
			vertices[index + 2] = quad[2];
			vertices[index + 3] = quad[2];
			vertices[index + 4] = quad[3];
			vertices[index + 5] = quad[0];

			glm::vec3 normal1 = getTriangleNormal(quad[0], quad[1], quad[2]);
			glm::vec3 normal2 = getTriangleNormal(quad[2], quad[3], quad[0]);

			normals[index] = { normal1.x, normal1.y, normal1.z };
			normals[index + 1] = { normal1.x, normal1.y, normal1.z };
			normals[index + 2] = { normal1.x, normal1.y, normal1.z };
			normals[index + 3] = { normal2.x, normal2.y, normal2.z };
			normals[index + 4] = { normal2.x, normal2.y, normal2.z };
			normals[index + 5] = { normal2.x, normal2.y, normal2.z };

			terrain.heights[z * samples + x] = getTerrainHeight(xpos, zpos);

			xpos += d;
			index += 6;
		}
		xpos = 0;
		zpos += d;
	}

	terrain.v_vbo = genArrayVBO(vertices.size() * sizeof(Vector3), &vertices[0]);
	terrain.n_vbo = genArrayVBO(normals.size() * sizeof(Vector3), &normals[0]);
	terrain.vao = 0;
}

//Renders a terrain object to screen
void drawTerrain(GLuint textures[4], Terrain terrain)
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, textures[2]);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, textures[3]);

	glBindVertexArray(terrain.vao);
	glDrawArrays(GL_TRIANGLES, 0, terrain.samples * terrain.samples * 6);
}

int main()
{
	//Defines random seed to be used by the rand function and loads the terrain height map
	srand(time(NULL));
	heightMap.loadFromFile("images/heightMap.png");

	//Initializes the window and it's properties
	bool wireframe = false;
	sf::ContextSettings settings;
	settings.antialiasingLevel = 8;
	settings.majorVersion = 3;
	settings.minorVersion = 3;
	settings.depthBits = 24;
	settings.stencilBits = 8;
	sf::RenderWindow window(sf::VideoMode(1920, 1080), "SFML Thing", sf::Style::Default, settings);

	//Initalizes OpenGL and enables neccesarry functionality
	if (glewInit() != GLEW_OK)
	{
		return GLEW_INIT_FAILURE;
	}
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0625f, 0.304f, 0.519f, 1.f);

	//Creates a new vao for the terrain
	Terrain terrain;
	generateTerrain(terrain, 50, 256, true);
	VAOslot tvaoSlots[2];
	tvaoSlots[0].vbo = terrain.v_vbo;
	tvaoSlots[0].index = 0;
	tvaoSlots[0].vs = 3;
	tvaoSlots[0].type = GL_FLOAT;
	tvaoSlots[0].stride = sizeof(Vector3);
	tvaoSlots[0].offset = (void*)0;
	tvaoSlots[1].vbo = terrain.n_vbo;
	tvaoSlots[1].index = 1;
	tvaoSlots[1].vs = 3;
	tvaoSlots[1].type = GL_FLOAT;
	tvaoSlots[1].stride = 3 * sizeof(GLfloat);
	tvaoSlots[1].offset = (void*)0;
	terrain.vao = genVAO(tvaoSlots, 2);
	glm::mat4 terrainModel = glm::mat4(1.f);
	terrainModel = glm::scale(terrainModel, glm::vec3(terrain.size, 1.f, terrain.size));

	//Creates all of the shaders
	GLuint terrainShader, floraShader, basicShader, depthPassShader, depthPassInstShader;
	terrainShader = genShaderProgram(loadShader("Shaders/terrainVertexShader.glsl", GL_VERTEX_SHADER), loadShader("Shaders/terrainFragmentShader.glsl", GL_FRAGMENT_SHADER));
	floraShader = genShaderProgram(loadShader("Shaders/modelVertexShader.glsl", GL_VERTEX_SHADER), loadShader("Shaders/modelFragmentShader.glsl", GL_FRAGMENT_SHADER));
	basicShader = genShaderProgram(loadShader("Shaders/basicVertexShader.glsl", GL_VERTEX_SHADER), loadShader("Shaders/basicFragmentShader.glsl", GL_FRAGMENT_SHADER));
	depthPassShader = genShaderProgram(loadShader("Shaders/depthVertexShader.glsl", GL_VERTEX_SHADER), loadShader("Shaders/depthFragmentShader.glsl", GL_FRAGMENT_SHADER));
	depthPassInstShader = genShaderProgram(loadShader("Shaders/depthVertexShader2.glsl", GL_VERTEX_SHADER), loadShader("Shaders/depthFragmentShader2.glsl", GL_FRAGMENT_SHADER));

	//Loads all the terrain textures and the sun texture
	GLuint terrainTextures[4];
	terrainTextures[0] = loadTexture("images/TerrainTextureMap.png", GL_RGBA);
	terrainTextures[1] = loadTexture("images/PathTexture.png");
	terrainTextures[2] = loadTexture("images/DirtTexture.png");
	terrainTextures[3] = loadTexture("images/GrassTexture.jpg");
	glUseProgram(terrainShader);
	glUniform1i(glGetUniformLocation(terrainShader, "textureMap"), 0);
	glUniform1i(glGetUniformLocation(terrainShader, "texture_r"), 1);
	glUniform1i(glGetUniformLocation(terrainShader, "texture_g"), 2);
	glUniform1i(glGetUniformLocation(terrainShader, "texture_b"), 3);
	glUniform1i(glGetUniformLocation(terrainShader, "depthMap"), 4);
	GLuint sunTexture = loadTexture("images/sun.png", GL_RGBA);

	//Creates a VAO for the tree model
	OBJ treeOBJ;
	loadOBJ("models/obj/OakTree1.obj", treeOBJ);
	VAOslot treeVaoSlots[3];
	treeVaoSlots[0].vbo = treeOBJ.v_vbo;
	treeVaoSlots[0].index = 0;
	treeVaoSlots[0].vs = 3;
	treeVaoSlots[0].type = GL_FLOAT;
	treeVaoSlots[0].stride = 3 * sizeof(GLfloat);
	treeVaoSlots[0].offset = (void*)0;
	treeVaoSlots[1].vbo = treeOBJ.n_vbo;
	treeVaoSlots[1].index = 1;
	treeVaoSlots[1].vs = 3;
	treeVaoSlots[1].type = GL_FLOAT;
	treeVaoSlots[1].stride = 3 * sizeof(GLfloat);
	treeVaoSlots[1].offset = (void*)0;
	treeVaoSlots[2].vbo = treeOBJ.u_vbo;
	treeVaoSlots[2].index = 2;
	treeVaoSlots[2].vs = 2;
	treeVaoSlots[2].type = GL_FLOAT;
	treeVaoSlots[2].stride = 2 * sizeof(GLfloat);
	treeVaoSlots[2].offset = (void*)0;
	treeOBJ.vao = genVAO(treeVaoSlots, 3);
	glm::mat4 treeModel = glm::mat4(1.f);
	glUseProgram(floraShader);
	GLuint treeTexture = loadTexture("images/tree/TreeTexture.png");
	glUniform1i(glGetUniformLocation(terrainShader, "tex"), 0);
	glUniform1i(glGetUniformLocation(terrainShader, "depthMap"), 1);
	//Creates all of the trees in the scene scattered and rotated randomly 
	glm::mat4 positions[50];
	for (int i = 0; i < 50; i++)
	{
		float x = rand() / (float)RAND_MAX + rand() % (terrain.size - 1);
		float z = rand() / (float)RAND_MAX + rand() % (terrain.size - 1);
		float rot = rand() % 360 * 3.14f / 180.f;
		float bias = 0.5;
		glm::vec3 pos = glm::vec3(x, getTerrainCollisionHeight(terrain, x, z) - bias, z);
		positions[i] = glm::translate(glm::mat4(1.f), pos);
		positions[i] = glm::rotate(positions[i], rot, glm::vec3(0, 1, 0));
		positions[i] = glm::scale(positions[i], glm::vec3(2, 2, 2));
		glUseProgram(floraShader);
		glUniformMatrix4fv(glGetUniformLocation(floraShader, ("models[" + std::to_string(i) + "]").c_str()), 1, GL_FALSE, &positions[i][0][0]);
		glUseProgram(depthPassInstShader);
		glUniformMatrix4fv(glGetUniformLocation(depthPassInstShader, ("models[" + std::to_string(i) + "]").c_str()), 1, GL_FALSE, &positions[i][0][0]);
	}

	//Creates a new vao that only contains the vertices of a quad
	VAOslot quadSlot;
	quadSlot.index = 0;
	quadSlot.offset = (void*)0;
	quadSlot.stride = 3 * sizeof(GLfloat);
	quadSlot.type = GL_FLOAT;
	quadSlot.vs = 3;
	quadSlot.vbo = genArrayVBO(sizeof(quadVertices), quadVertices);
	GLuint quadVAO = genVAO(&quadSlot, 1);

	//Defines the depth texture and its parameters for the depth render framebuffer
	GLuint shadowResolution = 2048;
	GLuint depthTexture;
	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	float border[] = { 1.f, 1.f, 1.f, 1.f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowResolution, shadowResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	//Sets up the depth render framebuffer. Most depth information in the scene is rendered to this buffer
	GLuint framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Camera control variables
	CameraFP cameraFP(glm::vec3(20, 10, 20), 3.f);
	cameraFP.setBounds(glm::vec2(0, terrain.size), glm::vec2(0, terrain.size));
	glm::vec3 lightDir = glm::vec3(-0.2f, -1.0f, -0.3f);
	float g = 9.8f;
	float v = 0.f;
	bool onGround = false;

	sf::Clock clock; //Used for timing purposes

	//Game loop
	while (window.isOpen())
	{
		//Event loop -> Manages key presses and other window events
		float dt = clock.restart().asSeconds();
		sf::Event event;
		while (window.pollEvent(event))
		{
			switch (event.type)
			{
			case sf::Event::Closed:
				window.close();
				break;
			case sf::Event::KeyReleased:
				if (event.key.code == sf::Keyboard::Escape)
					window.close();
				if (event.key.code == sf::Keyboard::Tab)
					wireframe ^= 1; //Toggles wireframe mode if the tab key is released
				break;
			case sf::Event::Resized:
				window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
				glViewport(0, 0, event.size.width, event.size.height);
				break;
			}
		}

		//Turns wireframe on or off
		if (wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//Camera controls and terrain collision detection
		sf::Vector2i mousePos = sf::Mouse::getPosition(window);
		float moffsetx = (mousePos.x - window.getSize().x / 2.f) * 0.05;
		float moffsety = -(mousePos.y - window.getSize().y / 2.f) * 0.05;
		sf::Mouse::setPosition(sf::Vector2i(window.getSize().x / 2, window.getSize().y / 2), window);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && onGround)
		{
			v = -g * 0.5;
			onGround = false;
		}

		v += g * dt;
		float terrainHeight = getTerrainCollisionHeight(terrain, cameraFP.getPosition().x, cameraFP.getPosition().z) + 3.f;
		glm::mat4 projection = glm::perspective(glm::radians(70.f), window.getSize().x / (float)window.getSize().y, 0.5f, 150.f);
		cameraFP.inputProc(event, dt, moffsetx, moffsety);
		cameraFP.move(0, -v * dt, 0);
		if (cameraFP.getPosition().y < terrainHeight)
		{
			cameraFP.setPosition(cameraFP.getPosition().x, terrainHeight, cameraFP.getPosition().z);
			onGround = true;
		}
		cameraFP.activateView();
		glm::mat4 cameraView = cameraFP.getView();

		//Depth pass -> Renders the screen to the depth buffer for shadow mapping
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glClear(GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, shadowResolution, shadowResolution);
		glUseProgram(depthPassShader);
		glm::mat4 lightProj = glm::ortho(-80.f, 80.f, -80.f, 80.f, 2.f, 200.f);
		glm::mat4 lightView = glm::lookAt(glm::vec3(110/1.1f, 50.f, 110 / 1.1f), glm::vec3(0), glm::vec3(0, 1, 0));
		glUniformMatrix4fv(glGetUniformLocation(depthPassShader, "lightSpaceTransform"), 1, GL_FALSE, &(lightProj * lightView)[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(depthPassShader, "model"), 1, GL_FALSE, &terrainModel[0][0]);
		drawTerrain(terrainTextures, terrain);
		glCullFace(GL_FRONT);
		glUseProgram(depthPassInstShader);
		glUniformMatrix4fv(glGetUniformLocation(depthPassInstShader, "lightSpaceTransform"), 1, GL_FALSE, &(lightProj * lightView)[0][0]);
		glBindVertexArray(treeOBJ.vao);
		glDrawArraysInstanced(GL_TRIANGLES, 0, treeOBJ.vertexCount, 15);
		glCullFace(GL_BACK);

		//Render pass -> Renders the scene to the screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, window.getSize().x, window.getSize().y);

		//Renders the terrain
		glUseProgram(terrainShader);
		glUniformMatrix4fv(glGetUniformLocation(terrainShader, "projection"), 1, GL_FALSE, &projection[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(terrainShader, "view"), 1, GL_FALSE, &cameraView[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(terrainShader, "model"), 1, GL_FALSE, &terrainModel[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(terrainShader, "lightSpaceTransform"), 1, GL_FALSE, &(lightProj* lightView)[0][0]);
		glUniform1i(glGetUniformLocation(terrainShader, "terrainSize"), terrain.size);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		drawTerrain(terrainTextures, terrain);

		//Draws all of the treees
		glEnable(GL_CULL_FACE);
		glUseProgram(floraShader);
		glUniformMatrix4fv(glGetUniformLocation(floraShader, "projection"), 1, GL_FALSE, &projection[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(floraShader, "view"), 1, GL_FALSE, &cameraView[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(floraShader, "model"), 1, GL_FALSE, &treeModel[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(floraShader, "lightSpaceTransform"), 1, GL_FALSE, &(lightProj* lightView)[0][0]);
		glUniform3fv(glGetUniformLocation(floraShader, "lightDirection"), 1, &lightDir[0]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, treeTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glBindVertexArray(treeOBJ.vao);
		glDrawArraysInstanced(GL_TRIANGLES, 0, treeOBJ.vertexCount, 15);
		glDisable(GL_CULL_FACE);

		//Renders the sun
		glDepthMask(GL_FALSE);
		glUseProgram(basicShader);
		glm::mat4 proj = projection * cameraView * glm::scale(
			glm::rotate(
				glm::translate(
					glm::mat4(1.f),
					glm::vec3(110, 50.f, 110)),
				glm::radians(45.f),
				glm::vec3(0, 1, 0)
			),
			glm::vec3(30, 30, 1));
		glUniformMatrix4fv(glGetUniformLocation(basicShader, "proj"), 1, GL_FALSE, &proj[0][0]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, sunTexture);
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDepthMask(GL_TRUE);
		
		/*
		 * Renders the depth map to the screen
		basicShader2.bind();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glBindVertexArray(depthQuadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		*/

		window.display();
	}
}