#include <stdio.h>

int countOnes(int* matrix, int width, int height) {

    int count = 0;
    for (int i = 0; i < width * height; i++) {
        if (matrix[i] == 1) {
            count++;
        }
    }
    return count;
    
}