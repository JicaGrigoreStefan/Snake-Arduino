#include "LedControl.h" //libraria de control a matricei led
struct Pin {
	static const short joystickX = A2;   // joystick X 
	static const short joystickY = A3;   // joystick Y 
	static const short joystickVCC = 15; //conectarea virtual VCC 
	static const short joystickGND = 14; //conectarea virtual GND
	static const short potentiometer = A7;
	static const short CLK = 10;   // clock ul pentru matricea led
	static const short CS  = 11;  // chip-select pentru matricea led
	static const short DIN = 12; // data in pentru matricea led
};
const short intensity = 8; // maximul(15) de lumina al matriciei led
const short messageSpeed = 5; //rapiditatea de trecere a mesajului
const short initialSnakeLength = 1; //marime initiala sarpe
void setup() {
	Serial.begin(115200);  // biti monitorului
	initialize();         // pinii si matricea led
	calibrateJoystick(); // se calibreaza joystick ul pt pozitia initiala
	showSnakeMessage(); // scrolarea mesajului sarpe
}
void loop() {
	generateFood();    // daca nu e mancarea o genereaza
	scanJoystick();    // scanare joystick si falfaie mancarea
	calculateSnake();  // pozitia sarpelui
	handleGameStates();  //starea sarpelui(daca a pierdut)
}
LedControl matrix(Pin::DIN, Pin::CLK, Pin::CS, 1);  //se creează LedControl denumit matrix,pentru controlul matricei de LED-uri si se specifică pinii precum și numărul de matrici (1)
struct Point {
	int row = 0, col = 0;
	Point(int row = 0, int col = 0): row(row), col(col) {}    //matrice rand si coloana initial nula
};
struct Coordinate {
	int x = 0, y = 0;
	Coordinate(int x = 0, int y = 0): x(x), y(y) {}   //deasemenea coordonate
};
bool win = false;  //jocul castigat 
bool gameOver = false;  //jocul s a terminat
Point snake;   //capul sarpelui
Point food(-1, -1);   // valori pentru hrana neplasata
Coordinate joystickHome(500, 500);  // snake parameters
int snakeLength = initialSnakeLength; // choosed by the user in the config section
int snakeSpeed = 1; // will be set according to potentiometer value, cannot be 0
int snakeDirection = 0; // if it is 0, the snake does not move
// direction constants
const short up     = 1;
const short right  = 2;
const short down   = 3; // 'down - 2' must be 'up'
const short left   = 4; // 'left - 2' must be 'right'
// threshold where movement of the joystick will be accepted
const int joystickThreshold = 160;  // artificial logarithmity (steepness) of the potentiometer (-1 = linear, 1 = natural, bigger = steeper (recommended 0...1))
const float logarithmity = 0.4;  // snake body segments storage
int gameboard[8][8] = {};
void generateFood() {
	if (food.row == -1 || food.col == -1) {
		// self-explanatory
		if (snakeLength >= 64) {
			win = true;
			return; // prevent the food generator from running, in this case it would run forever, because it will not be able to find a pixel without a snake
		}  // generate food until it is in the right position
		do {
			food.col = random(8);
			food.row = random(8);
		} while (gameboard[food.row][food.col] > 0);
	}
}// watches joystick movements & blinks with food
void scanJoystick() {
	int previousDirection = snakeDirection; // save the last direction
	long timestamp = millis();
	while (millis() < timestamp + snakeSpeed) {     // calculate snake speed exponentially (10...1000ms)
		float raw = mapf(analogRead(Pin::potentiometer), 0, 1023, 0, 1);
		snakeSpeed = mapf(pow(raw, 3.5), 0, 1, 10, 1000); // change the speed exponentially
		if (snakeSpeed == 0) snakeSpeed = 1; // safety: speed can not be 0     // determine the direction of the snake
		analogRead(Pin::joystickY) < joystickHome.y - joystickThreshold ? snakeDirection = up    : 0;
		analogRead(Pin::joystickY) > joystickHome.y + joystickThreshold ? snakeDirection = down  : 0;
		analogRead(Pin::joystickX) < joystickHome.x - joystickThreshold ? snakeDirection = left  : 0;
		analogRead(Pin::joystickX) > joystickHome.x + joystickThreshold ? snakeDirection = right : 0;  // ignore directional change by 180 degrees (no effect for non-moving snake)
		snakeDirection + 2 == previousDirection && previousDirection != 0 ? snakeDirection = previousDirection : 0;
		snakeDirection - 2 == previousDirection && previousDirection != 0 ? snakeDirection = previousDirection : 0;   // intelligently blink with the food
		matrix.setLed(0, food.row, food.col, millis() % 100 < 50 ? 1 : 0);
	}
}
void calculateSnake() {
	switch (snakeDirection) {
		case up:
			snake.row--;
			fixEdge();
			matrix.setLed(0, snake.row, snake.col, 1);
			break;                                                      // calculate snake movement data
		case right:
			snake.col++;
			fixEdge();
			matrix.setLed(0, snake.row, snake.col, 1);
			break;
		case down:
			snake.row++;
			fixEdge();
			matrix.setLed(0, snake.row, snake.col, 1);
			break;
		case left:
			snake.col--;
			fixEdge();
			matrix.setLed(0, snake.row, snake.col, 1);
			break;
		default:               // if the snake is not moving, exit
			return;
	}
	if (gameboard[snake.row][snake.col] > 1 && snakeDirection != 0) {
		gameOver = true;                                                           // if there is a snake body segment, this will cause the end of the game (snake must be moving)
		return;
	}
	if (snake.row == food.row && snake.col == food.col) {         // check if the food was eaten
		food.row = -1; // reset food
		food.col = -1;
		snakeLength++;                          // increment snake length
		for (int row = 0; row < 8; row++) {
			for (int col = 0; col < 8; col++) {
				if (gameboard[row][col] > 0 ) {            // increment all the snake body segments
					gameboard[row][col]++;
				}
			}
		}
	}
	gameboard[snake.row][snake.col] = snakeLength + 1;                        // add new segment at the snake head location            // will be decremented in a moment      // decrement all the snake body segments, if segment is 0, turn the corresponding led off
	for (int row = 0; row < 8; row++) {
		for (int col = 0; col < 8; col++) {
			if (gameboard[row][col] > 0 ) {              // if there is a body segment, decrement it's value
				gameboard[row][col]--;
			}
			matrix.setLed(0, row, col, gameboard[row][col] == 0 ? 0 : 1);                     // display the current pixel
		}
	}
}
void fixEdge() {
	snake.col < 0 ? snake.col += 8 : 0;
	snake.col > 7 ? snake.col -= 8 : 0;                      // causes the snake to appear on the other side of the screen if it gets out of the edge
	snake.row < 0 ? snake.row += 8 : 0;
	snake.row > 7 ? snake.row -= 8 : 0;
}
void handleGameStates() {
	if (gameOver || win) {
		unrollSnake();
		showScoreMessage(snakeLength - initialSnakeLength);
		if (gameOver) showGameOverMessage();
		else if (win) showWinMessage();                            
		win = false;                                     // re-init the game
		gameOver = false;
		snake.row = random(8);
		snake.col = random(8);
		food.row = -1;
		food.col = -1;
		snakeLength = initialSnakeLength;
		snakeDirection = 0;
		memset(gameboard, 0, sizeof(gameboard[0][0]) * 8 * 8);
		matrix.clearDisplay(0);
	}
}
void unrollSnake() {
	matrix.setLed(0, food.row, food.col, 0);                    // switch off the food LED
	delay(800);                    // flash the screen 5 times
	for (int i = 0; i < 5; i++) {
		for (int row = 0; row < 8; row++) {                  // invert the screen   
			for (int col = 0; col < 8; col++) {
				matrix.setLed(0, row, col, gameboard[row][col] == 0 ? 1 : 0);
			}
		}
		delay(20);  // invert it back
		for (int row = 0; row < 8; row++) {
			for (int col = 0; col < 8; col++) {
				matrix.setLed(0, row, col, gameboard[row][col] == 0 ? 0 : 1);
			}
		}
		delay(50);
	}
	delay(600);
	for (int i = 1; i <= snakeLength; i++) {
		for (int row = 0; row < 8; row++) {
			for (int col = 0; col < 8; col++) {
				if (gameboard[row][col] == i) {
					matrix.setLed(0, row, col, 0);
					delay(100);
				}
			}
		}
	}
}
void calibrateJoystick() {
	Coordinate values;
	for (int i = 0; i < 10; i++) {
		values.x += analogRead(Pin::joystickX);       // calibrate the joystick home for 10 times
		values.y += analogRead(Pin::joystickY);
	}
	joystickHome.x = values.x / 10;
	joystickHome.y = values.y / 10;
}
void initialize() {
	pinMode(Pin::joystickVCC, OUTPUT);
	digitalWrite(Pin::joystickVCC, HIGH);
	pinMode(Pin::joystickGND, OUTPUT);
	digitalWrite(Pin::joystickGND, LOW);
	matrix.shutdown(0, false);
	matrix.setIntensity(0, intensity);
	matrix.clearDisplay(0);
	randomSeed(analogRead(A5));
	snake.row = random(8);
	snake.col = random(8);
}
void dumpGameBoard() {
	String buff = "\n\n\n";
	for (int row = 0; row < 8; row++) {
		for (int col = 0; col < 8; col++) {
			if (gameboard[row][col] < 10) buff += " ";
			if (gameboard[row][col] != 0) buff += gameboard[row][col];
			else if (col == food.col && row == food.row) buff += "@";
			else buff += "-";
			buff += " ";
		}
		buff += "\n";
	}
	Serial.println(buff);
}
const PROGMEM bool snakeMessage[8][56] = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};
const PROGMEM bool gameOverMessage[8][90] = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};
const PROGMEM bool scoreMessage[8][58] = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};
const PROGMEM bool digits[][8][8] = {
	{
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 1, 1, 1, 1, 0, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 1, 1, 0, 1, 1, 1, 0},
		{0, 1, 1, 1, 0, 1, 1, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 0, 1, 1, 1, 1, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 1, 1, 0, 0, 0},
		{0, 0, 0, 1, 1, 0, 0, 0},
		{0, 0, 1, 1, 1, 0, 0, 0},
		{0, 0, 0, 1, 1, 0, 0, 0},
		{0, 0, 0, 1, 1, 0, 0, 0},
		{0, 0, 0, 1, 1, 0, 0, 0},
		{0, 1, 1, 1, 1, 1, 1, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 1, 1, 1, 1, 0, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 0, 0, 0, 0, 1, 1, 0},
		{0, 0, 0, 0, 1, 1, 0, 0},
		{0, 0, 1, 1, 0, 0, 0, 0},
		{0, 1, 1, 0, 0, 0, 0, 0},
		{0, 1, 1, 1, 1, 1, 1, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 1, 1, 1, 1, 0, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 0, 0, 0, 0, 1, 1, 0},
		{0, 0, 0, 1, 1, 1, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 0, 1, 1, 1, 1, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 1, 0, 0},
		{0, 0, 0, 1, 1, 1, 0, 0},
		{0, 0, 1, 0, 1, 1, 0, 0},
		{0, 1, 0, 0, 1, 1, 0, 0},
		{0, 1, 1, 1, 1, 1, 1, 0},
		{0, 0, 0, 0, 1, 1, 0, 0},
		{0, 0, 0, 0, 1, 1, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 1, 1, 1, 1, 1, 1, 0},
		{0, 1, 1, 0, 0, 0, 0, 0},
		{0, 1, 1, 1, 1, 1, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 0},
		{0, 0, 0, 0, 0, 1, 1, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 0, 1, 1, 1, 1, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 1, 1, 1, 1, 0, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 1, 1, 0, 0, 0, 0, 0},
		{0, 1, 1, 1, 1, 1, 0, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 0, 1, 1, 1, 1, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 1, 1, 1, 1, 1, 1, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 0, 0, 0, 1, 1, 0, 0},
		{0, 0, 0, 0, 1, 1, 0, 0},
		{0, 0, 0, 1, 1, 0, 0, 0},
		{0, 0, 0, 1, 1, 0, 0, 0},
		{0, 0, 0, 1, 1, 0, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 1, 1, 1, 1, 0, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 0, 1, 1, 1, 1, 0, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 0, 1, 1, 1, 1, 0, 0}
	},
{
		{0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 1, 1, 1, 1, 0, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 0, 1, 1, 1, 1, 1, 0},
		{0, 0, 0, 0, 0, 1, 1, 0},
		{0, 1, 1, 0, 0, 1, 1, 0},
		{0, 0, 1, 1, 1, 1, 0, 0}
	}
};
void showSnakeMessage() {
	[&] {
		for (int d = 0; d < sizeof(snakeMessage[0]) - 7; d++) {           // scrolls the 'snake' message around the matrix
			for (int col = 0; col < 8; col++) {
				delay(messageSpeed);
				for (int row = 0; row < 8; row++) {
					matrix.setLed(0, row, col, pgm_read_byte(&(snakeMessage[row][col + d])));          	// this reads the byte from the PROGMEM and displays it on the screen
				}
			}
			if (analogRead(Pin::joystickY) < joystickHome.y - joystickThreshold
			        || analogRead(Pin::joystickY) > joystickHome.y + joystickThreshold          	// if the joystick is moved, exit the message
			        || analogRead(Pin::joystickX) < joystickHome.x - joystickThreshold
			        || analogRead(Pin::joystickX) > joystickHome.x + joystickThreshold) {
				return; 
			}
		}
	}();
	matrix.clearDisplay(0);
	while (analogRead(Pin::joystickY) < joystickHome.y - joystickThreshold
	        || analogRead(Pin::joystickY) > joystickHome.y + joystickThreshold              // wait for joystick co come back
	        || analogRead(Pin::joystickX) < joystickHome.x - joystickThreshold
	        || analogRead(Pin::joystickX) > joystickHome.x + joystickThreshold) {}

}
void showGameOverMessage() {
	[&] {
		for (int d = 0; d < sizeof(gameOverMessage[0]) - 7; d++) {                              // scrolls the 'game over' message around the matrix
			for (int col = 0; col < 8; col++) {
				delay(messageSpeed);
				for (int row = 0; row < 8; row++) {
					matrix.setLed(0, row, col, pgm_read_byte(&(gameOverMessage[row][col + d])));
				}
			}

			if (analogRead(Pin::joystickY) < joystickHome.y - joystickThreshold
			        || analogRead(Pin::joystickY) > joystickHome.y + joystickThreshold                   			// if the joystick is moved, exit the message
			        || analogRead(Pin::joystickX) < joystickHome.x - joystickThreshold
			        || analogRead(Pin::joystickX) > joystickHome.x + joystickThreshold) {
				return; 
			}
		}
	}();
	matrix.clearDisplay(0);
	while (analogRead(Pin::joystickY) < joystickHome.y - joystickThreshold
	        || analogRead(Pin::joystickY) > joystickHome.y + joystickThreshold              	// wait for joystick co come back
	        || analogRead(Pin::joystickX) < joystickHome.x - joystickThreshold
	        || analogRead(Pin::joystickX) > joystickHome.x + joystickThreshold) {}
}
void showWinMessage() {
}
void showScoreMessage(int score) {
	if (score < 0 || score > 99) return;
	int second = score % 10;
	int first = (score / 10) % 10;
	[&] {
		for (int d = 0; d < sizeof(scoreMessage[0]) + 2 * sizeof(digits[0][0]); d++) {
			for (int col = 0; col < 8; col++) {
				delay(messageSpeed);
				for (int row = 0; row < 8; row++) {
					if (d <= sizeof(scoreMessage[0]) - 8) {
						matrix.setLed(0, row, col, pgm_read_byte(&(scoreMessage[row][col + d])));
					}
					int c = col + d - sizeof(scoreMessage[0]) + 6; // move 6 px in front of the previous message
					// if the score is < 10, shift out the first digit (zero)
					if (score < 10) c += 8;
					if (c >= 0 && c < 8) {
						if (first > 0) matrix.setLed(0, row, col, pgm_read_byte(&(digits[first][row][c]))); // show only if score is >= 10 (see above)
					} else {
						c -= 8;
						if (c >= 0 && c < 8) {
							matrix.setLed(0, row, col, pgm_read_byte(&(digits[second][row][c]))); // show always
						}
					}
				}
			}
			if (analogRead(Pin::joystickY) < joystickHome.y - joystickThreshold
			        || analogRead(Pin::joystickY) > joystickHome.y + joystickThreshold             	// if the joystick is moved, exit the message
			        || analogRead(Pin::joystickX) < joystickHome.x - joystickThreshold
			        || analogRead(Pin::joystickX) > joystickHome.x + joystickThreshold) {
				return;
			}
		}
	}();
	matrix.clearDisplay(0);
}
float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}