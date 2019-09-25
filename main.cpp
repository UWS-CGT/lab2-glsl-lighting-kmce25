#pragma warning(disable: 4996)



// MD2 animation renderer
// This demo will load and render an animated MD2 model, an OBJ model and a skybox
// Most of the OpenGL code for dealing with buffer objects, etc has been moved to a 
// utility library, to make creation and display of mesh objects as simple as possible

// Windows specific: Uncomment the following line to open a console window for debug output
#if _DEBUG
#pragma comment(linker, "/subsystem:\"console\" /entry:\"WinMainCRTStartup\"")
#endif

#include "rt3d.h"
#include "rt3dObjLoader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stack>
#include "md2model.h"

//#include <SDL_ttf.h>

using namespace std;

#define DEG_TO_RADIAN 0.017453293

// Globals
// Real programs don't use globals :-D

float attenuationConst = 1.0f;
float attenuationLinear = 0.02f;
float attenuationQuadratic = 0.01f;

GLuint meshIndexCount = 0;
GLuint md2VertCount = 0;
GLuint meshObjects[5];

GLuint shaderProgram;
GLuint skyboxProgram;

GLfloat r = 0.0f;

glm::vec3 eye(0.0f, 1.0f, 0.0f);
glm::vec3 at(0.0f, 1.0f, -1.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);

stack<glm::mat4> mvStack; 

// TEXTURE STUFF
GLuint textures[2];
GLuint skybox[5];
GLuint labels[5];

rt3d::lightStruct light0 = {
	{0.3f, 0.3f, 0.3f, 1.0f}, // ambient
	{1.0f, 1.0f, 1.0f, 1.0f}, // diffuse
	{1.0f, 1.0f, 1.0f, 1.0f}, // specular
	{-10.0f, 10.0f, 10.0f, 1.0f}  // position
};
glm::vec4 lightPos(0.0f, 2.0f, 0.0f, 1.0f); //light position

rt3d::materialStruct material0 = {
	{0.2f, 0.4f, 0.2f, 1.0f}, // ambient
	{0.5f, 1.0f, 0.5f, 1.0f}, // diffuse
	{0.0f, 0.1f, 0.0f, 1.0f}, // specular
	2.0f  // shininess
};
rt3d::materialStruct material1 = {
	{0.4f, 0.4f, 1.0f, 1.0f}, // ambient
	{0.8f, 0.8f, 1.0f, 1.0f}, // diffuse
	{0.8f, 0.8f, 0.8f, 1.0f}, // specular
	1.0f  // shininess
};

// md2 stuff
md2model tmpModel;
int currentAnim = 0;


//TTF_Font * textFont;

// textToTexture
//GLuint textToTexture(const char * str, GLuint textID/*, TTF_Font *font, SDL_Color colour, GLuint &w,GLuint &h */) {
/*	TTF_Font *font = textFont;
	SDL_Color colour = { 255, 255, 255 };
	SDL_Color bg = { 0, 0, 0 };

	SDL_Surface *stringImage;
	stringImage = TTF_RenderText_Blended(font,str,colour);

	if (stringImage == NULL)
		//exitFatalError("String surface not created.");
		std::cout << "String surface not created." << std::endl;

	GLuint w = stringImage->w;
	GLuint h = stringImage->h;
	GLuint colours = stringImage->format->BytesPerPixel;

	GLuint format, internalFormat;
	if (colours == 4) {   // alpha
		if (stringImage->format->Rmask == 0x000000ff)
			format = GL_RGBA;
	    else
		    format = GL_BGRA;
	} else {             // no alpha
		if (stringImage->format->Rmask == 0x000000ff)
			format = GL_RGB;
	    else
		    format = GL_BGR;
	}
	internalFormat = (colours == 4) ? GL_RGBA : GL_RGB;

	GLuint texture = textID;

	if (texture == 0) {
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	} //Do this only when you initialise the texture to avoid memory leakage

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, stringImage->w, stringImage->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, stringImage->pixels);
	glBindTexture(GL_TEXTURE_2D, NULL);

	SDL_FreeSurface(stringImage);
	return texture;
}

*/
// Set up rendering context
SDL_Window * setupRC(SDL_GLContext &context) {																
	SDL_Window * window;
    if (SDL_Init(SDL_INIT_VIDEO) < 0)																	// Initialize video
        rt3d::exitFatalError("Unable to initialize SDL"); 
	  
    // Request an OpenGL 3.0 context.
	
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); 

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);  // double buffering on
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8); // 8 bit alpha buffering
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4); // Turn on x4 multisampling anti-aliasing (MSAA)
 
    // Create 800x600 window
    window = SDL_CreateWindow("SDL/GLM/OpenGL Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,				
        800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
	if (!window) // Check window was created OK
        rt3d::exitFatalError("Unable to create window");
 
    context = SDL_GL_CreateContext(window);																// 0.0 - Create opengl context and attach to window - Context is a data structure that collects all the info to render an image.
    SDL_GL_SetSwapInterval(1);																			// set swap buffers to sync with monitor's vertical refresh rate
	return window;
}

