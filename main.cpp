#include <iostream>
#include <string>
#include <queue>
#include <algorithm>
#include <set>
#include <vector>
#include <map>
#include <chrono>
#include <thread>

#include "debug_utils.hpp"
#include "Shader.h"
#include "Compute_Shader.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

#define TEX_N 64
#define CELL 32
#define VSYNC 0
#define STEPS 4

struct shaderData{
	glm::ivec2 cells[TEX_N*TEX_N];
	GLuint start_idx[CELL*CELL];
	GLfloat values[14];
};


int SCREEN_WIDTH = 1200;
int SCREEN_HEIGHT = 1200;

int button = 0;
int pause = 0;
int little_step = 0;
bool sync_sim = 0;
glm::vec2 mousePos;

ComputeShader cs_particles;
ComputeShader cs_gradient;
ComputeShader cs_density;
ComputeShader cs_init;
shaderData sd;

const char* const screen_vert = "shaders/screen_vert.glsl";
const char* const screen_frag = "shaders/screen_frag.glsl";

const GLfloat quad_vertices[] =
{
	-1.0f, -1.0f , 0.0f, 0.0f, 0.0f,
	-1.0f,  1.0f , 0.0f, 0.0f, 1.0f,
	 1.0f,  1.0f , 0.0f, 1.0f, 1.0f,
	 1.0f, -1.0f , 0.0f, 1.0f, 0.0f,
};

const GLuint quad_indices[] =
{
	0, 2, 1,
	0, 3, 2
};

GLuint particlesTex, dens_gradTex, DBO;

void optionsWindow(){
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	static double curTime = glfwGetTime();
	static double accDeltaTime = 0.0;
	static double avgDeltaTime = 1.0;
	static int cnt = 1;
	// MainWindow::ShowExampleAppCustomRendering();

	ImGui::SetNextWindowSize(ImVec2(400,390));
	int windowFlags = ImGuiWindowFlags_NoResize;
	// int windowFalgs = 0;
	ImGui::Begin("Options", 0, windowFlags);
	{
		double deltaTime = glfwGetTime() - curTime;
		curTime = glfwGetTime();

		accDeltaTime += deltaTime;
		if(accDeltaTime >= 0.25){
			avgDeltaTime = (accDeltaTime / cnt);
			accDeltaTime = 0.0;
			cnt = 0;
		}
		
		ImGui::Text("FPS: %d", (int)(1.0 / avgDeltaTime));
		ImGui::Text("x: %lf y: %lf", mousePos.x, mousePos.y);
		if(ImGui::Button("Shaders")){
			button = 1;
		}
		ImGui::Checkbox("sync sim", &sync_sim);
		ImGui::SliderFloat("size v[7]", &sd.values[7], 0.025, 1.0);
		ImGui::SliderFloat("gravity v[8]", &sd.values[8], 0.0, 10.0);
		ImGui::SliderFloat("pressure v[9]", &sd.values[9], 0.0, 1.0);
		ImGui::SliderFloat("p factor v[10]", &sd.values[10], 0.0, 2.0);
		ImGui::SliderFloat("radius v[11]", &sd.values[11], 0.0, 0.125);
		ImGui::SliderFloat("f drag v[12]", &sd.values[13], 0.995, 0.999);

		// MainWindow::drawCanvas();
	}
	ImGui::End();

	cnt++;
}

void renderImguiFrame(GLFWwindow* window) {
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// start UI frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// setup dockspace of main glfw window
	// this makes glfw window dockable but still see-thru. remove flag if opaque background is needed
	// also must come befor any window declaration
	ImGui::DockSpaceOverViewport(0, ImGuiDockNodeFlags_PassthruCentralNode);
	
	// drawOptions MUST come before drawViewport
	optionsWindow();
	
	// Renders the ImGUI elements
	ImGui::Render();

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}

