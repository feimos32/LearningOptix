#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glew.h>
#  if defined( _WIN32 )
#    include <GL/wglew.h>
#    include <GL/freeglut.h>
#  else
#    include <GL/glut.h>
#  endif
#endif

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

uint32_t width = 512;
uint32_t height = 512;

// 全局变量
const char* const Project_NAME = "optixTest";
using namespace optix;
Context        context = 0;
Buffer result_buffer;
Buffer getOutputBuffer();

void displayBuffer(RTbuffer bufferInput);

void glutInitialize(int* argc, char** argv);
void createContext();

void glutDisplay();
void glutResize(int w, int h);
void registerExitHandler();

void glutRun();

int main(int argc, char** argv)
{
	try
	{
		glutInitialize(&argc, argv);

		// 创建环境
		createContext();

		glutRun();
		
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << "OptiX Error: '" << e.what() << "'\n";
		system("pause");
		exit(1);
	}
	return 0;
}

// ********************************************************** 
// 以下代码都是场景初始化函数
// ********************************************************** 

// 生成Context
void createContext()
{
	context = optix::Context::create();
	context->setRayTypeCount(1); //1种光线(有时可以2种，渲染+阴影)
	context->setEntryPointCount(1);

	//申请Buffer
	result_buffer = context->createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT4, width, height);
	context["result_buffer"]->set(result_buffer);
	//编译Shader
	std::string ptx = "./x64/Debug/render_test.cu.ptx";
	//std::string ptx = "./x64/Debug/optixHello_generated_draw_color.cu.ptx"; 
	//从ptx字串中标识 "draw_solid_color"入口函数，并创建并返回光线发生器模块
	optix::Program ray_gen_program = context->createProgramFromPTXFile(ptx, "draw_solid_color");
	context->setRayGenerationProgram(0, ray_gen_program);
	context["draw_color"]->setFloat(0.8f, 0.7f, 0.0f);
}

// ********************************************************** 
// 以下代码都是与渲染相关的函数
// ********************************************************** 

int a = 0;
// true表示加，false表示减
bool sub_add_flag = true;
void glutDisplay()
{
	if(sub_add_flag == true) a++;
	else a--;
	if (a >= 255) {
		sub_add_flag = false;
	}
	else if(a <= 0){
		sub_add_flag = true;
	}
	float color1 = (float)a / 255.0f;

	context["draw_color"]->setFloat(color1, 1.0f - color1, 0.0f);

	//开启第0个入口
	context->launch(0, width, height);

	displayBuffer(getOutputBuffer()->get());

	glutSwapBuffers();
}
void glutRun()
{
	// Initialize GL state                                                            
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glViewport(0, 0, width, height);

	glutShowWindow();
	glutReshapeWindow(width, height);

	// register glut callbacks
	glutDisplayFunc(glutDisplay);
	glutIdleFunc(glutDisplay);
	glutReshapeFunc(glutResize);

	registerExitHandler();

	glutMainLoop();
}

// ********************************************************** 
// 以下代码都是OpenGL 窗口生成和交互函数
// ********************************************************** 

