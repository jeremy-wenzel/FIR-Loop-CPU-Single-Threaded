#include <stdio.h>
#include <stdlib.h>
#include <smmintrin.h>
#include <malloc.h>

struct Data{
	float angle;
	float elevation;
	char side;
	__attribute__ ((aligned(16)))float h[200];
};

/* 
	May be used to read in data. Not sure if aligning array correctly
*/
void readData(struct Data *d, int size) {
	FILE* fp = fopen("sub040.data", "r");

	bool valid = false;
	int j = 0;
	while(fscanf(fp, "%f %f %c", &d[j].angle, &d[j].elevation, &d[j].side) != EOF) {
		if(d[j].elevation == 0)
			valid = true;
		for(int i = 0; i < 200; i++) {
			fscanf(fp, "%f", &d[j].h[i]);
			if(valid == false)
				d[j].h[i] = 0;
		}
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

	for(int i = 0; i < size; i++)
		output[i] = (float) input[i];

	return output;

}

/**
	This function takes in an float array of int size, makes a 
	int16_t array of int size, and then converts the input from 
	int16_t to float
*/
void floatToInt(float* input, int16_t* output, int size) {


	for(int i = 0; i < size; i++) {
		if(input[i] > 32767.0)
			input[i] = 32767.0;
		else if(input[i] < -32768.0)
			input[i] = -32768.0;
		output[i] = (int16_t) input[i];
	}
}

/*
	This function takes in a float x array, float y array,
	float h array, and int size
*/
void FIR(float* x, float* y, float* h, int size) {
	__m128 sum = _mm_set1_ps(0x0000);
	// Set first 200 values of y to 0
	for(int i = 0; i < 200; i+=4) 
		_mm_store_ps(&y[i], sum);

	// Begin vector stuff
	for(int i = 200; i < size; i+=4) {
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
           	__m128 ld2 = _mm_load_ps(&x[i-j+3]);	// 200 - 199 + 3 = 200 -196 = 4

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


}

/* The actual Demo Driver */
void do_HRTF_Demo(Data *d, int size) {

	FILE* outFileLeft = fopen("output-80-left.pcm", "w");
	if(outFileLeft == 0) {
		fprintf(stderr, "Could not open left output file\n");
		exit(-1);
	}
	
	FILE* outFileRight = fopen("output-80-right.pcm", "w");
	if(outFileRight == 0) {
		fprintf(stderr, "Could not open right output file\n");
		exit(-1);
	}
	
	FILE* outFileCombined = fopen("output-80-combined.pcm", "w");
	if(outFileCombined == 0) {
		fprintf(stderr, "Could not open combined output file\n");
		exit(-1);
	}
	/* HRTF Demo Begin */
	FILE *inputFile = fopen("raw.pcm", "r");
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
	printf("Reading from input file\n");
	int read  = fread(input, sizeof(int16_t), fileSize, inputFile);
	// Check that the amount read and the fileSize are the same
	if(read != fileSize) {	
		fprintf(stderr, "Read is not the same as fileSize");
		exit(-1);
	}
	printf("Finished reading from input file\n");
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

	//TODO: actually run with x, h ,y, and fileSize
	
	// Get left side
	printf("Starting FIR\n");
	FIR(x_input, yLeft, d[0].h, fileSize);	
		
	// Get the right side
	FIR(x_input, yRight, d[1].h, fileSize);
	printf("Ending FIR\n");

	free(x_input);
	free(input);
	//TODO: turn y array into int16_t array
	printf("converting\n");
	int16_t *y_left_output = new int16_t[fileSize];
	floatToInt(yLeft, y_left_output, fileSize);
	printf("ending converting\n");
	int16_t *y_right_output = new int16_t[fileSize];
	floatToInt(yRight, y_right_output, fileSize);
	int16_t *y_combined = new int16_t[fileSize*2];

	printf("Total samples = %i\n", fileSize*2);

	combine(y_left_output, y_right_output, y_combined, fileSize);
	// Put y array into file
	fwrite(y_left_output, sizeof(int16_t), fileSize, outFileLeft);
	fwrite(y_right_output, sizeof(int16_t), fileSize, outFileRight);
	fwrite(y_combined, sizeof(int16_t), fileSize*2, outFileCombined);

	// Delete Arrays; May screw things up
	delete[] y_combined;
	delete[] y_right_output;
	delete[] y_left_output;
}

int main()
{

	struct Data d[100];
	printf("Reading from .data file\n");
	readData(d, 100);		
	printf("Finished reading from .data file\n");
	do_HRTF_Demo(d, 1000);
	for(int i = 0; i < 100; i++){
		printf("%i: Angle = %f  elevation = %f\n", i, d[i].angle, d[i].elevation);
	}
	
    return(0);
}