void terminateImgui() {
	// Deletes all ImGUI instances
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

int initImgui(GLFWwindow* window){
	// Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;    // Enable mouse Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	io.ConfigWindowsMoveFromTitleBarOnly = true;
	io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", 20);
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
	//	ImGuiDockNodeFlags_PassthruCentralNode
	return 0;
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	mousePos = glm::vec2(xpos, (SCREEN_HEIGHT - ypos));
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{   // make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	SCREEN_HEIGHT = height;
	SCREEN_WIDTH = width;
	glViewport(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
	return;
}

int initGLFW(GLFWwindow* &window, string program_window_name) {
	if (!glfwInit()) return 1;

#ifdef __APPLE__
	/* We need to explicitly ask for a 3.2 context on OS X */
	glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

	glfwWindowHint(GLFW_SAMPLES, 4);

	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, program_window_name.c_str(), NULL, NULL);
	if (window == NULL)
	{
		cout << "Failed to create GLFW window" << endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(VSYNC);

	// glfwSetErrorCallback(glfw_error_callback);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	// glfwSetScrollCallback(window, scroll_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);

	// glad: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cout << "Failed to initialize GLAD" << endl;
		return -1;
	}

	GLFWimage images[1]; 
	images[0].pixels = stbi_load("fluid.png", &images[0].width, &images[0].height, 0, 4); //rgba channels 
	glfwSetWindowIcon(window, 1, images); 
	stbi_image_free(images[0].pixels);
	
	return 0;
}

void drawBackground(){
	static Shader sh("shaders/simple.glsl");
	static vector<GLfloat> positions; 
	static GLuint positions_VBO = 0;

	glBindVertexArray(0);

	if(!positions_VBO){
		int qnt_x = 6;
		int qnt_y = 6;
		float sq_w, sq_h, sq_x, sq_y;
		float center_offset = 0.2f;
		sq_w = SCREEN_WIDTH * (1.0f - center_offset * 2.0f);
		sq_h = SCREEN_HEIGHT * (1.0f - center_offset * 2.0f);
		sq_x = SCREEN_WIDTH * center_offset;
		sq_y = SCREEN_HEIGHT * center_offset;

		float px = 0.0, py = 0.0;
		for(int i = 0; i <= qnt_y; i++){
			for(int j = 0; j <= qnt_x; j++){
				positions.push_back(px);
				positions.push_back(py);
				px += sq_w / qnt_x;
			}
			px = 0.0;
			py += sq_w / qnt_y;
		}

		for(int i = 0; i < positions.size(); i+=2){
			positions[i] += sq_x;
			positions[i+1] += sq_y;
		}

		for(int i = 0; i < positions.size(); i++){
			positions[i] = (positions[i] / SCREEN_WIDTH) * 2 - 1.0f; 
		}

		glGenBuffers(1, &positions_VBO);
		glBindBuffer(GL_ARRAY_BUFFER, positions_VBO);
		glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat), positions.data(), GL_DYNAMIC_DRAW);
	}

	// rgb(255, 210, 150)
	glClearColor(35.0f/255, 34.0f/255, 54.0f/255, 1.0f);
	// glClearColor(255.0f/255, 240.0f/255, 235.0f/255, 1.0f);
	glPointSize(3.0f);

	// enable shader
	sh.use();
	// bind the data that will be passed to attribute 0
	glBindBuffer(GL_ARRAY_BUFFER, positions_VBO);
	// substitute data if needeed
	// glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(GLfloat), positions.data());
	// enable attribute 0 for next draw call
	glEnableVertexAttribArray(0);
	// define the attribute caracteristics (type, size, stride, etc...)
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	// draw poits
	glDrawArrays(GL_POINTS, 0, (int)positions.size()/2);
	// unbind data buffer
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(0);
}

void drawParticles(){
	static Shader sh("shaders/particle.glsl");
	static GLuint VAO, VBO, EBO;
	
	if(!VAO || !VBO || !EBO){
		glCreateVertexArrays(1, &VAO);
		glCreateBuffers(1, &VBO);
		glCreateBuffers(1, &EBO);

		glNamedBufferData(VBO, sizeof(quad_vertices), quad_vertices, GL_DYNAMIC_DRAW);
		glNamedBufferData(EBO, sizeof(quad_indices), quad_indices, GL_DYNAMIC_DRAW);

		glEnableVertexArrayAttrib(VAO, 0);
		glVertexArrayAttribBinding(VAO, 0, 0);
		glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);

		glEnableVertexArrayAttrib(VAO, 1);
		glVertexArrayAttribBinding(VAO, 1, 0);
		glVertexArrayAttribFormat(VAO, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat));

		glVertexArrayVertexBuffer(VAO, 0, VBO, 0, 5 * sizeof(GLfloat));
		glVertexArrayElementBuffer(VAO, EBO);
	}

	sh.use();
	glBindTextureUnit(0, particlesTex);
	sh.setInt("parts", 0);
	glBindTextureUnit(1, dens_gradTex);
	sh.setInt("density", 1);
	glBindVertexArray(VAO);
	glDrawElementsInstanced(
		GL_TRIANGLES,
		sizeof(quad_indices) / sizeof(quad_indices[0]),
		GL_UNSIGNED_INT,
		0,
		TEX_N * TEX_N
	);
}