// A simple texture loading function
// lots of room for improvement - and better error checking!
GLuint loadBitmap(char *fname) {            
	GLuint texID;
	glGenTextures(1, &texID);																			//0.1 - Generate texture names - void glGenTextures(GLsizei n, GLuint * textures); n = amount of texture names to be generated. *textures  = Specifies an array in which the generated texture names are stored

	// load file - using core SDL library
 	SDL_Surface *tmpSurface;																			//Represent areas of "graphical" memory, memory that can be drawn to.
	tmpSurface = SDL_LoadBMP(fname);
	if (!tmpSurface) {
		std::cout << "Error loading bitmap" << std::endl;
	}

	// bind texture and set parameters
	glBindTexture(GL_TEXTURE_2D, texID);																//0.2 - Binds a texture (texID) to a texturing target. void glBindTexture(GLenum target, GLuint texture); target = textures type. If rebinding, has to be the same type
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);									//0.3 - glTexParameteri sets the parameters of the texture. The texture minifying function is used whenever the pixel being textured maps to an area greater than one texture element, 0.4 GL_LINEAR is one of the functions - Returns the weighted average of the four texture elements that are closest to the center of the pixel being textured. 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);									// GL_TEXTURE_MAG_FILTER - function is used when the pixel being textured maps to an area less than or equal to one texture element. S = x, T = y
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);								// 0.5
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	SDL_PixelFormat *format = tmpSurface->format;														//SDL_PixelFormat - a structure that contains pixel format information (describes the format of the pixel data stored at the pixels field of a SDL_Surface)
	
	GLuint externalFormat, internalFormat;																//0.6 - Internal - is the format that OpenGL will use to internally store the texture data you give it.
	if (format->Amask) {																				//Amask - Alpha mask
		internalFormat = GL_RGBA;
		externalFormat = (format->Rmask < format-> Bmask) ? GL_RGBA : GL_BGRA;
	}
	else {
		internalFormat = GL_RGB;
		externalFormat = (format->Rmask < format-> Bmask) ? GL_RGB : GL_BGR;
	}

	glTexImage2D(GL_TEXTURE_2D,0,internalFormat,tmpSurface->w, tmpSurface->h, 0,						//0.7 - Specifies a two-dimensional texture image
		externalFormat, GL_UNSIGNED_BYTE, tmpSurface->pixels);		
	glGenerateMipmap(GL_TEXTURE_2D);																	//generates mipmaps for a specified texture object. Mipmaps = 1/3 extra texture memory.

	SDL_FreeSurface(tmpSurface);																		// texture loaded, free the temporary buffer
	return texID;	// return value of texture ID
}


