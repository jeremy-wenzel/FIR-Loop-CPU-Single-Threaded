#include <stdio.h>
#include <stdlib.h>
#include <smmintrin.h>
#include <time.h>
#include <malloc.h>

struct Data{
	float angle;
	float elevation;
	char side;
	__attribute__ ((aligned(16)))float h[200];
};

/* 
	Used to read data in from the .data file
*/
void readData(struct Data *d, int size) {
	FILE* fp = fopen("sub040.data", "r");

	bool valid = false;
	int j = 0;
	// go through file
	while(fscanf(fp, "%f %f %c", &d[j].angle, &d[j].elevation, &d[j].side) != EOF) {
		// I am only picking coordinates at elevation 0.
		if(d[j].elevation == 0)
			valid = true;
		// Grab the 200 floats
		for(int i = 0; i < 200; i++) {
			fscanf(fp, "%f", &d[j].h[i]);
			if(valid == false)
				d[j].h[i] = 0;
		}
		// Only go to next array when we have an elevation of 0
		if(valid == true)
			j++;
		valid = false;
	}
	fclose(fp);
}

/**
	This function takes in an int16_t array of int size, makes a 
	float array of int size, aligned to 16 bytes, and then converts
	the input from int16_t to float
*/
float* intToFloat(int16_t *input, int size) {
	float* output = (float*) memalign(16, size*sizeof(float));
	if(output == NULL) {
		printf("Could not allocate memory at intToFloat");
		exit (-1);
	}

	for(int i = 0; i < size; i++)
		output[i] = (float) input[i];

	return output;

}

/**
	This function takes in an float array of int size, makes a 
	int16_t array of int size, and then converts the input from 
	int16_t to float. This method also makes sure that there is 
	no clipping when converting from float to int
*/
void floatToInt(float* input, int16_t* output, int size) {

	for(int i = 0; i < size; i++) {
		// In case of overflow
		if(input[i] > 32767.0)
			input[i] = 32767.0;
		// In case of underflow
		else if(input[i] < -32768.0)
			input[i] = -32768.0;
		output[i] = (int16_t) input[i];
	}
}

void floatAddFloat(float* output, float* input1, float* input2, int size) {

	for(int i = 0; i < size; i++) {
		output[i] = input1[i] + input2[i];
		output[i] = output[i] / 2;
	}
}
/*
	This function takes in a float x array, float y array,
	float h array, and int size
*/
void FIR(float* x, float* y, float* h, int start, int size) {
	__m128 sum = _mm_set1_ps(0x0000);
	// Set first 200 values of y to 0
	

	// Begin vector stuff
	for(int i = start; i < size; i+=4) {
		sum = _mm_set1_ps(0x0000);	// make a sum variable
		for(int j = 199; j > 0; j-= 4) {
			//TODO: Get on white board to make sure this is right
			// Get h values
			__m128 h3 = _mm_set1_ps(h[j]);
			__m128 h2 = _mm_set1_ps(h[j-1]);
			__m128 h1 = _mm_set1_ps(h[j-2]);
			__m128 h0 = _mm_set1_ps(h[j-3]);

			// example going down i = 200 and j = 199
			// so ld1 would be = x[0] x[1] x[2] x[3]
			__m128 ld1 = _mm_load_ps(&x[i-j-1]);	// 200 - 199 - 1= 200 - 200= 0

			// follow example from above
			// ld2 would be = x[4] x[5] x[6] x[7]
           	__m128 ld2 = _mm_load_ps(&x[i-j +3]);	// 200 - 199 + 3 = 200 -196 = 4

			// r1 = x[2] x[3] x[4] x[5]
            __m128 r1 = _mm_shuffle_ps(ld1,ld2, 0x4e);

			// r0 = x[1] x[2] x[3] x[4]
	        __m128 r0 = _mm_shuffle_ps(ld1, r1, 0x99);
		
			// r2 = x[3] x[4] x[5] x[6]
            __m128 r2 = _mm_shuffle_ps(r1, ld2, 0x99);

			// r3 = x[4] x[5] x[6] x[7]
            __m128 r3 = ld2;

			// r0 = (x[1] x[2] x[3] x[4]) * h[199]
            r0 = _mm_mul_ps(r0, h3);

			// r1 = (x[2] x[3] x[4] x[5]) * h[198]
            r1 = _mm_mul_ps(r1, h2);

			// r2 = (x[3] x[4] x[5] x[6]) * h[197]
	        r2 = _mm_mul_ps(r2, h1);

			// r3 = (x[4] x[5] x[6] x[7]) * h[196]
            r3 = _mm_mul_ps(r3, h0);

			sum = _mm_add_ps(sum, r0);
			sum = _mm_add_ps(sum, r1);
	        sum = _mm_add_ps(sum, r2);
            sum = _mm_add_ps(sum, r3);

		}
		// Store the sum
		_mm_store_ps(&y[i], sum);
	}
}

