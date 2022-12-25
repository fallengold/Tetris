#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <conio.h>
#include <time.h>
#include <math.h>

#define SPACE_BIT 0
#define BLOCK_BIT 1

#define DEFAULT_CENTRE_YCOR 2
#define DEFAULT_CENTRE_INIT_XCOR 7

#define WALL ((uint16_t)0xe007)
#define FLOOR ((uint16_t)0xffff)
#define TETRIS_HEIGHT 27
#define CHUNK_HEIGHT 5
#define BLOCK_NUMBER 7

#define NORMAL_FALL_SPEED 1.5
#define ACE_FALL_SPEED 30

#define WHITE 7
#define GREY 8

/*A self-defined delay*/
#define DELAY_SEC(dly)             \
    const clock_t start = clock(); \
    clock_t current;               \
    do                             \
    {                              \
        current = clock();         \
    } while ((double)(current - start) / CLOCKS_PER_SEC < dly);

uint16_t tetrisFrame[TETRIS_HEIGHT];
uint16_t tetrisEmptyBlock[BLOCK_NUMBER] =
    {0x0000, 0x0000, 0x0000, 0x0000, 0x0000};
uint16_t tetrisBlocks[BLOCK_NUMBER][CHUNK_HEIGHT] =
    {
        {0x0000, 0x0180, 0x0180, 0x0000, 0x0000},
        // ■ ■
        // ■ ■
        {0x0000, 0x0000, 0x03c0, 0x0000, 0x0000},
        //
        // ■ '■' ■ ■
        {0x0000, 0x0100, 0x0380, 0x0000, 0x0000},
        //   ■
        // ■ '■' ■
        {0x0000, 0x0200, 0x0380, 0x0000, 0x0000},
        // ■
        // ■ '■' ■
        {0x0000, 0x0080, 0x0380, 0x0000, 0x0000},
        //       ■
        // ■ '■' ■
        {0x0000, 0x0180, 0x0300, 0x0000, 0x0000},
        //    ■ ■
        // ■ '■'
        {0x0000, 0x0300, 0x0180, 0x0000, 0x0000}
        //   ■  ■
        //     '■' ■
};

const char *messageBox[2] =
    {
        "Press Space R to start a game",
        "You lose!Press R to restart a new game or press Esc to quit"};

int curBlockLevel;
int curBlockCentreXcor;
int curBlockType;
float curFallSpeed = NORMAL_FALL_SPEED;
int isExit;
int isInit;
int isBoot;
int score;
uint16_t curBlock[CHUNK_HEIGHT];

int getBit(int x, int y)
{
    if ((y - curBlockLevel > CHUNK_HEIGHT - 1 || y - curBlockLevel < 0) || isInit)
    {
        if (((tetrisFrame[y] & (uint16_t)1 << (15 - x)) != 0))
        {
            return BLOCK_BIT;
        }
        else
        {
            return SPACE_BIT;
        }
    }
    else
    {
        if ((((tetrisFrame[y] | curBlock[y - curBlockLevel]) & (uint16_t)1 << (15 - x)) != 0 && !isInit))
        // if terrain or block occupy the current position
        {
            return BLOCK_BIT;
        }
        else
        {
            return SPACE_BIT;
        }
    }
}

void printBit(int x, int y)
{
    if (getBit(x, y))
    {
        printf("%s", "\u25A0");
    }
    else
    {
        printf("%s", "\u25A1");
    }
}

void setConsoleStatus(int x, int y, int color)
{
    static COORD coor;
    coor.X = 2 * x;
    coor.Y = y;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coor);
}

void render(int startLevel, int endLevel, int startCol, int endCol, int color)
// render an given rectangle area
{
    for (int y = startLevel; y < endLevel; y++)
    {
        for (int x = startCol; x < endCol; x++)
        {
            setConsoleStatus(x, y, color);
            if (y > 2)
            // render the part below ceiling
            {
                printBit(x, y);
            }
            else
            // render the ceiling
            {
                printf("%s", "\u25A0");
            }
        }
    }
}

void showMessage(int messageIndex)
{
    setConsoleStatus(20, 8, WHITE);
    printf("%s", messageBox[messageIndex]);
}

void showScore()
{
    setConsoleStatus(20, 10, WHITE);
    printf("Score:%d", score);
}

void clearMessage()
{
    setConsoleStatus(20, 8, WHITE);
    printf("                                                           ");
}

