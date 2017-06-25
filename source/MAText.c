#include "MAText.h"

#include "gfx.h"
#include <stdio.h>	  // For sprintf. Remove this when no longer needed
#include <stdlib.h>
#include <string.h>

int getCharIndex(MAFont * font, uint8_t asciiID) {
	int count = font->nChars;
	int i;

	for (i=0; i<count; i++) {
		MACharDesc aCharDesc = font->charDescs[i];
		if (aCharDesc.asciiID == asciiID) {
			return i;
		}
	}

	return -1;
}

int getCharIndexOrErrorCharIndex(MAFont * font, uint8_t asciiID) {
	if (asciiID == 0) {
		return -1;
	}

	//Try to get the index of the actual character
	int charIndex = getCharIndex(font, asciiID);

	//If no index was found, try to get the index of the '?' character
	if (charIndex == -1) {
		charIndex = getCharIndex(font, 63);
	}

	//If no index was found, try to get the index of the space character
	if (charIndex == -1) {
		charIndex = getCharIndex(font, 32);
	}

	//If no index was found by this time, -1 will be returned

	//Return whatever index was found
	return charIndex;
}

int MADrawCharacter(gfxScreen_t screen, gfx3dSide_t side, MAFont *font, int charDescIndex, int cursorx, int cursory, int red, int green, int blue) {
	MACharDesc aCharDesc = font->charDescs[charDescIndex];

	//Get the image using the x/y/w/h from the image data
	//Memcpy this to a new image

	int dataOffsetTop = (aCharDesc.x * font->pngW);
	int dataOffsetLeft = ( font->pngW - aCharDesc.y - aCharDesc.h );
	int initialSourceByte = dataOffsetTop + dataOffsetLeft;
	int imageDataSize = (aCharDesc.w * aCharDesc.h)*4;

	u8 destImageData[imageDataSize];
	int destByte = 0;
	int row, col;
	for (row=0; row<aCharDesc.w; row++) {
		for (col=0; col<aCharDesc.h; col++) {
			int rowOffset = (row * font->pngW);
			int colOffset = col;
			int sourceByte = initialSourceByte + rowOffset + colOffset;

			memcpy(&destImageData[destByte], &blue, 1);					 destByte++;
			memcpy(&destImageData[destByte], &green, 1);					destByte++;
			memcpy(&destImageData[destByte], &red, 1);					  destByte++;
			memcpy(&destImageData[destByte], &font->data[sourceByte], 1);   destByte++;
		}
	}

	//Draw the character
	gfxDrawSpriteAlphaBlend(screen, side, destImageData, aCharDesc.h, aCharDesc.w, cursorx, cursory);

	//Return the width of the character (for moving the cursor along to draw the next character)
	return aCharDesc.w;
}

void truncateTextToFitMaxWidth(char * text, int maxWidth, MAFont * font, bool force) {
	//If a maximum width in pixels was specified
	if (maxWidth > 0) {
		//Get the current width
		int textWidth = MATextWidthInPixels(text, font);

		//If the current width exceeds the maximum width
		if (textWidth > maxWidth) {
			//Start by truncating the original string by four characters - enough to remove one and add '...'
			int numTruncatedChars = 4;

			//Create a new string to contain, at a minimum, the original string truncated by one character and with '...' added
			char * truncatedText = malloc(strlen(text));

			//While the string is too big to fit
			while (textWidth > maxWidth) {
				//Erase the truncated string (all of it - not just the bit that was truncated before)
				memset(&truncatedText[0], 0, strlen(text));

				//Copy the original string, minus the number of characters which need to be truncated, to the truncated string
				strncpy(truncatedText, text, strlen(text)-numTruncatedChars);

				//Add '...' to the end
				strcat(truncatedText, "...");

				//Recalculate the width of the truncated string
				textWidth = MATextWidthInPixels(truncatedText, font);

				//Increase the number of characters to truncate in case we need to loop round again and make the string even shorter next time
				numTruncatedChars++;

				//If we have truncated everything, give up and break. The original string will be drawn, but might not fit
				if (numTruncatedChars >= strlen(text)) {
					break;
				}
			}

			//By this point, we should have a truncated string with '...' on the end which will fit within maxWidth
			//Erase the original (too long) string
			memset(&text[0], 0, strlen(text));

			//Overwrite it with the truncated string
			strncpy(text, truncatedText, strlen(truncatedText));

			//Free the memory for the truncated string
			free(truncatedText);
		}

		//If the text will fit but we want to force ellipses onto it
		else if (force) {
			char forced[strlen(text) + 4];
			strcpy(forced, text);
			strcat(forced, "...");
			memset(&text[0], 0, strlen(text));
			strncpy(text, forced, strlen(forced));
		}
	}
}

