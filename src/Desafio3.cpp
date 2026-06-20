/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 07/03/2025
 */

#include <iostream>
#include <string>
#include <assert.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

string cubePath = "../assets/Modelos3D/Cube.obj";
string monkeyPath = "../assets/Modelos3D/Suzanne.obj";

// Protótipos das funções
int setupShader();
GLuint loadSimpleOBJ(string filePath, int &nVertices);
GLuint generateTexture(string filePath);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar *vertexShaderSource = "#version 450\n"
								   "layout (location = 0) in vec3 position;\n"
								   "layout (location = 1) in vec3 color;\n"
								   "layout (location = 2) in vec2 tex_coord;\n"
								   "uniform mat4 model;\n"
								   "uniform mat4 projection;\n"
								   "out vec4 finalColor;\n"
								   "out vec2 texCoord;\n"
								   "void main()\n"
								   "{\n"
								   //...pode ter mais linhas de código aqui!
								   "gl_Position = model * vec4(position, 1.0);\n"
								   "finalColor = vec4(color, 1.0);\n"
								   "texCoord = vec2(tex_coord.x, tex_coord.y);\n"
								   "}\0";

// Códifo fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar *fragmentShaderSource = "#version 450\n"
									 "in vec4 finalColor;\n"
									 "in vec2 texCoord;\n"
									 "out vec4 color;\n"
									 "uniform sampler2D tex_buffer;\n"
									 "void main()\n"
									 "{\n"
									 "color = texture(tex_buffer, texCoord) * finalColor;\n"
									 "}\n\0";

// Estrutura para armazenar o estado de cada objeto
struct ObjetoTransformacao
{
	float posX = 0;
	float posY = 0;
	float tamanho = 0.5f;
	bool rotateX = false;
	bool rotateY = false;
	bool rotateZ = false;
	GLuint texID;
};

ObjetoTransformacao objetos[2]; // Array para os dois objetos
bool rotateX = false, rotateY = false, rotateZ = false;
float posX = 0, posY = 0, tamanho = 0.5;
int objetoSelecionado = 0;
// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Muita atenção aqui: alguns ambientes não aceitam essas configurações
	// Você deve adaptar para a versão do OpenGL suportada por sua placa
	// Sugestão: comente essas linhas de código para desobrir a versão e
	// depois atualize (por exemplo: 4.5 com 4 e 5)
	// glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	// glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	// glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Essencial para computadores da Apple
	// #ifdef __APPLE__
	//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// #endif

	// Criação da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Ola 3D -- Áron!", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	// Obtendo as informações de versão
	const GLubyte *renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte *version = glGetString(GL_VERSION);	/* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	// Gerando um buffer simples, com a geometria de um triângulo
	GLuint VAO[2];
	int nVerticesArr[2] = {0, 0};

	objetos[0].texID = generateTexture("../assets/tex/pixelWall.png");
	objetos[1].texID = generateTexture("../assets/Modelos3D/Suzanne.png");
	VAO[0] = loadSimpleOBJ(cubePath, nVerticesArr[0]);
	VAO[1] = loadSimpleOBJ(monkeyPath, nVerticesArr[1]);

	glUseProgram(shaderID);
	glUniform1i(glGetUniformLocation(shaderID, "tex_buffer"), 0);

	glm::mat4 model = glm::mat4(1); // matriz identidade;
	GLint modelLoc = glGetUniformLocation(shaderID, "model");
	//
	model = glm::rotate(model, /*(GLfloat)glfwGetTime()*/ glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glEnable(GL_DEPTH_TEST);

	objetos[0].posX = -0.5f;
	objetos[1].posX = +0.5f;

	while (!glfwWindowShouldClose(window))
	{
		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(0);

		float angle = (GLfloat)glfwGetTime();

		// Renderiza ambos os objetos
		for (int i = 0; i < 2; i++)
		{
			glm::mat4 model = glm::mat4(1);

			// Aplica transformações apenas ao objeto selecionado
			if (i == objetoSelecionado)
			{
				// Translação do objeto selecionado (primeira)
				model = glm::translate(model, glm::vec3(objetos[i].posX, objetos[i].posY, 0.0f));

				// Rotações do objeto selecionado (segunda - no eixo local)
				if (objetos[i].rotateX)
				{
					model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
				}
				else if (objetos[i].rotateY)
				{
					model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
				}
				else if (objetos[i].rotateZ)
				{
					model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
				}

				// Escala do objeto selecionado (terceira)
				model = glm::scale(model, glm::vec3(objetos[i].tamanho, objetos[i].tamanho, objetos[i].tamanho));
			}
			else
			{
				// Objeto não selecionado mantém sua última posição
				model = glm::translate(model, glm::vec3(objetos[i].posX, objetos[i].posY, 0.0f));
				model = glm::scale(model, glm::vec3(objetos[i].tamanho, objetos[i].tamanho, objetos[i].tamanho));
			}

			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, objetos[i].texID);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, objetos[i].texID);

			// Desenha o objeto
			glBindVertexArray(VAO[i]);
			glDrawArrays(GL_TRIANGLES, 0, nVerticesArr[i]);
			glDrawArrays(GL_POINTS, 0, nVerticesArr[i]);
			glBindVertexArray(0);
		}

		// Troca os buffers da tela*/
		glfwSwapBuffers(window);
	}
	// Pede pra OpenGL desalocar os buffers
	glDeleteVertexArrays(2, VAO);
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	// Rotação
	if (key == GLFW_KEY_X && action == GLFW_PRESS)
	{
		objetos[objetoSelecionado].rotateX = true;
		objetos[objetoSelecionado].rotateY = false;
		objetos[objetoSelecionado].rotateZ = false;
	}

	if (key == GLFW_KEY_Y && action == GLFW_PRESS)
	{
		objetos[objetoSelecionado].rotateX = false;
		objetos[objetoSelecionado].rotateY = true;
		objetos[objetoSelecionado].rotateZ = false;
	}

	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
	{
		objetos[objetoSelecionado].rotateX = false;
		objetos[objetoSelecionado].rotateY = false;
		objetos[objetoSelecionado].rotateZ = true;
	}

	// Transladação
	if (key == GLFW_KEY_W && action == GLFW_PRESS)
	{
		objetos[objetoSelecionado].posY += 0.1f;
	}

	if (key == GLFW_KEY_S && action == GLFW_PRESS)
	{
		objetos[objetoSelecionado].posY -= 0.1f;
	}

	if (key == GLFW_KEY_A && action == GLFW_PRESS)
	{
		objetos[objetoSelecionado].posX -= 0.1f;
	}

	if (key == GLFW_KEY_D && action == GLFW_PRESS)
	{
		objetos[objetoSelecionado].posX += 0.1f;
	}

	if (key == GLFW_KEY_MINUS && action == GLFW_PRESS)
	{
		if (objetos[objetoSelecionado].tamanho > 0.1f)
		{
			objetos[objetoSelecionado].tamanho -= 0.1f;
		}
	}

	if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS)
	{
		if (objetos[objetoSelecionado].tamanho < 1.9f)
		{
			objetos[objetoSelecionado].tamanho += 0.1f;
		}
	}

	// Seleção do objeto
	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
	{
		objetoSelecionado = 0;
	}
	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
	{
		objetoSelecionado = 1;
	}
}

// Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
//  shader simples e único neste exemplo de código
//  O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
//  fragmentShader source no iniçio deste arquivo
//  A função retorna o identificador do programa de shader
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
				  << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

GLuint generateTexture(string filePath)
{
	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;

	// Corrige a textura invertida!
	stbi_set_flip_vertically_on_load(true);

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) // jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else // png
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D); // geração do mipmap
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}

GLuint loadSimpleOBJ(string filePATH, int &nVertices)
{
	srand(time(NULL));
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> texCoords;
	std::vector<glm::vec3> normals;
	std::vector<GLfloat> vBuffer;
	glm::vec3 color = glm::vec3(0.0, 0.0, 0.0);

	std::ifstream arqEntrada(filePATH.c_str());
	if (!arqEntrada.is_open())
	{
		std::cerr << "Erro ao tentar ler o arquivo " << filePATH << std::endl;
		return -1;
	}

	std::string line;
	while (std::getline(arqEntrada, line))
	{
		std::istringstream ssline(line);
		std::string word;
		ssline >> word;

		if (word == "v")
		{
			glm::vec3 vertice;
			ssline >> vertice.x >> vertice.y >> vertice.z;
			vertices.push_back(vertice);
		}
		else if (word == "vt")
		{
			glm::vec2 vt;
			ssline >> vt.s >> vt.t;
			texCoords.push_back(vt);
		}
		else if (word == "vn")
		{
			glm::vec3 normal;
			ssline >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if (word == "f")
		{
			color.r = 1;
			color.g = 1;
			color.b = 1;
			while (ssline >> word)
			{
				int vi = 0, ti = 0, ni = 0;
				std::istringstream ss(word);
				std::string index;

				if (std::getline(ss, index, '/'))
					vi = !index.empty() ? std::stoi(index) - 1 : 0;
				if (std::getline(ss, index, '/'))
					ti = !index.empty() ? std::stoi(index) - 1 : 0;
				if (std::getline(ss, index))
					ni = !index.empty() ? std::stoi(index) - 1 : 0;

				vBuffer.push_back(vertices[vi].x);
				vBuffer.push_back(vertices[vi].y);
				vBuffer.push_back(vertices[vi].z);
				vBuffer.push_back(color.r);
				vBuffer.push_back(color.g);
				vBuffer.push_back(color.b);
				// Adiciona coordenadas de textura (u,v) se disponível, senão (0,0)
				if (!texCoords.empty() && ti >= 0 && ti < (int)texCoords.size())
				{
					vBuffer.push_back(texCoords[ti].s);
					vBuffer.push_back(texCoords[ti].t);
				}
				else
				{
					vBuffer.push_back(0.0f);
					vBuffer.push_back(0.0f);
				}
			}
		}
	}

	arqEntrada.close();

	std::cout << "Gerando o buffer de geometria..." << std::endl;
	GLuint VBO, VAO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	nVertices = vBuffer.size() / 8; // x, y, z, r, g, b, u, v (valores armazenados por vértice)

	return VAO;
}