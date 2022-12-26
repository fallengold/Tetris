#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <stdint.h>

#define ADD_ONE_ROUND_SCORE(count, score) \
    {                                     \
        switch (count)                    \
        {                                 \
        case 1:                           \
            (*score) += 2;                \
            break;                        \
        case 2:                           \
            (*score) += 3;                \
            break;                        \
        case 3:                           \
            (*score) += 4;                \
            break;                        \
        case 4:                           \
            (*score) += 5;                \
            break;                        \
        default:                          \
            (*score) += 0;                \
        };                                \
    }

int main()
{
    int score = 0;
    ADD_ONE_ROUND_SCORE(4, &score);
    printf("%d", score);
}