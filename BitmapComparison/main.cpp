#define _SCL_SECURE_NO_WARNINGS
// Allows the use of fopen without warning
#define _CRT_SECURE_NO_WARNINGS		

#include <iostream>
#include <cstdio>
#include "bitmap-master/bitmap_image.hpp"
#include "glut\glut.h"
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <queue>
#include <math.h>
#include <iomanip>

// Picture Data
uint8_t* OriginalData = 0;		
uint8_t* AlteredData = 0;		
// OpenGL texture ids for rendering.
GLuint  OriginalTexture = 0;
GLuint  AlteredTexture = 0;
// Picture width and height
int h = 0; 
int w = 0;



struct PixelPosition
{
	// The x and y coordinates of the different pixel
	int x;			
	int y;			
};

// The difference in pixels are stored in the PixelPositin vector 
std::vector<PixelPosition> pos;

void BitMapComparison(bitmap_image* image1, bitmap_image* image2)
{
	PixelPosition position;

	unsigned int total_number_of_pixels = 0;
	unsigned int total_different_pixels = 0;

	// Since both pictures are the same size, we'll loop thourgh the size of just one of the picture
	const unsigned int height = image1->height();
	const unsigned int width = image1->width();

	for (unsigned int y = 0; y < height; ++y)	// Loops through the y-coordinate
	{
		for (unsigned int x = 0; x < width; ++x)	// Loops through the x-coordinate
		{
			rgb_t OriginalColor;
			rgb_t AlteredColor;

			image1->get_pixel(x, y, OriginalColor);
			image2->get_pixel(x, y, AlteredColor);

			if (OriginalColor == AlteredColor)
				total_number_of_pixels++;
			else
			{
				total_different_pixels++;
				// Add to the vector of different pixels. 
				position.x = x;
				position.y = y;
				pos.push_back(position);
			}
		}
	}

	printf("Number of same pixels: %d\n", total_number_of_pixels);
	printf("Number of different pixels: %d\n", total_different_pixels);
}

void DrawOnPicture(bitmap_image inputImage)
{
	// Initializing a blank canvas
	bitmap_image image(inputImage.width(), inputImage.height());
	// Setting the canvas background color to black (RGB = 0 0 0) 
	image.set_all_channels(0, 0, 0);
	// Loops through all the different pixels
	for (unsigned int i = 0; i < pos.size(); i++)
	{
		// Debug, shows the pixel coordinates
		std::cout << pos[i].x << " " << pos[i].y << std::endl;
		// Initialize the drawing code
		image_drawer draw(image);
		// Drawing with a white pixel on the blank canvas
		draw.pen_width(1);
		draw.pen_color(255, 255, 255);
		draw.plot_pen_pixel(pos[i].x, pos[i].y);
	}
	// Saves the output in the "BitmapComparison" folder.
	image.save_image("output.bmp");
}

void draw()
{
	glBindTexture(GL_TEXTURE_2D, AlteredTexture);
	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, w, h, GL_BGR_EXT, GL_UNSIGNED_BYTE, AlteredData);
	glBindTexture(GL_TEXTURE_2D, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	// Draw Original texture to left half of the screen
	glPushMatrix();
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, OriginalTexture);
	glBegin(GL_QUADS);
	glTexCoord2d(0, 1); glVertex2d(1, 1);
	glTexCoord2d(0, 0); glVertex2d(1, 1 + 256);
	glTexCoord2d(1, 0); glVertex2d(1 + 256, 1 + 256);
	glTexCoord2d(1, 1); glVertex2d(1 + 256, 1);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();

	// Draw output texture to right half of the screen
	glPushMatrix();
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, AlteredTexture);
	glBegin(GL_QUADS);
	glTexCoord2d(0, 1); glVertex2d(2 + 256, 1);
	glTexCoord2d(0, 0); glVertex2d(2 + 256, 1 + 256);
	glTexCoord2d(1, 0); glVertex2d(2 + 512, 1 + 256);
	glTexCoord2d(1, 1); glVertex2d(2 + 512, 1);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();

	glutSwapBuffers();
	glutPostRedisplay();
}

bool init()
{
	glMatrixMode(GL_PROJECTION);
	glOrtho(0, 512 + 4, 256 + 2, 0, -1, 1);
	// Initializing the altered texture
	glGenTextures(1, &AlteredTexture);
	glBindTexture(GL_TEXTURE_2D, AlteredTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

// "Messy" .bmp texture loading
// This was taken from another project using a two screen bmp file loading GLUT programming execise
// Sorry if this is hard to read :(
GLuint loadBMPTexture(std::string fileName, bitmap_image* image, uint8_t** data)
{
	assert(image->width() != 0);
	assert(image->height() != 0);
	assert(data != 0);

	// Opening the .bmp file
	FILE *file;
	if ((file = fopen(fileName.c_str(), "rb")) == NULL)
		return 0;
	fseek(file, 18, SEEK_SET);
	
	int width = 0;
	fread(&width, 2, 1, file);
	fseek(file, 2, SEEK_CUR);
	int height = 0;
	fread(&height, 2, 1, file);

	printf("Image \"%s\" (%dx%d)\n", fileName.c_str(), width, height);

	*data = new uint8_t[3 * width * height];
	assert(data != 0);
	fseek(file, 30, SEEK_CUR);
	fread(*data, 3, width * height, file);
	fclose(file);

	GLuint  texId;
	// Initializing the original texture
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height, GL_BGR_EXT, GL_UNSIGNED_BYTE, *data);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (image->width())
		width = image->width();
	if (image->height())
		height = image->height();

	return texId;
}

int StartCompare(int argc, char ** argv)
{
	//The .bmp files used in the code
	std::string fileName1 = "bmpOriginal.bmp";
	std::string fileName2 = "bmpAltered.bmp";

	bitmap_image image1(fileName1);
	bitmap_image image2(fileName2);

	w = image1.width();
	h = image1.height();

	if (!image1 || !image2)
	{
		printf("Error - Failed to open bmp");
		return 1;
	}

	// Start comparing the files
	BitMapComparison(&image1, &image2);
	// Draw the highlighted difference
	DrawOnPicture(image2);

	std::string fileName3 = "output.bmp";
	bitmap_image image3(fileName3);

	// Window creation using GLUT
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(2 * (512 + 4), 2 * (256 + 2));
	glutCreateWindow("BitMap Comparison Demo");
	glutDisplayFunc(draw);
	if (!init()) return -1;

	// Load the files onto the window
	OriginalTexture = loadBMPTexture(fileName1, &image1, &OriginalData);
	AlteredTexture = loadBMPTexture(fileName2, &image2, &AlteredData);

	//OriginalTexture = loadBMPTexture(fileName3, &image3, &OriginalData);

	glutMainLoop();

	

	delete OriginalData;
	delete AlteredData;

	return 0;
}


int main(int argc, char ** argv)
{
	StartCompare(argc, argv);
}

