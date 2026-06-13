/* Vivencial2
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
#include "../build/_deps/stb_image-src/stb_image.h"

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

// Estrutura para armazenar o estado de cada objeto
struct ObjetoTransformacao
{
	GLuint VAO;
	GLuint texID;
	glm::vec3 posicao;
	glm::vec3 rotacao;
	glm::vec3 tamanho;
	glm::vec3 cor;
};

struct Luz
{
	glm::vec3 posicao;
	glm::vec3 cor;
};

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 1000, HEIGHT = 1000;

struct Camera
{
	// Câmera
	glm::vec3 pos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	bool firstMouse = true;
	float lastX = WIDTH / 2.0, lastY = HEIGHT / 2.0;
	// para calcular o quanto que o mouse deslocou
	float yaw = -90.0, pitch = 0.0; // rotação em x e y
};

// Tempo entre o frame atual e o último frame
float deltaTime = 0.0f;

bool rotateX = false, rotateY = false, rotateZ = false;
float posX = 0, posY = 0, tamanho = 0.1f;
int objetoSelecionado = 0;

std::vector<ObjetoTransformacao> objetos; // Array para os dois objetos
std::vector<Luz> luzes;					  // Array para as luzes
Camera camera;					  // Array para as luzes

// Booleano de luzes ligadas
bool keyLightOn = false;  // Tecla 1
bool fillLightOn = false; // Tecla 2
bool backlightOn = false; // Tecla 3

// Protótipos das funções de callback e controle de câmera
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void updateCameraPos(GLFWwindow *window);

string cubePath = "../assets/Modelos3D/Cube.obj";
string monkeyPath = "../assets/Modelos3D/Suzanne.obj";

// Protótipos das funções
int setupShader();
GLuint generateTexture(string filePath);
GLuint loadSimpleOBJ(string filePath, int &nVertices);
bool fileExists(const string &path);

bool fileExists(const string &path)
{
	std::ifstream file(path);
	return file.good();
}

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar *vertexShaderSource = "#version 450\n"
								   "layout (location = 0) in vec3 position;\n"
								   "layout (location = 1) in vec3 color;\n"
								   "layout (location = 2) in vec3 normal;\n"
								   "layout (location = 3) in vec2 tex_coord;\n"

								   "uniform mat4 model;\n"
								   "uniform mat4 view;\n"
								   "uniform mat4 projection;\n"

								   "out vec4 finalColor;\n"
								   "out vec2 texCoord;\n"
								   "out vec4 fragPos;\n"
								   "out vec3 scaledNormal;\n"

								   "void main()\n"
								   "{\n"
								   "gl_Position = projection * view * model * vec4(position, 1.0);\n"
								   "fragPos = model * vec4(position, 1.0);\n"

								   "scaledNormal = normalize(mat3(transpose(inverse(model))) * normal);\n"

								   "texCoord = vec2(tex_coord.x, tex_coord.y);\n"
								   "finalColor = vec4(color, 1.0);\n"
								   "}\0";

// Códifo fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar *fragmentShaderSource = R"(
#version 450
    in vec4 finalColor;
    in vec2 texCoord;
    in vec4 fragPos;
    in vec3 scaledNormal;

    out vec4 color;

    uniform sampler2D tex_buffer;

    // Propriedades da superfície
    uniform vec3 ka;
    uniform vec3 kd;
    uniform vec3 ks;
    uniform float q;
    // Propriedades da fonte de luz
    struct Luz {
        vec3 posicao;
        vec3 cor;
        bool ligada;
    };
    uniform Luz luzes[3];

    // Posiçãoo da Câmera
    uniform vec3 cameraPos;
    //  uniform float Kc;
    //  uniform float Kl;
    //  uniform float Kq;
    void main()
    {
        vec4 objectColor = texture(tex_buffer, texCoord);
        vec3 N = normalize(scaledNormal);
        vec3 V = normalize(cameraPos - vec3(fragPos));
        vec3 result = vec3(0.0);

        for(int i = 0; i < 3; i++)
        {
            if (luzes[i].ligada == false){
                continue;    
            } 
            vec3 L = normalize(luzes[i].posicao - vec3(fragPos));
            // Fator de Atenuação na reflexão difusa
            float dist = length(luzes[i].posicao - vec3(fragPos));
            float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * (dist * dist));
    
            // Cálculo da parcela de iluminação ambiente
            vec3 ambient = ka * luzes[i].cor;
        
            // Cálculo da parcela de iluminação difusa
            float diff = max(dot(N,L),0.0);
            vec3 diffuse = kd * diff * luzes[i].cor * attenuation;
        
            // Cálculo da parcela de iluminação especular
            vec3 R = normalize(reflect(-L,N));
            float spec = max(dot(R,V),0.0);
            spec = pow(spec,q);
            vec3 specular = ks * spec * luzes[i].cor;
        
            result += (ambient + diffuse) * finalColor.rgb + specular;
        }
        color = vec4(result, 1.0);
    })";

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	// Muita atenção aqui: alguns ambientes não aceitam essas configurações
	// Você deve adaptar para a versão do OpenGL suportada por sua placa
	// Sugestão: comente essas linhas de código para desobrir a versão e
	// depois atualize (por exemplo: 4.5 com 4 e 5)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Essencial para computadores da Apple
	// #ifdef __APPLE__
	//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// #endif

	// Criação da janela GLFW
	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Ola Vivencial2/Desafio 5 -- Áron!", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

	// Configuração do callback de mouse para controle da câmera
	glfwSetCursorPosCallback(window, mouse_callback);

	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();
	glUseProgram(shaderID);

	glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 projection = glm::perspective(45.0f, (GLfloat)WIDTH / (GLfloat)HEIGHT, 0.1f, 100.0f);

	glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
	// Matriz de projeção paralela ortográfica
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3fv(glGetUniformLocation(shaderID, "cameraPos"), 1, glm::value_ptr(camera.pos));

	// Enviar a informação de qual variável armazenará o buffer da textura
	glUniform1i(glGetUniformLocation(shaderID, "tex_buffer"), 0);

	string assetsRoot = "../assets/";
	if (!fileExists(assetsRoot + "Modelos3D/Cube.obj"))
		assetsRoot = "assets/";
	if (!fileExists(assetsRoot + "Modelos3D/Cube.obj"))
	{
		std::cerr << "Erro: não foi possível encontrar assets em '" << assetsRoot << "'" << std::endl;
		return -1;
	}

	// Gerando um buffer simples, com a geometria de um triângulo;
	int nVerticesArr[2] = {0, 0};

	// Objeto 1: cubo
	ObjetoTransformacao objeto1;
	objeto1.VAO = loadSimpleOBJ(assetsRoot + "Modelos3D/Cube.obj", nVerticesArr[0]);
	objeto1.texID = generateTexture(assetsRoot + "tex/pixelWall.png");
	objeto1.posicao = glm::vec3(-0.5f, 0.0f, 0.0f);
	objeto1.rotacao = glm::vec3(false, false, false);
	objeto1.tamanho = glm::vec3(0.3f);
	objetos.push_back(objeto1);

	// Objeto 2: Suzane
	ObjetoTransformacao objeto2;
	objeto2.VAO = loadSimpleOBJ(assetsRoot + "Modelos3D/Suzanne.obj", nVerticesArr[1]);
	objeto2.texID = generateTexture(assetsRoot + "tex/pixelWall.png");
	objeto2.posicao = glm::vec3(0.5f, 0.0f, 0.0f);
	objeto2.rotacao = glm::vec3(false, false, false);
	objeto2.tamanho = glm::vec3(0.3f);
	objetos.push_back(objeto2);

	glm::vec3 ka = glm::vec3(0.1f);
	glm::vec3 kd = glm::vec3(0.6f);
	glm::vec3 ks = glm::vec3(0.5f);
	float q = 32.0f;

	Luz keyLight;
	keyLight.posicao = glm::vec3(-1.0f, 0.0f, -1.5f);
	keyLight.cor = glm::vec3(1.0f, 1.0f, 1.0f);
	luzes.push_back(keyLight);

	Luz fillLight;
	fillLight.posicao = glm::vec3(1.0f, 0.0f, -1.5f);
	fillLight.cor = glm::vec3(0.5f, 0.5f, 0.5f);
	luzes.push_back(fillLight);

	Luz backlight;
	backlight.posicao = glm::vec3(-0.5f, 1.0f, -0.8f);
	backlight.cor = glm::vec3(1.0f, 1.0f, 0.5f);
	luzes.push_back(backlight);

	// Configuração da Atenuação ---
	// Estes valores representam uma luz que cobre uma distância de cerca de 50 unidades.
	// Você pode ajustá-los para fazer a luz ir mais ou menos longe.
	// glUniform1f(glGetUniformLocation(shaderID, "Kc"), 1.0f);   // Constante
	// glUniform1f(glGetUniformLocation(shaderID, "Kl"), 0.09f);  // Linear
	// glUniform1f(glGetUniformLocation(shaderID, "Kq"), 0.032f); // Quadrática

	glUniform3fv(glGetUniformLocation(shaderID, "ka"), 1, glm::value_ptr(ka));
	glUniform3fv(glGetUniformLocation(shaderID, "kd"), 1, glm::value_ptr(kd));
	glUniform3fv(glGetUniformLocation(shaderID, "ks"), 1, glm::value_ptr(ks));
	glUniform1f(glGetUniformLocation(shaderID, "q"), q);

	glEnable(GL_DEPTH_TEST);

	float lastFrame = glfwGetTime();

	while (!glfwWindowShouldClose(window))
	{
		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		view = glm::lookAt(camera.pos, camera.pos + camera.front, camera.up);
		glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));

		// Limpa o buffer de cor
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(0);

		// Key Light
		glUniform3fv(glGetUniformLocation(shaderID, "luzes[0].posicao"), 1, glm::value_ptr(luzes[0].posicao));
		glUniform3fv(glGetUniformLocation(shaderID, "luzes[0].cor"), 1, glm::value_ptr(luzes[0].cor));
		glUniform1i(glGetUniformLocation(shaderID, "luzes[0].ligada"), keyLightOn);

		// Fill Light
		glUniform3fv(glGetUniformLocation(shaderID, "luzes[1].posicao"), 1, glm::value_ptr(luzes[1].posicao));
		glUniform3fv(glGetUniformLocation(shaderID, "luzes[1].cor"), 1, glm::value_ptr(luzes[1].cor));
		glUniform1i(glGetUniformLocation(shaderID, "luzes[1].ligada"), fillLightOn);

		// Back Light
		glUniform3fv(glGetUniformLocation(shaderID, "luzes[2].posicao"), 1, glm::value_ptr(luzes[2].posicao));
		glUniform3fv(glGetUniformLocation(shaderID, "luzes[2].cor"), 1, glm::value_ptr(luzes[2].cor));
		glUniform1i(glGetUniformLocation(shaderID, "luzes[2].ligada"), backlightOn);

		float angle = (GLfloat)glfwGetTime();

		// Renderiza ambos os objetos
		for (int i = 0; i < 2; i++)
		{
			glm::mat4 model = glm::mat4(1.0f);
			// Translação
			model = glm::translate(model, objetos[i].posicao);

			// Rotações (aplicadas se este for o objeto selecionado)
			if (i == objetoSelecionado)
			{
				if (objetos[i].rotacao.x)
					model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
				if (objetos[i].rotacao.y)
					model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
				if (objetos[i].rotacao.z)
					model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
			}

			// Escala
			model = glm::scale(model, objetos[i].tamanho);

			glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));

			// Bind da textura e VAO específicos do objeto i
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, objetos[i].texID);

			glBindVertexArray(objetos[i].VAO);
			glDrawArrays(GL_TRIANGLES, 0, nVerticesArr[i]);
			glBindVertexArray(0);
		}

		// Troca os buffers da tela*/
		glfwSwapBuffers(window);
	}
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
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
		GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
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
			color.r = (rand() % 100 + 1) / 100.0f;
			color.g = (rand() % 100 + 1) / 100.0f;
			color.b = (rand() % 100 + 1) / 100.0f;
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
				// Adiciona normal (se disponível) e coordenadas de textura (u,v)
				if (!normals.empty() && ni >= 0 && ni < (int)normals.size())
				{
					vBuffer.push_back(normals[ni].x);
					vBuffer.push_back(normals[ni].y);
					vBuffer.push_back(normals[ni].z);
				}
				else
				{
					vBuffer.push_back(0.0f);
					vBuffer.push_back(0.0f);
					vBuffer.push_back(0.0f);
				}
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

	// Layout da posição (location 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid *)(0));
	glEnableVertexAttribArray(0);

	// Layout da cor (location 1)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// Layout da normal (location 2)
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid *)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// Layout da UV (location 3)
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid *)(9 * sizeof(GLfloat)));
	glEnableVertexAttribArray(3);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	nVertices = vBuffer.size() / 11; // x, y, z, r, g, b, nx, ny, nz, u, v (valores armazenados por vértice)

	return VAO;
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
		objetos[objetoSelecionado].rotacao.x = true;
		objetos[objetoSelecionado].rotacao.y = false;
		objetos[objetoSelecionado].rotacao.z = false;
	}

	if (key == GLFW_KEY_Y && action == GLFW_PRESS)
	{
		objetos[objetoSelecionado].rotacao.x = false;
		objetos[objetoSelecionado].rotacao.y = true;
		objetos[objetoSelecionado].rotacao.z = false;
	}

	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
	{
		objetos[objetoSelecionado].rotacao.x = false;
		objetos[objetoSelecionado].rotacao.y = false;
		objetos[objetoSelecionado].rotacao.z = true;
	}

	updateCameraPos(window);

	if (key == GLFW_KEY_MINUS && action == GLFW_PRESS)
	{
		if (objetos[objetoSelecionado].tamanho.x > 0.1f)
		{
			objetos[objetoSelecionado].tamanho -= 0.1f;
		}
	}

	if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS)
	{
		if (objetos[objetoSelecionado].tamanho.x < 1.9f)
		{
			objetos[objetoSelecionado].tamanho += 0.1f;
		}
	}

	// Seleção do objeto
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
	{
		objetoSelecionado = (objetoSelecionado == 1) ? 0 : 1;
	}
	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
		keyLightOn = !keyLightOn;
	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
		fillLightOn = !fillLightOn;
	if (key == GLFW_KEY_3 && action == GLFW_PRESS)
		backlightOn = !backlightOn;
}