/**
 	Creats a Test array of given size. Meant for debugging
*/
float* createTestArray(int size) {
	float* test = new float[size];
	for(int i = 0; i < size; i++)
		test[i] = 1;
	return test;
}

void combine(int16_t *left, int16_t *right, int16_t *combined, int fileSize) {

	for(int i = 0; i < fileSize; i++) {
		combined[i*2] = left[i];
		combined[i*2 + 1] = right[i];
	}
}

void steroToHeadphones(Data *d, int size) {

	struct Data LP;
	struct Data LN;
	struct Data RP;
	struct Data RN;

	for(int i = 0; i < size; i++) {
		if(d[i].angle == -30 && d[i].side == 'L')
			LN = d[i];
		if(d[i].angle == 30 && d[i].side == 'L')
			LP = d[i];
		if(d[i].angle == -30 && d[i].side == 'R')
			RN = d[i];
		if(d[i].angle == 30 && d[i].side == 'R')
			RP = d[i];
	}

	// open right file
	FILE *rightFile = fopen("01.R.pcm", "r");
	if(rightFile == NULL) {
		fprintf(stderr, "Could not open the right input file (steroToHeadphones)\n");
		exit(-1);
	}

	fseek(rightFile, 0, SEEK_END);			// Go to the end of file
	int fileSize = ftell(rightFile);		// Get the file size
	rewind(rightFile);						// Rewind to beginning of file
	fileSize = fileSize / sizeof(int16_t);	// Get the number of int16_t numbers
	
	// open left file
	FILE *leftFile = fopen("01.L.pcm", "r");
	if(leftFile == NULL) {
		fprintf(stderr, "Could not open the left input file (steroToHeadphones)\n");
		exit(-1);
	}

	// put right side into int16_t array
	int16_t *right_array = new int16_t[fileSize];
	int read  = fread(right_array, sizeof(int16_t), fileSize, rightFile);
	if(read != fileSize) {	
		fprintf(stderr, "Read is not the same as fileSize in right array (steroToHeadphones)\n");
		exit(-1);
	}
	// put left side into int16_t array
	int16_t *left_array = new int16_t[fileSize];
	read  = fread(left_array, sizeof(int16_t), fileSize, leftFile);
	if(read != fileSize) {	
		fprintf(stderr, "Read is not the same as fileSize in left array (steroToHeadphones)\n");
		exit(-1);
	}

	// Convert right array to float array and then free the right array
	float *rightInput = intToFloat(right_array, fileSize);
	free(right_array);
	// Convert left array to float array and then free the left array
	float *leftInput = intToFloat(left_array, fileSize);
	free(left_array);


	//Lout = (FIR(LIN, L-) + FIR(RIN, L+)) / 2
	float* yLeftNeg = (float*) memalign(16, sizeof(float)*fileSize);
	float* yLeftPos = (float*) memalign(16, sizeof(float)*fileSize);
	FIR(leftInput, yLeftNeg, LN.h, 200, fileSize-4);
	FIR(rightInput, yLeftPos, LP.h, 200, fileSize-4);


	// Add LeftNeg and LeftPos to LeftTotal
	float *yLeftTotal = (float*) memalign(16, sizeof(float)*fileSize);
	floatAddFloat(yLeftTotal, yLeftNeg, yLeftPos, fileSize);


	//Rout = (FIR(LIN, R-) + FIR(RIN, R+)) / 2
	float* yRightNeg = (float*) memalign(16, sizeof(float)*fileSize);
	float* yRightPos = (float*) memalign(16, sizeof(float)*fileSize);
	FIR(leftInput, yRightNeg, RN.h, 200, fileSize-4);
	FIR(rightInput, yRightPos, RP.h, 200, fileSize-4);

	// Add the RightNeg and RightPos to RightTotal
	float* yRightTotal = (float*) memalign(16, sizeof(float)*fileSize);
	floatAddFloat(yRightTotal, yRightNeg, yRightPos, fileSize);


	// Free some stuff
	free(yLeftNeg);
	free(yLeftPos);
	free(yRightNeg);
	free(yRightPos);

	// put right total into int16_t array
	int16_t *rightOut = new int16_t[fileSize];
	floatToInt(yRightTotal, rightOut, fileSize);
	

	// put left total into int16_t array
	int16_t *leftOut = new int16_t[fileSize];
	floatToInt(yLeftTotal, leftOut, fileSize);
	

	// put leftOut and rightOut into one file
	FILE* outFile = fopen("steroToHeadphones.pcm", "w");
	int16_t *total = new int16_t[fileSize*2];
	combine(leftOut, rightOut, total, fileSize);
	fwrite(total, sizeof(int16_t), fileSize*2, outFile);

	// Free and close the rest of the data
	free(total);
	free(rightOut);
	free(leftOut);
	fclose(rightFile);
	fclose(leftFile);
	fclose(outFile);

}