void init(void) {
	
	shaderProgram = rt3d::initShaders("../phong-tex.vert","../phong-tex.frag");
	rt3d::setLight(shaderProgram, light0);
	rt3d::setMaterial(shaderProgram, material0);

	skyboxProgram = rt3d::initShaders("../textured.vert","../textured.frag");

	vector<GLfloat> verts;
	vector<GLfloat> norms;
	vector<GLfloat> tex_coords;
	vector<GLuint> indices;
	rt3d::loadObj("../resource/cube.obj", verts, norms, tex_coords, indices);
	GLuint size = indices.size();
	meshIndexCount = size;
	textures[0] = loadBitmap("../resource/fabric.bmp");
	meshObjects[0] = rt3d::createMesh(verts.size()/3, verts.data(), nullptr, norms.data(), tex_coords.data(), size, indices.data());

	meshObjects[2] = rt3d::createMesh(verts.size() / 3, verts.data(), nullptr, norms.data(), tex_coords.data(), size, indices.data());


	textures[1] = loadBitmap("../resource/hobgoblin2.bmp");
	meshObjects[1] = tmpModel.ReadMD2Model("../resource/tris.MD2");
	md2VertCount = tmpModel.getVertDataCount();

	
	
	skybox[0] = loadBitmap("../resource/Town-skybox/Town_ft.bmp");
	skybox[1] = loadBitmap("../resource/Town-skybox/Town_bk.bmp");
	skybox[2] = loadBitmap("../resource/Town-skybox/Town_lf.bmp");
	skybox[3] = loadBitmap("../resource/Town-skybox/Town_rt.bmp");
	//skybox[4] = loadBitmap("Town-skybox/Town_up.bmp");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	/*
	// set up TrueType / SDL_ttf
	if (TTF_Init()== -1)
		cout << "TTF failed to initialise." << endl;

	textFont = TTF_OpenFont("../resource/MavenPro-Regular.ttf", 48);
	if (textFont == NULL)
		cout << "Failed to open font." << endl;
		
	labels[0] = 0;//First init to 0 to avoid memory leakage if it is dynamic
	labels[0] = textToTexture(" Hello ", labels[0]);//Actual set up of label. If dynamic, this should go in draw function
	*/
}

glm::vec3 moveForward(glm::vec3 pos, GLfloat angle, GLfloat d) {
	return glm::vec3(pos.x + d*std::sin(r*DEG_TO_RADIAN), pos.y, pos.z - d*std::cos(r*DEG_TO_RADIAN));
}

glm::vec3 moveRight(glm::vec3 pos, GLfloat angle, GLfloat d) {
	return glm::vec3(pos.x + d*std::cos(r*DEG_TO_RADIAN), pos.y, pos.z + d*std::sin(r*DEG_TO_RADIAN));
}

void update(void) {
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_W])
	{		
		eye = moveForward(eye, r, 0.1f);		
	}
	if (keys[SDL_SCANCODE_S])
	{
		eye = moveForward(eye, r, -0.1f);		
	}
	if (keys[SDL_SCANCODE_A])
	{
		eye = moveRight(eye, r, -0.1f);		
	}
	if (keys[SDL_SCANCODE_D])
	{
		eye = moveRight(eye, r, 0.1f);		
	}

	if (keys[SDL_SCANCODE_I])
	{
		lightPos = glm::vec4(moveForward(lightPos, 0.0f, 0.1f), 1.0f);
	}

	if (keys[SDL_SCANCODE_J])
	{
		lightPos = glm::vec4(moveRight(lightPos, 0.0f, -0.1f), 1.0f);
	}

	if (keys[SDL_SCANCODE_K])
	{
		lightPos = glm::vec4(moveForward(lightPos, 0.0f, -0.1f), 1.0f);
	}

	if (keys[SDL_SCANCODE_L])
	{
		lightPos = glm::vec4(moveRight(lightPos, 0.0f, 0.1f), 1.0);
	}

	if (keys[SDL_SCANCODE_U])
	{
		lightPos.y += 0.1;
	}

	if (keys[SDL_SCANCODE_H])
	{
		lightPos.y -= 0.1;
	}

	if ( keys[SDL_SCANCODE_R] ) eye.y += 0.1;
	if ( keys[SDL_SCANCODE_F] ) eye.y -= 0.1;

	if (keys[SDL_SCANCODE_Z]) light0.ambient[0] += 1.0f;


	//cout << lightPos.x << "\n" << lightPos.y << "\n" << lightPos.z << "\n";

	if ( keys[SDL_SCANCODE_COMMA] ) r -= 1.0f;
	if ( keys[SDL_SCANCODE_PERIOD] ) r += 1.0f;

	if ( keys[SDL_SCANCODE_1] ) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_CULL_FACE);
	}
	if ( keys[SDL_SCANCODE_2] ) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_CULL_FACE);
	}
	if ( keys[SDL_SCANCODE_Z] ) {
		if (--currentAnim < 0) currentAnim = 19;
		cout << "Current animation: " << currentAnim << endl;
	}
	if ( keys[SDL_SCANCODE_X] ) {
		if (++currentAnim >= 20) currentAnim = 0;
		cout << "Current animation: " << currentAnim << endl;
	}


	if (keys[SDL_SCANCODE_B])
	{
		attenuationQuadratic -= 0.01;
		cout << attenuationQuadratic;
	}

	if (keys[SDL_SCANCODE_N])
	{
		attenuationQuadratic += 0.01;
		cout << attenuationQuadratic;
	}

	
}