void glutInitialize(int* argc, char** argv)
{
	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_ALPHA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(width, height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow(Project_NAME);
	glutHideWindow();
}
void glutResize(int w, int h)
{
	if (w == (int)width && h == (int)height) return;
	width = w;
	height = h;

	glViewport(0, 0, width, height);

	glutPostRedisplay();
}
void destroyContext()
{
	// 暂无功能
}
void registerExitHandler()
{
	// this function is freeglut-only
	glutCloseFunc(destroyContext);
}

// ********************************************************** 
// 以下代码都是用来生成和使用OpenGL Buffer
// ********************************************************** 

Buffer getOutputBuffer()
{
	return context["result_buffer"]->getBuffer();
}
enum bufferPixelFormat
{
	BUFFER_PIXEL_FORMAT_DEFAULT, // The default depending on the buffer type
	BUFFER_PIXEL_FORMAT_RGB,     // The buffer is RGB or RGBA
	BUFFER_PIXEL_FORMAT_BGR,     // The buffer is BGR or BGRA
};
// Converts the buffer format to gl format
GLenum glFormatFromBufferFormat(bufferPixelFormat pixel_format, RTformat buffer_format)
{
	if (buffer_format == RT_FORMAT_UNSIGNED_BYTE4)
	{
		switch (pixel_format)
		{
		case BUFFER_PIXEL_FORMAT_DEFAULT:
			return GL_BGRA;
		case BUFFER_PIXEL_FORMAT_RGB:
			return GL_RGBA;
		case BUFFER_PIXEL_FORMAT_BGR:
			return GL_BGRA;
		default:
			throw Exception("Unknown buffer pixel format");
		}
	}
	else if (buffer_format == RT_FORMAT_FLOAT4)
	{
		switch (pixel_format)
		{
		case BUFFER_PIXEL_FORMAT_DEFAULT:
			return GL_RGBA;
		case BUFFER_PIXEL_FORMAT_RGB:
			return GL_RGBA;
		case BUFFER_PIXEL_FORMAT_BGR:
			return GL_BGRA;
		default:
			throw Exception("Unknown buffer pixel format");
		}
	}
	else if (buffer_format == RT_FORMAT_FLOAT3)
		switch (pixel_format)
		{
		case BUFFER_PIXEL_FORMAT_DEFAULT:
			return GL_RGB;
		case BUFFER_PIXEL_FORMAT_RGB:
			return GL_RGB;
		case BUFFER_PIXEL_FORMAT_BGR:
			return GL_BGR;
		default:
			throw Exception("Unknown buffer pixel format");
		}
	else if (buffer_format == RT_FORMAT_FLOAT)
		return GL_LUMINANCE;
	else
		throw Exception("Unknown buffer format");
}
// The pixel format of the buffer or 0 to use the default for the pixel type
void displayBuffer(RTbuffer bufferInput)
{
	// The pixel format of the buffer or 0 to use the default for the pixel type
	bufferPixelFormat format = BUFFER_PIXEL_FORMAT_DEFAULT; 
	bool disable_srgb_conversion = false;
	optix::Buffer buffer = Buffer::take(bufferInput);

	// Query buffer information
	RTsize buffer_width_rts, buffer_height_rts;
	buffer->getSize(buffer_width_rts, buffer_height_rts);
	uint32_t width = static_cast<int>(buffer_width_rts);
	uint32_t height = static_cast<int>(buffer_height_rts);
	RTformat buffer_format = buffer->getFormat();

	GLboolean use_SRGB = GL_FALSE;
	if (!disable_srgb_conversion && (buffer_format == RT_FORMAT_FLOAT4 || buffer_format == RT_FORMAT_FLOAT3))
	{
		glGetBooleanv(GL_FRAMEBUFFER_SRGB_CAPABLE_EXT, &use_SRGB);
		if (use_SRGB)
			glEnable(GL_FRAMEBUFFER_SRGB_EXT);
	}

	static unsigned int gl_tex_id = 0;
	if (!gl_tex_id)
	{
		glGenTextures(1, &gl_tex_id);
		glBindTexture(GL_TEXTURE_2D, gl_tex_id);

		// Change these to GL_LINEAR for super- or sub-sampling
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		// GL_CLAMP_TO_EDGE for linear filtering, not relevant for nearest.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	glBindTexture(GL_TEXTURE_2D, gl_tex_id);

	// send PBO or host-mapped image data to texture
	const unsigned pboId = buffer->getGLBOId();
	GLvoid* imageData = 0;
	if (pboId)
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboId);
	else
		imageData = buffer->map(0, RT_BUFFER_MAP_READ);

	RTsize elmt_size = buffer->getElementSize();
	if (elmt_size % 8 == 0) glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
	else if (elmt_size % 4 == 0) glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	else if (elmt_size % 2 == 0) glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
	else                          glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	GLenum pixel_format = glFormatFromBufferFormat(format, buffer_format);

	if (buffer_format == RT_FORMAT_UNSIGNED_BYTE4)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, pixel_format, GL_UNSIGNED_BYTE, imageData);
	else if (buffer_format == RT_FORMAT_FLOAT4)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, width, height, 0, pixel_format, GL_FLOAT, imageData);
	else if (buffer_format == RT_FORMAT_FLOAT3)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F_ARB, width, height, 0, pixel_format, GL_FLOAT, imageData);
	else if (buffer_format == RT_FORMAT_FLOAT)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE32F_ARB, width, height, 0, pixel_format, GL_FLOAT, imageData);
	else
		throw Exception("Unknown buffer format");

	if (pboId)
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	else
		buffer->unmap();

	// 1:1 texel to pixel mapping with glOrtho(0, 1, 0, 1, -1, 1) setup:
	// The quad coordinates go from lower left corner of the lower left pixel
	// to the upper right corner of the upper right pixel.
	// Same for the texel coordinates.

	glEnable(GL_TEXTURE_2D);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(0.0f, 0.0f);

	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(1.0f, 0.0f);

	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(1.0f, 1.0f);

	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(0.0f, 1.0f);
	glEnd();

	glDisable(GL_TEXTURE_2D);

	if (use_SRGB)
		glDisable(GL_FRAMEBUFFER_SRGB_EXT);
}





















