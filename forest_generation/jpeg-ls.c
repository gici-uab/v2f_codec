//gcc -fPIC -shared -o jpeg-ls.so jpeg-ls.c
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int * jpeg_ls(int * img, int x, int y, int z){
    int A,B,C;
    int *predictions;

     if((predictions = (int *) malloc(sizeof(int)*x*y*z)) == NULL)
    {
        fprintf(stderr, "\nFailed to assign memory for predictions array\n\n");
        exit(1);
    }

    for(int k=0; k<z; k++){
        for(int j=0; j<y; j++){
            for(int i=0; i<x; i++){
                A = B = C = 0;
                if(i>=1){
                    A = img[i-1 + j*x + k*x*y];
                    if(j>=1) C = img[i-1 + (j-1)*x + k*x*y];
                }
                if(j>=1) B = img[i + (j-1)*x + k*x*y];

                if(C >= MAX(A,B)) predictions[i + j*x + k*x*y] = MIN(A,B);
                else if(C <= MIN(A,B)) predictions[i + j*x + k*x*y] = MAX(A,B);
                else predictions[i + j*x + k*x*y] = A + B - C;

            }
        }
    }

    return predictions;
}