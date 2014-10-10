#include <stdio.h>
#include <stdlib.h>
#include <smmintrin.h>

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

			// 
                	r0 = _mm_mul_ps(r0, h3);
                	r1 = _mm_mul_ps(r1, h2);
                	r2 = _mm_mul_ps(r2, h1);
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
		test[i] = 2;
	return test;
}
int main()
{

	Data d = readData();	
	float *x = createTestArray(400);
	float *y = createTestArray(400);
	FIR(x, y, d.h, 400);
   printf("Read String1 |%f|\n", d.angle );
   printf("Read String2 |%f|\n", d.elevation );
   printf("Read side |%c|\n", d.side);
   printf("Read float |%.15f|\n", d.h[0]);
   return(0);
}