int isHit(uint16_t *chunk)
{
    for (int i = 0; i < CHUNK_HEIGHT; i++)
    {
        if ((chunk[i] & tetrisFrame[i + curBlockLevel]) != 0)
        {
            return 1;
        }
    }
    return 0;
}

/*To see if current game is losed*/
int isLose()
{
    return (tetrisFrame[3] != WALL);
}

void setAbsBit(uint16_t *matrix, int xAbsCor, int yAbsCor) // Set the bit based on relative frame with the centre of the current block centre;
{
    matrix[yAbsCor] |= (uint16_t)1 << (15 - xAbsCor);
}

void rotate()
{
    uint16_t tempMatrix[CHUNK_HEIGHT] = {0};
    for (int x = curBlockCentreXcor - 2; x < curBlockCentreXcor + 3; x++)
    {
        for (int y = 0; y < CHUNK_HEIGHT; y++)
        {
            if ((curBlock[y] & (uint16_t)1 << (15 - x)) != 0)
            {
                setAbsBit(tempMatrix, curBlockCentreXcor + (y - DEFAULT_CENTRE_YCOR), DEFAULT_CENTRE_YCOR - (x - curBlockCentreXcor));
            }
        }
    }

    if (isHit(tempMatrix)) // to check if rotating matrix hit the wall of something
    {
        return;
    }
    for (int y = 0; y < CHUNK_HEIGHT; y++)
    {
        curBlock[y] = tempMatrix[y];
    }
}

void eventRenderChunk() // render the chunk
{
    render(curBlockLevel - 1, curBlockLevel + CHUNK_HEIGHT, curBlockCentreXcor - 3, curBlockCentreXcor + 4, WHITE);
}

void keyboardGameHandle() // handle the press event if a key is pressed
{
    char keyPress;
    keyPress = getch();
    if (keyPress == 27)
    {
        isExit = 1;
    }
    else if (keyPress == 'a' || keyPress == 'A')
    {
        for (int i = 0; i < CHUNK_HEIGHT; i++)
        {
            curBlock[i] = curBlock[i] << 1;
        }
        curBlockCentreXcor--;

        if (isHit(curBlock))
        {
            for (int i = 0; i < CHUNK_HEIGHT; i++) // resetBlock the status
            {
                curBlock[i] = curBlock[i] >> 1;
            }
            curBlockCentreXcor++;
        }
    }
    else if (keyPress == 'd' || keyPress == 'D')
    {
        for (int i = 0; i < CHUNK_HEIGHT; i++)
        {
            curBlock[i] = curBlock[i] >> 1;
        }
        curBlockCentreXcor++;

        if (isHit(curBlock))
        {
            for (int i = 0; i < CHUNK_HEIGHT; i++) // resetBlock the status
            {
                curBlock[i] = curBlock[i] << 1;
            }
            curBlockCentreXcor--;
        }
    }
    else if (keyPress == 's' || keyPress == 'S')
    {
        curFallSpeed = ACE_FALL_SPEED;
    }
    else if (keyPress == 'w' || keyPress == 'W')
    {
        if (curBlockType != 0) // no need to rotate the first block
        {
            rotate();
        }
    }
    eventRenderChunk();
}

void eventOneRoundHandle()
{
    int isKeyboardLock = 0;
    int counterFall = 0;
    while (1) // the block falls every frame
    {
        if (curBlockLevel >= TETRIS_HEIGHT - 3 && isExit)
        {
            break;
        }
        if (isHit(curBlock)) // if the block is falling on the terrain
        {
            curBlockLevel--;
            eventRenderChunk();
            break; // upshift the position by one
        }
        if (++counterFall > CLOCKS_PER_SEC / 10 * 1 / curFallSpeed) // the blocks fall
        {
            counterFall = 0;
            curBlockLevel++;
            eventRenderChunk();
        }
        if (curBlockLevel < 3)
        {
            isKeyboardLock = 1;
        }
        else
        {
            isKeyboardLock = 0;
        }
        if (kbhit() && !isKeyboardLock) // detect input
        {
            keyboardGameHandle();
        }
        else
        {
            curFallSpeed = NORMAL_FALL_SPEED; // no 's' is pressed down, resetBlock the speed
        }

        DELAY_SEC(0.01);
    }
    for (int i = 0; i < CHUNK_HEIGHT; i++)
    {
        tetrisFrame[i + curBlockLevel] |= curBlock[i]; // add the pile-up block into the terrain!
        eventRenderChunk();
    }
}