void MADrawText(gfxScreen_t screen, gfx3dSide_t side, int cursorx, int cursory, char * text, MAFont * font, int red, int green, int blue) {

	int offset = 0;

	char out[1024];
	strcpy(out, "");

	while(offset>-1) {
		uint8_t code = (uint8_t)text[offset];

		char s[8];
		memset(s, 0, 8);
		sprintf(s, "%u\n", code);
		strcat(out, s);

		offset += 1;
		if (offset >= strlen(text)) offset = -1;


		/*
		 We now have the correct character code, corrected for unicode chars.
		 Yay!!!!!
		 We should now be able to match the character to the code in the struct and draw it. Fingers crossed...
		 */

		int charIndex = getCharIndexOrErrorCharIndex(font, code);
		if (charIndex > -1) {
			int xAdvance = MADrawCharacter(screen, side, font, charIndex, cursorx, cursory, red, green, blue);
			cursory += xAdvance;
		}
	}

}

bool stringContainsChar(char * string, char * c) {
	char * result = strpbrk(string, c);
	return (result != NULL);
}

bool charIsWordBreak(char * c) {
	return stringContainsChar(".,-?! ", c);
}

bool charIsNewline(char * c) {
	return stringContainsChar("\n\r", c);
}

int MADrawTextWrap(gfxScreen_t screen, gfx3dSide_t side, int cursorx, int cursory, char * text, MAFont * font, int red, int green, int blue, int maxWidth, int maxLines) {

	//If no maximum width was specified, just draw the text
	if (maxWidth < 1) {
		MADrawText(screen, side, cursorx, cursory, text, font, red, green, blue);
		return 1;
	}

	if (maxLines == 1) {
		truncateTextToFitMaxWidth(text, maxWidth, font, false);
		MADrawText(screen, side, cursorx, cursory, text, font, red, green, blue);
		return 1;
	}

	//Store initial cursor position in case we need to go down a line
	int initialy = cursory;

	int lineHeight = font->lineHeight;

	//Start from line 1
	int currentline = 1;

	int maxWords = 128;
	int maxWordLen = 64;

	char words[maxWords][maxWordLen];
	memset(words, 0, maxWords*maxWordLen);

	int numWords = 0;

	int numChars = strlen(text);
	int currentWordStartPosition = 0;
	int i=0;

	/*
	 This was originally added for testing the output of the sentence splitting function below.
	 It is no longer used or even referred to beyond its declaration here and copying one character to it.
	 I have no idea why, but if this is removed the text wrapping goes all fucked.
	 I should probably try to figure it out, but I would rather have text wrapping working than
		save 1k of memory, and I don't really have the time to figure it out.
	 */
	char out[1024];
	strcpy(out, "|");

	while (i < numChars) {
		char c[1];
		memset(c, 0, 1);
		strncpy(c, &text[i], 1);

		if (charIsNewline(c)) { //We have reached a newline character
			int currentWordEndPosition = i;
			int currentWordLength = currentWordEndPosition - currentWordStartPosition;
			char currentWord[maxWordLen];
			memset(currentWord, 0, maxWordLen);
			strncpy(currentWord, &text[currentWordStartPosition], currentWordLength);
			strncpy(words[numWords], currentWord, maxWordLen);
			numWords++;
			i++;
			currentWordStartPosition = i;

			char newlineWord[13];
			strcpy(newlineWord, "***NEWLINE***");
			strncpy(words[numWords], newlineWord, maxWordLen);
			numWords++;
		} else if (charIsWordBreak(c)) { //We have reached the end of a word

			//Now scan until the beginning of the next word or the end of the string
			while (true) {
				i++; //Go to the next character

				//If we have reached the end of the string, stop scanning
				if (i >= numChars) {
					break;
				}

				//If we have not reached the end of the string, get the next character
				char c2[1];
				memset(c2, 0, 1);
				strncpy(c2, &text[i], 1);

				//If we have reached a non word break character, stop scanning
				if (!charIsWordBreak(c2)) {
					break;
				}
			}

			int currentWordEndPosition = i; //Get the index of the character and store it as current word end position

			int currentWordLength = currentWordEndPosition - currentWordStartPosition; //Calculate the length of the current word

			char currentWord[maxWordLen];
			memset(currentWord, 0, maxWordLen);
			strncpy(currentWord, &text[currentWordStartPosition], currentWordLength);

			strncpy(words[numWords], currentWord, maxWordLen); //Copy the word to the words array

			//Prepare to start scanning the next word
			numWords++;
			currentWordStartPosition = i;
		} else { //We have not yet reached the end of a word
			i++; //Go to the next character

			if (i >= numChars) { //Last word - this is pretty much the same code as above, so needs to be abstracted to a function to save repetition (later!)
				int currentWordEndPosition = i; //Get the index of the character and store it as current word end position

				//Calculate the length of the current word
				int currentWordLength = currentWordEndPosition - currentWordStartPosition;

				char currentWord[maxWordLen];
				memset(currentWord, 0, maxWordLen);
				strncpy(currentWord, &text[currentWordStartPosition], currentWordLength);

				strncpy(words[numWords], currentWord, maxWordLen);

				numWords++;
			}
		}
	}

	for (i=0; i < numWords; i++) {
		//Copy the current word from the array
		char currentWord[maxWordLen];
		memset(currentWord, 0, maxWordLen);
		strncpy(currentWord, words[i], maxWordLen);

		if (strcmp(currentWord, "***NEWLINE***") == 0) {
			cursory = initialy;
			cursorx -= lineHeight;
			currentline++;

			continue;
		}

		int currentWordWidth = MATextWidthInPixels(currentWord, font); //Calculate the width of the word about to be drawn

		int availableWidthForCurrentWord = maxWidth - (cursory - initialy); //Work out how much space is left on the current line

		if (i < numWords-1) { //If there are more words to draw after this one
			//Copy the next word from the array
			char nextWord[maxWordLen];
			memset(nextWord, 0, maxWordLen);
			strncpy(nextWord, words[i+1], maxWordLen);

			int nextWordWidth = MATextWidthInPixels(nextWord, font); //Calculate the width of the next word

			int availableWidthForNextWord = availableWidthForCurrentWord - currentWordWidth; //Calculate space available for the next word

			if (nextWordWidth <= availableWidthForNextWord) { //If the next word will fit on this line
				//Draw the current word and move the cursor along ready to draw the next word on the next cycle of the loop
				MADrawText(screen, side, cursorx, cursory, currentWord, font, red, green, blue);
				cursory += currentWordWidth;
			} else { //If the next word will not fit on this line
				if (maxLines < 1 || currentline < maxLines) { //If there is no restriction on the number of lines, or if we have not yet reached the last line
					MADrawText(screen, side, cursorx, cursory, currentWord, font, red, green, blue);
					cursory = initialy;
					cursorx -= lineHeight;
					currentline++;
				} else { //If there are no more lines left
					//Create the minimum truncated version of the next word (its first letter plus ellipses)
					char minimumTruncatedNextWord[5];
					memset(minimumTruncatedNextWord, 0, 5);
					strncpy(minimumTruncatedNextWord, nextWord, 1);
					strcat(minimumTruncatedNextWord, "...");

					int minimumTruncatedNextWordWidth = MATextWidthInPixels(minimumTruncatedNextWord, font); //Get the width of the minimum truncated next word

					if (minimumTruncatedNextWordWidth <= availableWidthForNextWord) { //If the minimum truncated next word will fit
						//Draw the current word
						MADrawText(screen, side, cursorx, cursory, currentWord, font, red, green, blue);
						cursory += currentWordWidth;

						truncateTextToFitMaxWidth(nextWord, availableWidthForNextWord, font, false); //Truncate the next word so it will fit on the line with ellipses

						MADrawText(screen, side, cursorx, cursory, nextWord, font, red, green, blue); //Draw the next word

						break; //Break since there's no space for any more words
					} else { //Even the minimum truncated next word won't fit on this line, and there isn't a line available below for it to go on.
						truncateTextToFitMaxWidth(currentWord, availableWidthForCurrentWord, font, true); //Truncate the current word and add ellipses

						MADrawText(screen, side, cursorx, cursory, currentWord, font, red, green, blue); //Draw the truncated current word

						break; //Break, since there's not space for any more words
					}
				}
			}
		} else { //This is the last word
			if (currentWordWidth <= availableWidthForCurrentWord) { //If the last word fits, just draw it and we're done
				MADrawText(screen, side, cursorx, cursory, currentWord, font, red, green, blue);
				/*
				 DONE
				 */
			} else { //The last word doesn't fit on the current line
				if (maxLines < 1 || currentline < maxLines) { //If there is no restriction on the number of lines, or if we still have lines available
					//Move the cursor to the beginning of the next line
					cursory = initialy;
					cursorx -= lineHeight;

					//Truncate the last word if necessary
					if (availableWidthForCurrentWord > maxWidth) {
						truncateTextToFitMaxWidth(currentWord, availableWidthForCurrentWord, font, false);
					}

					//Draw the last word
					MADrawText(screen, side, cursorx, cursory, currentWord, font, red, green, blue);

					/*
					 DONE
					 */
				} else { //There are no more lines available. Truncate the final word and display it
					truncateTextToFitMaxWidth(currentWord, availableWidthForCurrentWord, font, false);
					MADrawText(screen, side, cursorx, cursory, currentWord, font, red, green, blue);

					/*
					 DONE
					 */
				}
			}
		}
	}

	return currentline;
}

int MATextWidthInPixels(char * text, MAFont * font) {
	int size = 0;

	int i;
	int n = strlen(text);

	for (i=0; i<n; i++) {
		char c = text[i];
		int asciiID = c;
		int charIndex = getCharIndexOrErrorCharIndex(font, asciiID);

		if (charIndex > -1) {
			MACharDesc aCharDesc = font->charDescs[charIndex];

			size += aCharDesc.w;
		}
	}

	return size;
}
