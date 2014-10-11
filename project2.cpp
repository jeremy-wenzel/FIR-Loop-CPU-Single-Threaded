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
Data readData() {
	struct Data d;
	FILE* fp = fopen("sub003.data", "r");

	fscanf(fp, "%f %f %c", &d.angle, &d.elevation, &d.side);

	for(int i = 0; i < 200; i++)
		fscanf(fp, "%f", &d.h[i]);
	fclose(fp);
	return d;
}

/**
	This function takes in an int16_t array of int size, makes a 
	float array of int size, aligned to 16 bytes, and then converts
	the input from int16_t to float
*/
float* intToTloat(int16_t *input, int size) {
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
int16_t* floatToInt(float* input, int size) {
	int16_t* output = (int16_t*) (size*sizeof(int16_t));

	for(int i = 0; i < size; i++) {
		if(input[i] > 32767.0)
			input[i] = 32767.0;
		else if(input[i] < -32768.0)
			input[i] = -32768.0;
		output[i] = (int16_t) input[i];
	}
	return output;
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

/*
	This fucntion reads the data from the input stream and puts it into
	an array
*/

void do_HRTF_Demo() {

	FILE *inputFile = fopen("something.txt", "r");
	if(inputFile == NULL) {
		fprintf(stderr, "Could not open the input file\n");
		return (-1);
	}

}

int main()
{

	Data d = readData();	

	/* Test Stuff */
	float *x = createTestArray(400);
	float *y = createTestArray(400);
	float *h = createTestArray(200);
	FIR(x, y, h, 400);
	
	for(int i = 0; i < 400; i++)
		printf("y[%i] = %f\n", i, y[i]);

	/* HRTF Demo Begin */
	FILE *inputFile = fopen("something.txt", "r");
	if(inputFile == NULL) {
		fprintf(stderr, "Could not open the input file\n");
		return (-1);
	}

	fseek = (inputFile, 0, SEEK_END);		// Go to the end of file
	rewind(inputFile);						// Rewind to beginning of file
	int fileSize = ftell(inputFile);		// Get the file size
	fileSize = fileSize / sizeof(int16_t);	// Get the number of int16_t numbers
	int16_t *input = new int16_t[fileSize];	// Create array of number of int16_t

	// Begin to read the file
	int read  = fread(input, sizeof(int16_t), fileSize, inputFile);

	//TODO: put input into float x array

	//TODO: put data into float h array

	//TODO: create array y, aligned to 16

	//TODO: actually run with x, h ,y, and fileSize

	//TODO: turn y array into int16_t array

	//TODO: put y array into file

    return(0);
}
