#include <FastLED.h>

#define LED_PIN     8
#define NUM_LEDS    60 
#define BRIGHTNESS  200
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 300




CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

//=====communication====
bool connectionEstablished = false;
//used to iterate over inputBuffer
bool newData = false;

//=====SyncVariables=====
bool LocalAnim = false;
bool Sync = true;
CRGBPalette16 serialPalette;
float brigntness = 1;
int bpm = 128;

void setup() {
	delay(3000); // power-up safety delay
	//dissable led dithering to remove flickering when serial is being proccesed.
	FastLED.setDither(0);
	Serial.begin(500000);
	Serial.setTimeout(3);
	FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
	FastLED.setBrightness(BRIGHTNESS);

	currentPalette = RainbowColors_p;
	currentBlending = LINEARBLEND;
}


const int bufferSize = 18;
char inputBuffer[bufferSize];
unsigned long deltaTime = 0;
void loop()
{
	unsigned long oldTime = micros();

		//Serial.readBytesUntil('>', inputBuffer, 15);
	processSerial();
	if (newData) {
		parseinput(inputBuffer);
		newData = false;
	}

	//will update the pallet mapping every loop if there are animations running localy on the arduino. 
	//delta time should be used for frame-independent animations
	if (LocalAnim) {
		static uint8_t startIndex = 0;
		startIndex = startIndex + 0; /* motion speed */
		FillLEDsFromPaletteColors(startIndex);
	}
		//FastLED.show();
		//FastLED.delay(positive((1000 / UPDATES_PER_SECOND) - (millis() - deltaTime)));
	

	//calculates the amount of time all of the code took to run and takes that into account with the delay
	//The max resolution is multiples of 4 microseconds
	deltaTime = micros() - oldTime;


}


//takes in one character at a time then marks the input data as new once it finds an endmarker
void processSerial() {
	static byte ndx = 0;
	char endMarker = '>';
	char rc;

	while (Serial.available() > 0 && newData == false) {
		rc = Serial.read();
		interSerial();

		if (rc != endMarker) {
			inputBuffer[ndx] = rc;
			ndx++;
			if (ndx >= bufferSize) {
				ndx = bufferSize - 1;
			}
		}
		else {
			inputBuffer[ndx] = '\0'; // terminate the string
			//debug
			//Serial.print("process");
			//Serial.println(inputBuffer);
			ndx = 0;
			newData = true;
		}
	}

}

//this meathod is run on each character read from serial. This can take up to 20 microseconds without causing significant slowdowns.
void interSerial() {

}


int positive(const int &input){
	if (input < 0) {
		return 0;
	}
}

void parseinput(const char *input) {
	if (memcmp(input, "hex", 3) == 0) {
		char index[3];
		//this meathod directly modifys the input c string 
		getStringBetweenDelimiters(input, ":", ";", index);

		//stores a pointer so that it can iterate over the hex in the string
		char *p;
		p = strstr(input, ";") + 1;
		currentPalette[atoi(index)] = CRGB(
			//p is just a pointer and * gets the value of that memory address. So we add to the pointer to iterate over the array
			HextoByte(*p, *(p+1)), HextoByte(*(p+2), *(p+3)), HextoByte(*(p+4), *(p+5))
		);
	}
	else if (memcmp(input, "ref", 3) == 0) {
		FillLEDsFromPaletteColors(0);
		FastLED.show();
		Serial.println(deltaTime);
	}
	//prints the last frames deltaTime
}


int getStringBetweenDelimiters(const char* string, const char* leftDelimiter, const char* rightDelimiter, char* out)
{
	// find the left delimiter and use it as the beginning of the substring
	const char* beginning = strstr(string, leftDelimiter);
	if (beginning == NULL)
		return 1; // left delimiter not found

	// find the right delimiter
	const char* end = strstr(string, rightDelimiter);
	if (end == NULL)
		return 2; // right delimiter not found

	// offset the beginning by the length of the left delimiter, so beginning points _after_ the left delimiter
	beginning += strlen(leftDelimiter);

	// get the length of the substring
	ptrdiff_t segmentLength = end - beginning;
	// allocate memory and copy the substring there
	//*out = malloc(segmentLength + 1);
	strncpy(out, beginning, segmentLength);
	(out)[segmentLength] = 0;
	return 0; // success!
}


uint8_t HextoByte(char& x16, char& x1) {
	uint8_t decimal = HexLetterToNumber(x1) + 16 * HexLetterToNumber(x16);
	return decimal;
}

uint8_t HexLetterToNumber(char& x) {
	switch (x)
	{
	case('a'):
		return 10;
		break;
	case('b'):
		return 11;
		break;
	case('c'):
		return 12;
		break;
	case('d'):
		return 13;
		break;
	case('e'):
		return 14;
		break;
	case('f'):
		return 15;
	default:
		return int(x) - 48;
		break;
	}
}


void FillLEDsFromPaletteColors(uint8_t colorIndex)
{
	uint8_t brightness = 0xff;
	for (int i = 0; i < NUM_LEDS; i++) {
		leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
		colorIndex += 4;
	}
}


// There are several different palettes of colors demonstrated here.
//
// FastLED provides several 'preset' palettes: RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p.
//
// Additionally, you can manually define your own color palettes, or you can write
// code that creates color palettes on the fly.  All are shown here.

void ChangePalettePeriodically()
{
	uint8_t secondHand = (millis() / 1000) % 60;
	static uint8_t lastSecond = 99;

	if (lastSecond != secondHand) {
		lastSecond = secondHand;
		if (secondHand == 0) { currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; }
		if (secondHand == 10) { currentPalette = RainbowStripeColors_p;   currentBlending = NOBLEND; }
		if (secondHand == 15) { currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; }
		if (secondHand == 20) { SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; }
		if (secondHand == 25) { SetupTotallyRandomPalette();              currentBlending = LINEARBLEND; }
		if (secondHand == 30) { SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND; }
		if (secondHand == 35) { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; }
		if (secondHand == 40) { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; }
		if (secondHand == 45) { currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; }
		if (secondHand == 50) { currentPalette = myRedWhiteBluePalette_p; currentBlending = NOBLEND; }
		if (secondHand == 55) { currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; }
	}
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
	for (int i = 0; i < 16; i++) {
		currentPalette[i] = CHSV(random8(), 255, random8());
	}
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
	// 'black out' all 16 palette entries...
	fill_solid(currentPalette, 16, CRGB::Black);
	// and set every fourth one to white.
	currentPalette[0] = CRGB::White;
	currentPalette[4] = CRGB::White;
	currentPalette[8] = CRGB::White;
	currentPalette[12] = CRGB::White;

}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
	CRGB purple = CHSV(HUE_PURPLE, 255, 255);
	CRGB green = CHSV(HUE_GREEN, 255, 255);
	CRGB black = CRGB(0, 0, 0);

	currentPalette = CRGBPalette16(
		green, green, black, black,
		purple, purple, black, black,
		green, green, black, black,
		purple, purple, black, black);
}


// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
	CRGB::Red,
	CRGB::Gray, // 'white' is too bright compared to red and blue
	CRGB::Blue,
	CRGB::Black,

	CRGB::Red,
	CRGB::Gray,
	CRGB::Blue,
	CRGB::Black,

	CRGB::Red,
	CRGB::Red,
	CRGB::Gray,
	CRGB::Gray,
	CRGB::Blue,
	CRGB::Blue,
	CRGB::Black,
	CRGB::Black
};