void drawTextureQuad(GLuint texture){
	static GLuint VAO, VBO, EBO;
	static Shader sh("shaders/screen.glsl");
	
	if(!VAO || !VBO || !EBO){
		glCreateVertexArrays(1, &VAO);
		glCreateBuffers(1, &VBO);
		glCreateBuffers(1, &EBO);

		glNamedBufferData(VBO, sizeof(quad_vertices), quad_vertices, GL_DYNAMIC_DRAW);
		glNamedBufferData(EBO, sizeof(quad_indices), quad_indices, GL_DYNAMIC_DRAW);

		glEnableVertexArrayAttrib(VAO, 0);
		glVertexArrayAttribBinding(VAO, 0, 0);
		glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);

		glEnableVertexArrayAttrib(VAO, 1);
		glVertexArrayAttribBinding(VAO, 1, 0);
		glVertexArrayAttribFormat(VAO, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat));

		glVertexArrayVertexBuffer(VAO, 0, VBO, 0, 5 * sizeof(GLfloat));
		glVertexArrayElementBuffer(VAO, EBO);
	}

	sh.use();
	glBindTextureUnit(0, texture);
	sh.setInt("tex", 0);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, sizeof(quad_indices) / sizeof(quad_indices[0]), GL_UNSIGNED_INT, 0);
}

void drawCircle(glm::vec2 pos, float radius){
	static Shader sh("shaders/simple.glsl");
	static GLuint positions_VBO = 0;
	vector<GLfloat> positions; 
	positions.push_back((pos.x / SCREEN_WIDTH) * 2 - 1.0);
	positions.push_back((pos.y / SCREEN_HEIGHT) * 2 - 1.0);

	glBindVertexArray(0);

	if(!positions_VBO){
		glGenBuffers(1, &positions_VBO);
		glBindBuffer(GL_ARRAY_BUFFER, positions_VBO);
		glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat), positions.data(), GL_DYNAMIC_DRAW);
	}
	else{
		glBindBuffer(GL_ARRAY_BUFFER, positions_VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(GLfloat), positions.data());
	}

	glPointSize(radius);

	// enable shader
	sh.use();
	// bind the data that will be passed to attribute 0
	glBindBuffer(GL_ARRAY_BUFFER, positions_VBO);
	// substitute data if needeed
	// glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(GLfloat), positions.data());
	// enable attribute 0 for next draw call
	glEnableVertexAttribArray(0);
	// define the attribute caracteristics (type, size, stride, etc...)
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	// draw poits
	glDrawArrays(GL_POINTS, 0, (int)positions.size()/2);
	// unbind data buffer
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(0);
}

