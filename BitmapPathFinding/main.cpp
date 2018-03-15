#define _SCL_SECURE_NO_WARNINGS
// Allows the use of fopen without warning
#define _CRT_SECURE_NO_WARNINGS		

#include <iostream>
#include <cstdio>
#include "bitmap-master\bitmap_image.hpp" 
#include "glut\glut.h"
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <queue>
#include <math.h>
#include <iomanip>

uint8_t* OriginalData = 0;
uint8_t* AlteredData = 0;
GLuint  OriginalTexture = 0;
GLuint  AlteredTexture = 0;
int h = 0;
int w = 0;


//A-star variables
int startX = 0, startY = 0, endX = 0, endY = 0;
const int n = 128; // horizontal size of the map
const int m = 128; // vertical size size of the map
static int map[n][m];
static int pictureMap[n][m];
static int closed_nodes_map[n][m]; // map of closed (tried-out) nodes
static int open_nodes_map[n][m]; // map of open (not-yet-tried) nodes
static int dir_map[n][m]; // map of directions
const int dir = 8; // number of possible directions to go at any position
				   // if dir==4
				   //static int dx[dir]={1, 0, -1, 0};
				   //static int dy[dir]={0, 1, 0, -1};
				   // if dir==8
static int dx[dir] = { 1, 1, 0, -1, -1, -1, 0, 1 };
static int dy[dir] = { 0, 1, 1, 1, 0, -1, -1, -1 };

class node
{
	// current position
	int xPos;
	int yPos;
	// total distance already travelled to reach the node
	int level;
	// priority=level+remaining distance estimate
	int priority;  // smaller: higher priority

public:
	node(int xp, int yp, int d, int p)
	{
		xPos = xp; yPos = yp; level = d; priority = p;
	}

	int getxPos() const { return xPos; }
	int getyPos() const { return yPos; }
	int getLevel() const { return level; }
	int getPriority() const { return priority; }

	void updatePriority(const int & xDest, const int & yDest)
	{
		priority = level + estimate(xDest, yDest) * 10; //A*
	}

	// give better priority to going strait instead of diagonally
	void nextLevel(const int & i) // i: direction
	{
		level += (dir == 8 ? (i % 2 == 0 ? 10 : 14) : 10);
	}

	// Estimation function for the remaining distance to the goal.
	const int & estimate(const int & xDest, const int & yDest) const
	{
		static int xd, yd, d;
		xd = xDest - xPos;
		yd = yDest - yPos;

		// Euclidian Distance
		d = static_cast<int>(sqrt(xd*xd + yd*yd));

		// Manhattan distance
		//d=abs(xd)+abs(yd);

		// Chebyshev distance
		//d=max(abs(xd), abs(yd));

		return(d);
	}
};

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

bool operator<(const node & a, const node & b)
{
	return a.getPriority() > b.getPriority();
}

void FindStartAndFinish(bitmap_image* image)
{
	for (unsigned int y = 0; y < image->height(); ++y)	// Loops through the y-coordinate
	{
		//std::cout << "\n";
		for (unsigned int x = 0; x < image->width(); ++x)	// Loops through the x-coordinate
		{
			rgb_t CurrentPixel;
			image->get_pixel(x, y, CurrentPixel);

			if (CurrentPixel.red == 255 && CurrentPixel.green == 255 && CurrentPixel.blue == 255) //white empty space
			{
				map[x][y] = 0;
				//std::cout << ".";
			}
			if (CurrentPixel.red == 0 && CurrentPixel.green == 255 && CurrentPixel.blue == 0) //green obstacle
			{
				map[x][y] = 1;
				//std::cout << "G";
			}
			if (CurrentPixel.red == 0 && CurrentPixel.green == 0 && CurrentPixel.blue == 255) //blue Start pixel
			{
				map[x][y] = 2;
				//std::cout << "B";
				startX = x;
				startY = y;
			}
			if (CurrentPixel.red == 255 && CurrentPixel.green == 0 && CurrentPixel.blue == 0) //red Finish pixel
			{
				map[x][y] = 4;
				//std::cout << "R";
				endX = x;
				endY = y;
			}
		}
	}
}

