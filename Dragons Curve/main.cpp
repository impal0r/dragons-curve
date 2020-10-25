#include "SDL.h"
#include "SDL_syswm.h"
#include "SDL_ttf.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <ShellScalingApi.h>

/*
const char HELP_MSG[] =
"Iteratively generates the Dragon's Curve and displays it on screen.\n\
Press Alt-F4 to quit when in fullscreen mode.\n\
Note: the provided DLLs and fixedsys.ttf file must stay in the same directory as the executable.\n\
\n\
USAGE: \"Dragons Curve.exe\" [/?] [/option=value] [/configfile=PATH] \n\
\n\
OPTIONS:\n\
  /configfile=  Specify the path of the configuration file (default = \"./dragons-curve.cfg\")\n\
                Note: command line arguments override any options in a config file\n\
  /min_wait=    The number of milliseconds to wait before displaying successive iterations\n\
                (default = 500)\n\
  /fullscr=     Display the dragon's curve fullscreen or not (1 or 0) (default = 1)\n\
  /width=       Width of display window (ignored if fullscreen=1) (default = 800)\n\
  /height=      Height of display window (ignored if fullscreen=1) (default = 500)\n\
  /upto=        The iteration number to stop at (default = -1)\n\
  /?            Show this message and exit\n\
\n\
EXAMPLES:\n\
  \"Dragons Curve.exe\" /configfile=\"\"     --to run with all default values\n\
  \"Dragons Curve.exe\" /fullscr=0 /width=1000 /height=625 /min_wait=0\n\
  \"Dragons Curve.exe\" /configfile=\"C:\\Path\\Through\\Directory tree\\dragons-curve.cfg\"\n";
*/
const SDL_Color BLACK = { 0, 0, 0 };
const SDL_Color WHITE = { 255, 255, 255 };
const int fontSize = 28;
const int FPS = 30;

//Node of a singly linked list of points for dragons curve
struct coordsNode {
	float x;
	float y;
	coordsNode* next;
};

int iterateDragonsCurve(coordsNode* firstNode, SDL_Renderer* renderer) {
	bool LR = true; //false:left, true:right, we alternate bending left and right
	float tmp;
	coordsNode* tempNode1 = firstNode;
	coordsNode* tempNode2 = firstNode->next;
	coordsNode* extraNode;
	SDL_Event event;
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	while (true) {
		//Check if anything happened (so window doesn't become unresponsive)
		SDL_PollEvent(&event);
		switch (event.type)
		{
		case SDL_QUIT:
			return 0;
		}
		//add node between tempNode1 and tempNode2
		extraNode = static_cast<coordsNode*>(malloc(sizeof(coordsNode)));
		if (extraNode == NULL) {
			printf("Error: Out of memory\n");
			return 0;
		}
		if (tempNode1->y == tempNode2->y) { //east or west
			extraNode->x = (tempNode1->x + tempNode2->x) / 2;
			tmp = abs(tempNode1->x - tempNode2->x) / 2;
			if ((tempNode1->x < tempNode2->x && LR) || (tempNode1->x > tempNode2->x && !LR)) {
				extraNode->y = tempNode1->y + tmp;
			} else {
				extraNode->y = tempNode1->y - tmp;
			}
		}
		else if (tempNode1->x == tempNode2->x) { //north or south
			extraNode->y = (tempNode1->y + tempNode2->y) / 2;
			tmp = abs(tempNode1->y - tempNode2->y) / 2;
			if ((tempNode1->y < tempNode2->y && LR) || (tempNode1->y > tempNode2->y && !LR)) {
				extraNode->x = tempNode1->x - tmp;
			} else {
				extraNode->x = tempNode1->x + tmp;
			}
		}
		else { //diagonals
			//determine if x and y direction of line segment both positive or both negative
			//(we know magnitudes will be equal)
			if (LR ^ (tempNode2->x - tempNode1->x == tempNode2->y - tempNode1->y)) {
				extraNode->x = tempNode2->x;
				extraNode->y = tempNode1->y;
			} else {
				extraNode->x = tempNode1->x;
				extraNode->y = tempNode2->y;
			}
		}
		tempNode1->next = extraNode;
		extraNode->next = tempNode2;
		//Draw new lines
		SDL_RenderDrawLineF(renderer, tempNode1->x, tempNode1->y, extraNode->x, extraNode->y);
		SDL_RenderDrawLineF(renderer, extraNode->x, extraNode->y, tempNode2->x, tempNode2->y);
		//iteration
		if (!tempNode2->next) break;
		tempNode1 = tempNode2;
		tempNode2 = tempNode2->next;
		LR = !LR;
	}
	return 1;
}