void draw(SDL_Window * window) {
	// clear the screen
	glEnable(GL_CULL_FACE);
	glClearColor(0.5f,0.5f,0.5f,1.0f);
	glClear(GL_COLOR_BUFFER_BIT  | GL_DEPTH_BUFFER_BIT);


	glm::mat4 projection(1.0);
	projection = glm::perspective(float(60.0f*DEG_TO_RADIAN),800.0f/600.0f,1.0f,150.0f);
	rt3d::setUniformMatrix4fv(shaderProgram, "projection", glm::value_ptr(projection));
	

	GLfloat scale(1.0f); // just to allow easy scaling of complete scene
	
	glm::mat4 modelview(1.0); // set base position for scene
	mvStack.push(modelview);

	at = moveForward(eye,r,1.0f);
	mvStack.top() = glm::lookAt(eye,at,up);


	// draw a skybox
	glUseProgram(skyboxProgram);
	rt3d::setUniformMatrix4fv(skyboxProgram, "projection", glm::value_ptr(projection));

	glDepthMask(GL_FALSE); // make sure depth test is off
	glm::mat3 mvRotOnlyMat3 = glm::mat3(mvStack.top());
	mvStack.push( glm::mat4(mvRotOnlyMat3) );

	// front
	mvStack.push( mvStack.top() );
	glBindTexture(GL_TEXTURE_2D, skybox[0]);
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(2.0f, 2.0f, 2.0f));
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(0.0f, 0.0f, -2.0f));
	rt3d::setUniformMatrix4fv(skyboxProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawIndexedMesh(meshObjects[0],meshIndexCount,GL_TRIANGLES);
	mvStack.pop();

	// back
	mvStack.push( mvStack.top() );
	glBindTexture(GL_TEXTURE_2D, skybox[1]);
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(2.0f, 2.0f, 2.0f));
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(0.0f, 0.0f, 2.0f));
	rt3d::setUniformMatrix4fv(skyboxProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawIndexedMesh(meshObjects[0],meshIndexCount,GL_TRIANGLES);
	mvStack.pop();

	// left
	mvStack.push( mvStack.top() );
	glBindTexture(GL_TEXTURE_2D, skybox[2]);
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(2.0f, 2.0f, 2.0f));
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(-2.0f, 0.0f, 0.0f));
	rt3d::setUniformMatrix4fv(skyboxProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawIndexedMesh(meshObjects[0],meshIndexCount,GL_TRIANGLES);
	mvStack.pop();

	// right
	mvStack.push( mvStack.top() );
	glBindTexture(GL_TEXTURE_2D, skybox[3]);
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(2.0f, 2.0f, 2.0f));
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(2.0f, 0.0f, 0.0f));
	rt3d::setUniformMatrix4fv(skyboxProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawIndexedMesh(meshObjects[0],meshIndexCount,GL_TRIANGLES);
	mvStack.pop();


	mvStack.pop();

	// back to remainder of rendering
	glDepthMask(GL_TRUE); // make sure depth test is on
		
	glUseProgram(shaderProgram);

	glm::vec4 tmp = mvStack.top()*lightPos;
	light0.position[0] = tmp.x;
	light0.position[1] = tmp.y;
	light0.position[2] = tmp.z;
	rt3d::setLightPos(shaderProgram, glm::value_ptr(tmp));


	rt3d::setUniformMatrix4fv(shaderProgram, "projection", glm::value_ptr(projection));

	int attenConst = glGetUniformLocation(shaderProgram, "attenuationConst");
	glUniform1f(attenConst, attenuationConst);

	int attenLinear = glGetUniformLocation(shaderProgram, "attenuationLinear");
	glUniform1f(attenLinear, attenuationLinear);

	int attenQuadratic = glGetUniformLocation(shaderProgram, "attenuationQuadratic");
	glUniform1f(attenQuadratic, attenuationQuadratic);


	// draw a cube for ground plane
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(-10.0f, -0.1f, -10.0f));
	mvStack.top() = glm::scale(mvStack.top(),glm::vec3(20.0f, 0.1f, 20.0f));
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0],meshIndexCount,GL_TRIANGLES);
	mvStack.pop();

	// Animate the md2 model, and update the mesh with new vertex data
	tmpModel.Animate(currentAnim,0.1);
	rt3d::updateMesh(meshObjects[1],RT3D_VERTEX,tmpModel.getAnimVerts(),tmpModel.getVertDataSize());

	// draw the hobgoblin
	glCullFace(GL_FRONT); // md2 faces are defined clockwise, so cull front face
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	rt3d::materialStruct tmpMaterial = material1;
	rt3d::setMaterial(shaderProgram, tmpMaterial);
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(1.0f, 1.2f, -5.0f));
	mvStack.top() = glm::rotate(mvStack.top(), float(90.0f*DEG_TO_RADIAN), glm::vec3(-1.0f, 0.0f, 0.0f));
	mvStack.top() = glm::scale(mvStack.top(),glm::vec3(scale*0.05, scale*0.05, scale*0.05));
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawMesh(meshObjects[1], md2VertCount, GL_TRIANGLES);
	mvStack.pop();
	glCullFace(GL_BACK);
	   	 	
	//glUseProgram(skyboxProgram);//Use the texture shader again to draw the labelled cube
	//rt3d::setUniformMatrix4fv(skyboxProgram, "projection", glm::value_ptr(projection));
	//// draw a cube block on top of ground plane
	//// with text texture 
	////Keep the texture-only shader
	//glDepthMask(GL_FALSE); // make sure writing to update depth test is off
	////glBindTexture(GL_TEXTURE_2D, labels[0]);
	//mvStack.push(mvStack.top());
	//mvStack.top() = glm::translate(mvStack.top(), glm::vec3(-1.0f, 2.0f, -7.0f));
	//mvStack.top() = glm::scale(mvStack.top(), glm::vec3(1.0f, 1.0f, 0.0f));
	//rt3d::setUniformMatrix4fv(skyboxProgram, "modelview", glm::value_ptr(mvStack.top()));
	//rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	//mvStack.pop();
	  

	//Cube for light to follow
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(lightPos.x, lightPos.y, lightPos.z));
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(0.5f, 0.5f, 0.5f));
	rt3d::setUniformMatrix4fv(shaderProgram, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(shaderProgram, material0);
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();
	
	
	
	// remember to use at least one pop operation per push...
	mvStack.pop(); // initial matrix
	glDepthMask(GL_TRUE);

    SDL_GL_SwapWindow(window); // swap buffers
}


// Program entry point - SDL manages the actual WinMain entry point for us
int main(int argc, char *argv[]) {
    SDL_Window * hWindow; // window handle
    SDL_GLContext glContext; // OpenGL context handle
    hWindow = setupRC(glContext); // Create window and render context 

	// Required on Windows *only* init GLEW to access OpenGL beyond 1.1
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err) { // glewInit failed, something is seriously wrong
		std::cout << "glewInit failed, aborting." << endl;
		exit (1);
	}
	cout << glGetString(GL_VERSION) << endl;

	init();

	bool running = true; // set running to true
	SDL_Event sdlEvent;  // variable to detect SDL events
	while (running)	{	// the event loop
		while (SDL_PollEvent(&sdlEvent)) {
			if (sdlEvent.type == SDL_QUIT)
				running = false;
		}
		update();
		draw(hWindow); // call the draw function
	}

    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(hWindow);
    SDL_Quit();
    return 0;
}