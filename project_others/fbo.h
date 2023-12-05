#include <GL/glew.h>
#include <vector>

class FboInfo {
public:
	GLuint framebufferId;
	std::vector<GLuint> colorTextureTargets; 
	GLuint depthBuffer;
	int width;
	int height;
	bool isComplete;
	GLenum colorTargetType = GL_RGBA16F;

	FboInfo(int numberOfColorBuffers = 1);
		
	void resize(int w, int h);
	bool checkFramebufferComplete(void);
};
