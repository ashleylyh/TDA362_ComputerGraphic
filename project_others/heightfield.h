#include <string>
#include <GL/glew.h>

class HeightField
{
public:
	// Triangles edges per quad side
	int m_meshResolution;
	// Textures.
	GLuint m_texid_hf;
	GLuint m_texid_diffuse;
	GLuint m_texid_shininess;
	// Our VAO and its buffers.
	GLuint m_vao;
	GLuint m_positionBuffer;
	GLuint m_uvBuffer;
	GLuint m_indexBuffer;
	GLuint m_numIndices;

	HeightField(void);

	void loadPlainTexture(GLuint* texid, const std::string& path);
	void loadHeightField(const std::string& path);
	void loadShininess(const std::string& path);
	void loadDiffuseTexture(const std::string& diffusePath);

	void generateMesh(int tesselation);
	void submitTriangles(void);
};