void initTexture(GLuint* texture, int unit, int WIDTH, int HEIGHT){
	glCreateTextures(GL_TEXTURE_2D, 1, texture);
	glTextureParameteri(*texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(*texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(*texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(*texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(*texture, 1, GL_RGBA32F, WIDTH, HEIGHT);
	glBindImageTexture(unit, *texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
}

GLuint buildShaderDataBuffer(GLuint binding = 3){
	GLuint DBO;
	glGenBuffers(1, &DBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, DBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(shaderData), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, DBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	return DBO;
}

void step(shaderData* sd, ComputeShader* part, ComputeShader* dens, ComputeShader* grad, ComputeShader* debug = 0);
void init(shaderData* sd, ComputeShader* init);

void sort_grid(shaderData* sd){
	sort(sd->cells,sd->cells + TEX_N*TEX_N, [&](glm::ivec2 i, glm::ivec2 j){
		if(i.x == j.x) return i.y < j.y;
		return i.x < j.x;
	});
	for(int i = 0; i < CELL*CELL; i++){
		sd->start_idx[i] = 0x3f3f3f3f;
	}
	for(int i = 0; i < TEX_N*TEX_N; i++){
		if(i > 0 && sd->cells[i].x == sd->cells[i-1].x){
			continue;
		}
		sd->start_idx[sd->cells[i].x] = i;
	}
}

int main(int argc, char** argv) {
	vector<string> args = hut::readArgs(argc, argv);
	cout << "args: ";
	hut::printList(args);
	cout << endl;

	GLFWwindow* window = NULL;

	// DO THIS ONE FIRST!!!1!!
	initGLFW(window, "Hoff - fluid");
	initImgui(window);
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	cs_particles = ComputeShader("shaders/density_compute.glsl", "particleKernel");
	cs_density = ComputeShader("shaders/density_compute.glsl", "densityKernel");
	cs_init = ComputeShader("shaders/density_compute.glsl", "initKernel");
	sd.values[0] = 0;
	sd.values[1] = 0;
	sd.values[2] = 100.4;
	sd.values[3] = 0.1;
	sd.values[5] = SCREEN_WIDTH;
	sd.values[6] = SCREEN_HEIGHT;
	sd.values[7] = 0.06;
	sd.values[8] = 6.5;
	sd.values[9] = 1.0;
	sd.values[10] = 1.0;
	sd.values[11] = 1.0/CELL; //(0.125);
	sd.values[12] = 0.1;
	sd.values[13] = 0.995;

	DBO = buildShaderDataBuffer(3);

//	debug -

	// GLuint densitycs_density_global = ComputeShader("shaders/density_compute_global.glsl");

//	--

	initTexture(&particlesTex, 0, TEX_N, TEX_N);
	initTexture(&dens_gradTex, 1, TEX_N, TEX_N);

	glEnable(GL_CULL_FACE);  
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable( GL_BLEND );
	glEnable(GL_MULTISAMPLE); 

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, DBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, DBO);
	
	init(&sd, &cs_init);
	
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(shaderData), &sd);
	
	sort_grid(&sd);

	while(!glfwWindowShouldClose(window)){

		if(button){
			button = 0;
			cs_particles = ComputeShader("shaders/density_compute.glsl", "particleKernel");
			cs_density = ComputeShader("shaders/density_compute.glsl", "densityKernel");
			cs_init = ComputeShader("shaders/density_compute.glsl", "initKernel");

			// cs_density_global = ComputeShader("shaders/density_compute_global.glsl");

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, DBO);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, DBO);
			init(&sd, &cs_init);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(shaderData), &sd);
			sort_grid(&sd);

			// for(int i = 0; i < TEX_N*TEX_N; i++){
			// 	if(sd.cells[i].x == 63){
			// 		cout << i << ": " << "("<< sd.cells[i].x <<", " << sd.cells[i].y << ")" << endl;
			// 	}
			// }
		}

		sd.values[0] = mousePos.x;
		sd.values[1] = mousePos.y;

		sd.values[3] = 0.00;
		if(io.MouseDown[0]){
			sd.values[3] = 30;
		}

		int t = STEPS;
		while(!pause && t--){
			step(&sd, &cs_particles, &cs_density, nullptr);
			if(!t) little_step = 0;
			// step(&sd, &cs_particles, &cs_density, nullptr, &cs_density_global);
		}
		if(little_step){
			while(t--){
				step(&sd, &cs_particles, &cs_density, nullptr);
			}
			little_step = 0;
		}

		// draw calls
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
		
		drawBackground();
		// drawTextureQuad(densityGlobalTexture);
		// drawTextureQuad(densityTex);
		drawParticles();
		drawCircle(mousePos,sd.values[2]*2);
		
		renderImguiFrame(window);
		glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		glfwSwapBuffers(window);
		glfwPollEvents();

		if(ImGui::IsKeyPressed(ImGuiKey_Space)){
			pause = !pause;
		}
		if(ImGui::IsKeyPressed(ImGuiKey_RightArrow)){
			little_step = 1;
		}
	}

	terminateImgui();
	glfwTerminate();

	return 0;
}

/*
baseline: 		200
single-call: 	260

*/

void init(shaderData* sd, ComputeShader* init){
	sd->values[5] = SCREEN_WIDTH;
	sd->values[6] = SCREEN_HEIGHT;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, DBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, DBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(shaderData), sd);

	init->runProgram(TEX_N / 8, TEX_N / 4, 1);
}

void step(shaderData* sd, ComputeShader* part, ComputeShader* dens, ComputeShader* grad, ComputeShader* debug){

	static double curTime = glfwGetTime();
	double deltaTime = glfwGetTime() - curTime;
	curTime = glfwGetTime();

	if(deltaTime > 1.0/60.0 && sync_sim) return;

	sd->values[4] = (float)deltaTime;
	sd->values[5] = SCREEN_WIDTH;
	sd->values[6] = SCREEN_HEIGHT;
	// cout << sd->values[5] << " " << sd->values[6] << endl;


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, DBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, DBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(shaderData), sd);

	// if(debug) debug->runProgram(SCREEN_WIDTH / (8*4), SCREEN_HEIGHT / (4*4), 1);
	dens->runProgram(TEX_N / 8, TEX_N / 4, 1);
	// grad->runProgram(TEX_N / 8, TEX_N / 4, 1);
	part->runProgram(TEX_N / 8, TEX_N / 4, 1);

	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(shaderData), sd);

	sort_grid(sd);
}