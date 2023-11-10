#include "hdr.h"
#include <iostream>
#include <stb_image.h>
#include <stb_image_write.h>

namespace labhelper
{
struct HDRImage
{
	int width, height, components;
	float* data = nullptr;
	// Constructor
	HDRImage(const std::string& filename)
	{
		stbi_set_flip_vertically_on_load(true);
		data = stbi_loadf(filename.c_str(), &width, &height, &components, 3);
		if(data == nullptr)
		{
			std::cout << "Failed to load image: " << filename << ".\n";
			exit(1);
		}
	};
	// Destructor
	~HDRImage()
	{
		stbi_image_free(data);
	};
};

GLuint loadHdrTexture(const std::string& filename)
{
	GLuint texId;
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	HDRImage image(filename);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, image.width, image.height, 0, GL_RGB, GL_FLOAT, image.data);

	return texId;
}

GLuint loadHdrMipmapTexture(const std::vector<std::string>& filenames)
{
	GLuint texId;
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	HDRImage image(filenames[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, image.width, image.height, 0, GL_RGB, GL_FLOAT, image.data);
	glGenerateMipmap(GL_TEXTURE_2D);

	// We call this again because AMD drivers have some weird issue in the GenerateMipmap function that
	// breaks the first level of the image.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, image.width, image.height, 0, GL_RGB, GL_FLOAT, image.data);

	const int roughnesses = 8;
	for(int i = 1; i < roughnesses; i++)
	{
		HDRImage image(filenames[i]);
		glTexImage2D(GL_TEXTURE_2D, i, GL_RGB32F, image.width, image.height, 0, GL_RGB, GL_FLOAT, image.data);
	}

	return texId;
}

void saveHdrTexture(const std::string& filename, GLuint texture)
{
	std::vector<float> img;
	std::vector<uint8_t> img_png;

	glBindTexture(GL_TEXTURE_2D, texture);

	GLint lwidth, lheight;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &lwidth);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &lheight);

	const int n_channels = 3;
	img.resize(lwidth * lheight * n_channels);
	img_png.resize(img.size());

	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, img.data());

	for(int r = 0; r < lheight / 2; ++r)
	{
		int a = r * lwidth * n_channels;
		int b = (lheight - 1 - r) * lwidth * n_channels;
		for(int c = 0; c < lwidth * n_channels; ++c)
		{
			std::swap(img[a + c], img[b + c]);

			img_png[a + c] = uint8_t(255 * (img[a + c] / (1 + img[a + c])));
			img_png[b + c] = uint8_t(255 * (img[b + c] / (1 + img[b + c])));
		}
	}


	stbi_write_hdr((filename + ".hdr").c_str(), lwidth, lheight, 3, img.data());
	stbi_write_png((filename + ".png").c_str(), lwidth, lheight, 3, img_png.data(), 0);

}

} // namespace labhelper
