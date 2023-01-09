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

void glutInitialize(int* argc, char** argv)
{
	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_ALPHA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(width, height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Hello World");
	glutHideWindow();
}

void glutDisplay();
void glutResize(int w, int h);
void registerExitHandler();

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

int main(int argc, char** argv)
{
	try
	{
		glutInitialize(&argc, argv);

		glutRun();
		
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << "OptiX Error: '" << e.what() << "'\n";
		exit(1);
	}
	return 0;
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


	glClearColor(color1,1.0f - color1,1.0f,1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glutSwapBuffers();
}