void updateCameraPos(GLFWwindow *window)
{
	float cameraSpeed = 2.5 * deltaTime; // adjust accordingly

	glm::vec3 front;
	front.x = cos(glm::radians(camera.pitch)) * cos(glm::radians(camera.yaw));
	front.y = sin(glm::radians(camera.pitch));
	front.z = cos(glm::radians(camera.pitch)) * sin(glm::radians(camera.yaw));
	camera.front = glm::normalize(front);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.pos += cameraSpeed * camera.front;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.pos -= cameraSpeed * camera.front;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.pos -= glm::normalize(glm::cross(camera.front, camera.up)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.pos += glm::normalize(glm::cross(camera.front, camera.up)) * cameraSpeed;
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
	if (camera.firstMouse)
	{
		camera.lastX = xpos;
		camera.lastY = ypos;
		camera.firstMouse = false;
	}
	float xoffset = xpos - camera.lastX;
	float yoffset = camera.lastY - ypos;
	camera.lastX = xpos;
	camera.lastY = ypos;
	float sensitivity = 0.05;
	xoffset *= sensitivity;
	yoffset *= sensitivity;
	camera.yaw += xoffset;
	camera.pitch += yoffset;
	if (camera.pitch > 89.0f)
		camera.pitch = 89.0f;
	if (camera.pitch < -89.0f)
		camera.pitch = -89.0f;
	glm::vec3 front;
	front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
	front.y = sin(glm::radians(camera.pitch));
	front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
	camera.front = glm::normalize(front);
	glm::vec3 right = glm::normalize(glm::cross(camera.front, glm::vec3(0.0, 1.0, 0.0)));
	camera.up = glm::normalize(glm::cross(right, camera.front));
}