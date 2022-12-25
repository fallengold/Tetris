#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <stdint.h>


uint16_t testMatrix[4]=
{
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    
};

void setRelBit(uint16_t *matrix, int xAbsCor, int yAbsCor) // Set the bit based on relative frame with the centre of the current block centre;
{
    matrix[yAbsCor] |= (uint16_t)1<<(15 - xAbsCor);
}

int main()
{
    setRelBit(testMatrix, 2, 2);
    for(int i = 0; i < 4; i++)
    {
        printf("%d\n", testMatrix[i]);
    }
}
