#include <string>
#include <GL/glew.h>

class HeightField
{
public:
	int m_meshResolution; // triangles edges per quad side
	GLuint m_texid_hf;
	GLuint m_texid_diffuse;
	GLuint m_vao;
	GLuint m_positionBuffer;
	GLuint m_uvBuffer;
	GLuint m_indexBuffer;
	GLuint m_numIndices;
	std::string m_heightFieldPath;
	std::string m_diffuseTexturePath;

	HeightField(void);

	/// Load height field
	void loadHeightField(const std::string& heigtFieldPath);

	/// Load diffuse map
	void loadDiffuseTexture(const std::string& diffusePath);

	/// Generate mesh
	void generateMesh(int tesselation);

	/// Render height map
	void submitTriangles(void);
};