int main(int argc, char* argv[]) {
	//"Constants" ie. parameters
	bool FULLSCREEN = true;
	int WIDTH = 800;
	int HEIGHT = 500;
	int MIN_ITERATION_TIME = 500;
	unsigned int UPTO_ITERATION = -1;
	float START_X = -1;
	float START_Y = -1;
	float END_X = -1;
	float END_Y = -1;
	//which parameters have we set on the cmd line? (Cmd line args take precedence.) A bitmap of flags
	int set = 0;
	const int SET_FULLSCREEN = 1;
	const int SET_WIDTH = 2;
	const int SET_HEIGHT = 4;
	const int SET_MS_PER_ITER = 8;
	const int SET_ITERATION = 16;
	const int SET_START_X = 32;
	const int SET_START_Y = 64;
	const int SET_END_X = 128;
	const int SET_END_Y = 256;

	//AttachConsole(ATTACH_PARENT_PROCESS); //Get a console (maybe?)

	//Read cmd line args
	int i = 1;
	std::string key;
	std::string value;
	std::string config_file_path = ".\\dragons-curve.cfg";
	bool reading_path = false;
	while (i < argc) {
		if (reading_path) {
			value = value + " " + argv[i]; //c++ string concat
			if (value.back() == '"') {
				config_file_path = value.substr(1, value.size() - 2);
				std::cout << "Command line: Config file path is " << value << "\n";
			}
			else continue;
		}
		else if (*argv[i] == '/') {
			std::istringstream arg_stream(argv[i] + 1); //take the '/' off the start
			if (std::getline(arg_stream, key, '=')) {
				if (std::getline(arg_stream, value)) {
					if (key == "configfile") {
						if (value[0] == '"') {
							if (value.size() > 2 and value.back() == '"') {
								config_file_path = value.substr(0, value.size() - 2);;
								//std::cout << "Config file at " << value << "\n";
							}
							else if (value.size() <= 2) {
								continue;
							}
							else {
								reading_path = true;
							}
						}
						else { //path not quoted
							config_file_path = value;
						}
					}
					std::istringstream value_stream(value);
					if (key == "min_wait") {
						value_stream >> MIN_ITERATION_TIME;
						set |= SET_MS_PER_ITER;
						//std::cout << "Command line: MIN_ITERATION_TIME set to " << MIN_ITERATION_TIME << " milliseconds\n";
					} else if (key == "fullscr") {
						value_stream >> FULLSCREEN;
						set |= SET_FULLSCREEN;
						//std::cout << "Command line: FULLSCREEN set to " << (FULLSCREEN?"true":"false") << "\n";
					} else if (key == "width") {
						value_stream >> WIDTH;
						set |= SET_WIDTH;
						//std::cout << "Command line: WIDTH set to " << WIDTH << "\n";
					} else if (key == "height") {
						value_stream >> HEIGHT;
						set |= SET_HEIGHT;
						//std::cout << "Command line: HEIGHT set to " << HEIGHT << "\n";
					} else if (key == "upto") {
						value_stream >> UPTO_ITERATION;
						set |= SET_ITERATION;
						//std::cout << "Command line: UPTO_ITERATION set to " << UPTO_ITERATION << "\n";
					} else if (key == "startx") {
						value_stream >> START_X;
						set |= SET_START_X;
					} else if (key == "starty") {
						value_stream >> START_Y;
						set |= SET_START_Y;
					} else if (key == "endx") {
						value_stream >> END_X;
						set |= SET_END_X;
					} else if (key == "endy") {
						value_stream >> END_Y;
						set |= SET_END_Y;
					}
				}
				/*
				else if (key == "?") {
					printf(HELP_MSG);
					return 0;
				}*/
			}
		}
		i++;
	}

	//Read configuration from file (if it exists)
	std::ifstream configFile;
	std::string line;
	configFile.open(config_file_path);
	if (configFile.is_open()) {
		while (std::getline(configFile, line)) {
			std::istringstream line_stream(line);
			if (std::getline(line_stream, key, '=')) {
				if (std::getline(line_stream, value)) {
					std::istringstream value_stream(value);
					if (key == "MIN_ITERATION_TIME" and !(set & SET_MS_PER_ITER)) {
						value_stream >> MIN_ITERATION_TIME;
						//std::cout << "Configuration file: MIN_ITERATION_TIME set to " << MIN_ITERATION_TIME << " milliseconds\n";
					} else if (key == "FULLSCREEN" and !(set & SET_FULLSCREEN)) {
						value_stream >> FULLSCREEN;
						//std::cout << "Configuration file: FULLSCREEN set to " << (FULLSCREEN?"true":"false") << "\n";
					} else if (key == "WIDTH" and !(set & SET_WIDTH)) {
						value_stream >> WIDTH;
						//std::cout << "Configuration file: WIDTH set to " << WIDTH << "\n";
					} else if (key == "HEIGHT" and !(set & SET_HEIGHT)) {
						value_stream >> HEIGHT;
						//std::cout << "Configuration file: HEIGHT set to " << HEIGHT << "\n";
					} else if (key == "UPTO_ITERATION" and !(set & SET_ITERATION)) {
						value_stream >> UPTO_ITERATION;
						//std::cout << "Configuration file: UPTO_ITERATION set to " << UPTO_ITERATION << "\n";
					} else if (key == "START_X" and !(set & SET_START_X)) {
						value_stream >> START_X;
					} else if (key == "START_Y" and !(set & SET_START_Y)) {
						value_stream >> START_Y;
					} else if (key == "END_X" and !(set & SET_END_X)) {
						value_stream >> END_X;
					} else if (key == "END_Y" and !(set & SET_END_Y)) {
						value_stream >> END_Y;
					}
				}
			}
		}
	}
	//else printf("Could not open config file.");

	//Declarations
	int flags;
	int actual_width;
	int actual_height;
	SDL_Window* window = NULL;
	SDL_Surface* screenSurface = NULL;
	SDL_Renderer* renderer = NULL;
	TTF_Font* numberFont = NULL;
	SDL_Surface* numberSurface = NULL;
	SDL_Texture* numberTexture = NULL;
	SDL_Rect textrect;
	int textWidth, textHeight;
	SDL_Event event;
	//Init SDL Version (so that we can get window hwnd later)
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);

	//Set DPI awareness (so is not blurry)
	SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE); //from ShellScalingApi

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		goto quit;
	}
	//Initialise TTF
	if (TTF_Init() < 0) {
		printf("SDL_ttf could not initialize! SDL_Error: %s\n", TTF_GetError());
		goto quit;
	}

	//Create window (SDL_WINDOW_ALLOW_HIGHDPI doesnt seem to do anything?)
	flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_FULLSCREEN * FULLSCREEN;
	window = SDL_CreateWindow("Dragon's Curve", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH*!FULLSCREEN, HEIGHT*!FULLSCREEN, flags);
	if (window == NULL)
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
	}
	else
	{
		//Get window scaling level
		SDL_GetWindowWMInfo(window, &wmInfo);
		HWND hwnd = wmInfo.info.win.window;
		UINT dpiLevel = GetDpiForWindow(hwnd); //from ShellScalingApi
		//Set scaling level
		float dpiPercent = dpiLevel / 96.0F;
		//Update window dimensions
		if (FULLSCREEN) { //Get the actual size if we are fullscreen
			SDL_GetWindowSize(window, &actual_width, &actual_height);
		}
		else { //scale by dpi if not
			actual_width = (int)(WIDTH * dpiPercent);
			actual_height = (int)(HEIGHT * dpiPercent);
			SDL_SetWindowSize(window, actual_width, actual_height);
		}

		//Get window surface
		screenSurface = SDL_GetWindowSurface(window);
		//Init renderer
		renderer = SDL_CreateRenderer(window, -1, 0);
		//Get font
		numberFont = TTF_OpenFont("fixedsys.ttf", fontSize);

		if (START_X < 0 or START_X > actual_width) START_X = actual_width * 0.25F;
		if (START_Y < 0 or START_Y > actual_height) START_Y = actual_height * 0.34F;
		if (END_X < 0 or END_X > actual_width) END_X = actual_width * 0.8F;
		if (END_Y < 0 or END_Y > actual_height) END_Y = actual_height * 0.34F;
		//Implement the dragon's curve as a singly linked list
		coordsNode lastNode = { END_X, END_Y, NULL };
		coordsNode firstNode = { START_X, START_Y, &lastNode };
		//Draw 0th iteration
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderDrawLineF(renderer, (&firstNode)->x, (&firstNode)->y, (&lastNode)->x, (&lastNode)->y);

		unsigned int iterCounter = 0;
		const int BUFFERSIZE = 8; //8 digits will definitely be enough
		char iterCounterStr[BUFFERSIZE] = "0";
		UINT32 lastChangeTime = SDL_GetTicks();
		bool quit = false;
		while (!quit) {
			SDL_PollEvent(&event);
			switch (event.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			}

			if (/*SDL_GetTicks() >= lastChangeTime + MS_PER_ITER &&*/ iterCounter <= UPTO_ITERATION) {
				//Fill the screen white
				SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
				SDL_RenderFillRect(renderer, NULL);
				//Render iterCounter
				snprintf(iterCounterStr, BUFFERSIZE, "%d", iterCounter++); //Increments iterCounter before render
				SDL_FreeSurface(numberSurface);
				SDL_DestroyTexture(numberTexture);
				numberSurface = TTF_RenderText_Solid(numberFont, iterCounterStr, BLACK);
				numberTexture = SDL_CreateTextureFromSurface(renderer, numberSurface);
				SDL_QueryTexture(numberTexture, NULL, NULL, &textWidth, &textHeight);
				textrect = { 5, 2, (int)(textWidth * dpiPercent) , (int)(textHeight * dpiPercent) };
				//Show iterCounter
				SDL_RenderCopy(renderer, numberTexture, NULL, &textrect);
				//Iterate curve (draws it as well)
				int success = iterateDragonsCurve(&firstNode, renderer);
				if (!success) goto quit;
				//Show frame
				SDL_RenderPresent(renderer); //updates surface as well
				lastChangeTime = SDL_GetTicks();
			}

			//Wait one frame
			SDL_Delay(1000/FPS);
		}
	}

quit:
	//Destroy window
	TTF_CloseFont(numberFont);
	TTF_Quit();
	SDL_DestroyTexture(numberTexture);
	SDL_FreeSurface(numberSurface);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	//Quit SDL subsystems
	SDL_Quit();

	return 0;
}