void eventTetrisFrameInit()
{
    for (int i = 0; i < TETRIS_HEIGHT - 3; i++)
    {
        tetrisFrame[i] = WALL;
    }
    for (int i = 1; i < 4; i++)
    {
        tetrisFrame[TETRIS_HEIGHT - i] = FLOOR;
    }
}

int blockSeeds[7];
int blockSeedsNext[7];

void generateSeeds(int array[], int length)
{
    for (int i = 0; i < length; i++)
    {
        array[i] = i;
    }
    /*shuffle the array*/
    srand(time(NULL));
    for (int i = 0; i < length; i++)
    {
        int swapIndex = rand() % length;
        int temp = array[swapIndex];
        array[swapIndex] = array[i];
        array[i] = temp;
    }
}

void eventGenerateRandomSeeds()
{
    if (isInit)
    {
        generateSeeds(blockSeeds, BLOCK_NUMBER);
        generateSeeds(blockSeedsNext, BLOCK_NUMBER);
        return;
    }
    for (int i = 0; i < BLOCK_NUMBER; i++)
    {
        blockSeeds[i] = blockSeedsNext[i];
        generateSeeds(blockSeedsNext, BLOCK_NUMBER);
    }
}

int curBlockCount = 0;
void eventChooseBlock()
{
    if (curBlockCount >= 7) // TODO
    {
        curBlockCount = 0;
        eventGenerateRandomSeeds();
    }

    for (int i = 0; i < CHUNK_HEIGHT; i++)
    {
        curBlockType = blockSeeds[curBlockCount];
        curBlock[i] = tetrisBlocks[curBlockType][i]; // resetBlock current blocks
    }
    curBlockCount++;
}

void eventBlockStatusReset()
{
    curBlockLevel = 0; // resetBlock and init new status of the block
    curBlockCentreXcor = DEFAULT_CENTRE_INIT_XCOR;
    curFallSpeed = NORMAL_FALL_SPEED;
}

void eventGameReset()
{
    isInit = 1;
    SetConsoleTitle("Tetris");
    eventTetrisFrameInit();
    render(0, TETRIS_HEIGHT, 0, 16, WHITE);
    curBlockCentreXcor = DEFAULT_CENTRE_INIT_XCOR;
    curBlockLevel = 0;
    score = 0;
    eventGenerateRandomSeeds(blockSeeds, BLOCK_NUMBER);
    isInit = 0;
}

void eventEliminateBlock()
{
    int eliminateCount = 0;
    for (int i = 3; i < TETRIS_HEIGHT - 4; i++)
    {

        if (tetrisFrame[i + 1] == 0xffff)
        {
            eliminateCount++;
            tetrisFrame[i + 1] = WALL;
            for (int j = i; j > 2; j--)
            {
                uint16_t temp = tetrisFrame[j + 1];
                tetrisFrame[j + 1] = tetrisFrame[j];
                tetrisFrame[j] = temp;
            }
        }
    }
    for (int i = 0; i < 5; i++)
    {
        curBlock[i] = tetrisEmptyBlock[i];
    }
    /*caculate the score in current round*/
    if (eliminateCount > 0)
    {
        score += 100 * pow(2, eliminateCount - 1);
    }
    /*update the whole frame*/
    render(0, TETRIS_HEIGHT, 0, 16, WHITE);
    /*update the score*/
    showScore();
}

void keyboardBootHandle()
{
    char keyPress;
    keyPress = getch();
    if (keyPress == 27)
    {
        isExit = 1;
    }
    else if (keyPress == 'R' || keyPress == 'r')
    {
        isBoot = 0;
    }
}
void eventGameBoot(int statusIndex)
{
    int showFlag = 0;
    isBoot = 1;
    while (1)
    {
        if (!showFlag)
        {
            showMessage(statusIndex);
            showFlag = 1;
        }

        if (!isBoot)
        {
            clearMessage();
            break;
        }
        if (isExit)
        {
            break;
        }
        if (kbhit())
        {
            keyboardBootHandle();
        }
    }
}

int main()
{
    eventGameReset();
    showScore();
    eventGameBoot(0);
    while (1)
    {

        if (isExit)
        {
            break;
        }
        while (1)
        {
            if (isExit)
            {
                break;
            }
            eventChooseBlock();
            eventOneRoundHandle();
            eventEliminateBlock();
            if (isLose())
            {
                eventGameBoot(1);
                eventGameReset();
                break;
            }
            else
            {
                eventBlockStatusReset();
            }
        }
    }
}