std::string pathFind(const int & xStart, const int & yStart,
	const int & xFinish, const int & yFinish)
{
	static std::priority_queue<node> pq[2]; // list of open (not-yet-tried) nodes
	static int pqi; // pq index
	static node* n0;
	static node* m0;
	static int i, j, x, y, xdx, ydy;
	static char c;
	pqi = 0;

	std::cout << "Calculating optimal route... this may take a while" << std::endl;

	// reset the node maps
	for (y = 0; y<m; y++)
	{
		for (x = 0; x<n; x++)
		{
			closed_nodes_map[x][y] = 0;
			open_nodes_map[x][y] = 0;
		}
	}

	// create the start node and push into list of open nodes
	n0 = new node(xStart, yStart, 0, 0);
	n0->updatePriority(xFinish, yFinish);
	pq[pqi].push(*n0);

	//open_nodes_map[x][y] = n0->getPriority(); // mark it on the open nodes map
	open_nodes_map[xStart][yStart] = n0->getPriority();

	//a* search
	delete n0;
	while (!pq[pqi].empty())
	{
		// get the current node w/ the highest priority
		// from the list of open nodes
		n0 = new node(pq[pqi].top().getxPos(), pq[pqi].top().getyPos(),
			pq[pqi].top().getLevel(), pq[pqi].top().getPriority());

		x = n0->getxPos(); y = n0->getyPos();

		pq[pqi].pop(); // remove the node from the open list
		open_nodes_map[x][y] = 0;
		// mark it on the closed nodes map
		closed_nodes_map[x][y] = 1;

		// quit searching when the goal state is reached
		//if((*n0).estimate(xFinish, yFinish) == 0)
		if (x == xFinish && y == yFinish)
		{
			// generate the path from finish to start
			// by following the directions
			std::string path = "";
			while (!(x == xStart && y == yStart))
			{
				j = dir_map[x][y];
				c = '0' + (j + dir / 2) % dir;
				path = c + path;
				x += dx[j];
				y += dy[j];
			}

			// garbage collection
			delete n0;
			// empty the leftover nodes
			while (!pq[pqi].empty()) pq[pqi].pop();
			return path;
		}

		// generate moves (child nodes) in all possible directions
		for (i = 0; i<dir; i++)
		{
			xdx = x + dx[i]; ydy = y + dy[i];

			if (!(xdx<0 || xdx>n - 1 || ydy<0 || ydy>m - 1 || map[xdx][ydy] == 1
				|| closed_nodes_map[xdx][ydy] == 1))
			{
				// generate a child node
				m0 = new node(xdx, ydy, n0->getLevel(),
					n0->getPriority());
				m0->nextLevel(i);
				m0->updatePriority(xFinish, yFinish);

				// if it is not in the open list then add into that
				if (open_nodes_map[xdx][ydy] == 0)
				{
					open_nodes_map[xdx][ydy] = m0->getPriority();
					pq[pqi].push(*m0);
					// mark its parent node direction
					delete m0;
					dir_map[xdx][ydy] = (i + dir / 2) % dir;
				}
				else if (open_nodes_map[xdx][ydy]>m0->getPriority())
				{
					// update the priority info
					open_nodes_map[xdx][ydy] = m0->getPriority();
					// update the parent direction info
					dir_map[xdx][ydy] = (i + dir / 2) % dir;

					// replace the node
					// by emptying one pq to the other one
					// except the node to be replaced will be ignored
					// and the new node will be pushed in instead
					while (!(pq[pqi].top().getxPos() == xdx &&
						pq[pqi].top().getyPos() == ydy))
					{
						pq[1 - pqi].push(pq[pqi].top());
						pq[pqi].pop();
					}
					pq[pqi].pop(); // remove the wanted node

								   // empty the larger size pq to the smaller one
					if (pq[pqi].size()>pq[1 - pqi].size()) pqi = 1 - pqi;
					while (!pq[pqi].empty())
					{
						pq[1 - pqi].push(pq[pqi].top());
						pq[pqi].pop();
					}
					pqi = 1 - pqi;
					pq[pqi].push(*m0); // add the better node instead
				}
				else delete m0; // garbage collection
			}
		}
		delete n0; // garbage collection
	}
	return ""; // no route found
}

void MazeDraw(std::string route, bitmap_image inputImage)
{
	bitmap_image NewImage(inputImage.width(), inputImage.height());
	NewImage.set_all_channels(255, 255, 255);

	if (route.length()>0)
	{
		int j; char c;
		int x = startX;
		int y = startY;
		map[x][y] = 2;
		for (unsigned int i = 0; i<route.length(); i++)
		{
			c = route.at(i);
			j = atoi(&c);
			x = x + dx[j];
			y = y + dy[j];
			map[x][y] = 3;
		}
		map[x][y] = 4;

		// Initialize the drawing code
		image_drawer draw(NewImage);
		// Drawing with a white pixel on the blank canvas
		draw.pen_width(1);

		// display the map with the route
		for (int y = 0; y<m; y++)
		{
			for (int x = 0; x < n; x++)
			{
				if (map[x][y] == 1)
				{
					draw.pen_color(0, 255, 0);
					draw.plot_pen_pixel(x, y);
					//std::cout << "O"; //obstacle
				}
				if (map[x][y] == 2)
				{
					//std::cout << "S"; //start
					draw.pen_color(0, 0, 255);
					draw.plot_pen_pixel(x, y);
				}
				if (map[x][y] == 3)
				{
					//std::cout << "R"; //route
					draw.pen_color(255, 105, 180);
					draw.plot_pen_pixel(x, y);
				}
				if (map[x][y] == 4)
				{
					//std::cout << "F"; //finish
					draw.pen_color(255, 0, 0);
					draw.plot_pen_pixel(x, y);
				}
			}
			//std::cout << "\n" << std::endl;
		}
	}
	NewImage.save_image("SolvedMaze.bmp");
}


int Maze(int argc, char ** argv)
{
	std::string fileName1 = "maze.bmp";
	bitmap_image image1(fileName1);
	
	w = image1.width();
	h = image1.height();

	if (!image1)
	{
		printf("Error - Failed to open bmp");
		return 1;
	}
	FindStartAndFinish(&image1);
	std::cout << "\nStart X: " << startX << "\nStart Y: " << startY << " " << "\nEnd X: " << endX << "\nEnd Y: " << endY << std::endl;
	//std::cout << "test: " << TheMap.size() <<std::endl;

	clock_t start = clock();
	std::string route = pathFind(startX, startY, endX, endY);
	if (route == "") std::cout << "An empty route generated!" << std::endl;
	clock_t end = clock();
	double time_elapsed = double(end - start);
	std::cout << "\nTime to calculate the route (ms): " << time_elapsed << std::endl;

	MazeDraw(route, image1);

	std::string fileName2 = "SolvedMaze.bmp";
	bitmap_image image2(fileName2);
	
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
	Maze(argc, argv);
}