/* The actual Demo Driver */
void do_HRTF_Demo(Data *d, int size) {

	FILE* outFileCombined = fopen("output-80-combined.pcm", "w");
	if(outFileCombined == 0) {
		fprintf(stderr, "Could not open combined output file\n");
		exit(-1);
	}
	/* HRTF Demo Begin */
	FILE *inputFile = fopen("01.R.pcm", "r");
	if(inputFile == NULL) {
		fprintf(stderr, "Could not open the input file\n");
		exit(-1);
	}

	fseek(inputFile, 0, SEEK_END);		// Go to the end of file
	int fileSize = ftell(inputFile);		// Get the file size
	rewind(inputFile);						// Rewind to beginning of file
	fileSize = fileSize / sizeof(int16_t);	// Get the number of int16_t numbers
	
	// Create array of number of int16_t for file
	int16_t *input = new int16_t[fileSize];	

	// Begin to read the file
	int read  = fread(input, sizeof(int16_t), fileSize, inputFile);
	// Check that the amount read and the fileSize are the same
	if(read != fileSize) {	
		fprintf(stderr, "Read is not the same as fileSize\n");
		exit(-1);
	}
	// Put input into float x array
	float* x_input = intToFloat(input, fileSize);

	// Create float array y, aligned to 16
	float* yLeft = (float*) memalign(16, sizeof(float)*fileSize);
	if(yLeft == NULL) {
		fprintf(stderr, "Y Left input memory failed to allocate\n");
		exit (-1);
	}
	
	float* yRight = (float*) memalign(16, sizeof(float)*fileSize);
	if(yRight == NULL) {
		fprintf(stderr, "Y Left input memory failed to allocate\n");
		exit(-1);
	}

	
	// Get left side
	printf("Starting FIR\n");
	int i = 200;
	// Start timing here
	while(i < fileSize) {

		int num = rand() % 24;
		num = num * 2;
		int end = i + 220500;
		if(end > fileSize)
			end = fileSize-4;
		FIR(x_input, yLeft, d[num].h, i, end);	
			
		// Get the right side
		FIR(x_input, yRight, d[num+1].h, i, end);

		i+=220500;
	}
	// Ending timing here

	printf("Ending FIR\n");

	delete[] x_input;
	free(input);

	// Turn left array into int16_t array
	int16_t *y_left_output = new int16_t[fileSize];
	floatToInt(yLeft, y_left_output, fileSize);


	// Turn right array into int16_t array
	int16_t *y_right_output = new int16_t[fileSize];
	floatToInt(yRight, y_right_output, fileSize);
	
	int16_t *y_combined = new int16_t[fileSize*2];

	combine(y_left_output, y_right_output, y_combined, fileSize);
	// Put y array into file
	fwrite(y_combined, sizeof(int16_t), fileSize*2, outFileCombined);

	// Delete Arrays; May screw things up
	delete[] y_combined;
	delete[] y_right_output;
	delete[] y_left_output;
	fclose(outFileCombined);
}

int main()
{

	struct Data d[100];
	readData(d, 100);
	printf("Beginning HRTF Demo\n");		
	do_HRTF_Demo(d, 60);
	printf("Done. Demo is placed in file \"output-80-combined.pcm\"\n");
	
	printf("Beginning Stero to Headphones Conversion\n");
	steroToHeadphones(d, 60);
	printf("Done. Output is placed in file \"steroToHeadphones.pcm\"\n");
    return(0);
}
