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
        "Game Over! Press R to restart a new game or press Esc to quit"};

struct
{
    int level;
    int Xcor;
    int type;
    float speed;
} blockStatus;
uint16_t curBlock[CHUNK_HEIGHT];

enum
{
    INIT = 1,
    BOOT = 2,
    RESET = 3,
    GAME = 3,
    EXIT = 4,
} gameStatus;

int isExit;
int isInit;
int isBoot;
int score;

int getBit(int x, int y)
{
    /* If the bit is outside chunk, only need to consider single frame*/
    if ((y - blockStatus.level > CHUNK_HEIGHT - 1 || y - blockStatus.level < 0))
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
        /*If the bit is inside the chunk, consider the intersection of the frame and the chunk*/
        if ((((tetrisFrame[y] | curBlock[y - blockStatus.level]) & (uint16_t)1 << (15 - x)) != 0))
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
        /*this is the character for an solid bit*/
        printf("%s", "\u25A0");
    }
    else
    {
        /*this is the character for an empty bit*/
        printf("%s", "\u25A1");
    }
}

/*Move the printing console to the target position*/
void setConsoleStatus(int x, int y, int color)
{
    static COORD coor;
    coor.X = 2 * x;
    coor.Y = y;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coor);
}

/*Render a rectangle bits area given the position at four corners in given color*/
void render(int startLevel, int endLevel, int startCol, int endCol, int color)

{
    for (int y = startLevel; y < endLevel; y++)
    {
        for (int x = startCol; x < endCol; x++)
        {
            setConsoleStatus(x, y, color);
            if (y > 2)
            /*Render the part below ceiling*/
            {
                printBit(x, y);
            }
            else
            /*render the ceiling*/
            {
                printf("%s", "\u25A0");
            }
        }
    }
}
/*Render and update the chunk*/
void eventRenderChunk()
{
    render(blockStatus.level - 1, blockStatus.level + CHUNK_HEIGHT, blockStatus.Xcor - 3, blockStatus.Xcor + 4, WHITE);
}
/*Reset and render the whole frame*/
void eventResetRender()
{
    render(0, TETRIS_HEIGHT, 0, 16, WHITE);
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

void clearScore()
{
    setConsoleStatus(20, 10, WHITE);
    printf("                    ");
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
        if ((chunk[i] & tetrisFrame[i + blockStatus.level]) != 0)
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

void chunkRotate()
{
    uint16_t tempMatrix[CHUNK_HEIGHT] = {0};
    for (int x = blockStatus.Xcor - 2; x < blockStatus.Xcor + 3; x++)
    {
        for (int y = 0; y < CHUNK_HEIGHT; y++)
        {
            if ((curBlock[y] & (uint16_t)1 << (15 - x)) != 0)
            {
                setAbsBit(tempMatrix, blockStatus.Xcor + (y - DEFAULT_CENTRE_YCOR), DEFAULT_CENTRE_YCOR - (x - blockStatus.Xcor));
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

void keyboardGameHandle() // handle the press event if a key is pressed
{
    char keyPress;
    keyPress = getch();
    if (keyPress == 27)
    {
        gameStatus = EXIT;
    }
    else if (keyPress == 'a' || keyPress == 'A')
    {
        for (int i = 0; i < CHUNK_HEIGHT; i++)
        {
            curBlock[i] = curBlock[i] << 1;
        }
        blockStatus.Xcor--;

        if (isHit(curBlock))
        {
            for (int i = 0; i < CHUNK_HEIGHT; i++) // resetBlock the status
            {
                curBlock[i] = curBlock[i] >> 1;
            }
            blockStatus.Xcor++;
        }
    }
    else if (keyPress == 'd' || keyPress == 'D')
    {
        for (int i = 0; i < CHUNK_HEIGHT; i++)
        {
            curBlock[i] = curBlock[i] >> 1;
        }
        blockStatus.Xcor++;

        if (isHit(curBlock))
        {
            for (int i = 0; i < CHUNK_HEIGHT; i++) // resetBlock the status
            {
                curBlock[i] = curBlock[i] << 1;
            }
            blockStatus.Xcor--;
        }
    }
    else if (keyPress == 's' || keyPress == 'S')
    {
        blockStatus.speed = ACE_FALL_SPEED;
    }
    else if (keyPress == 'w' || keyPress == 'W')
    {
        if (blockStatus.type != 0) // no need to chunkRotate the first block
        {
            chunkRotate();
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
        if (blockStatus.level >= TETRIS_HEIGHT - 3)
        {
            break;
        }
        if (gameStatus == EXIT)
        {
            break;
        }
        if (isHit(curBlock)) // if the block is falling on the terrain
        {
            blockStatus.level--;
            eventRenderChunk();
            break; // upshift the position by one
        }
        if (++counterFall > CLOCKS_PER_SEC / (10 * blockStatus.speed)) // the blocks fall
        {
            counterFall = 0;
            blockStatus.level++;
            eventRenderChunk();
        }
        if (blockStatus.level < 3)
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
            blockStatus.speed = NORMAL_FALL_SPEED; // no 's' is pressed down, resetBlock the speed
        }

        DELAY_SEC(0.01);
    }
    for (int i = 0; i < CHUNK_HEIGHT; i++)
    {
        tetrisFrame[i + blockStatus.level] |= curBlock[i]; // add the pile-up block into the terrain!
        eventRenderChunk();
    }
}

void eventTetrisFrameReset()
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
    if (gameStatus == INIT)
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
        blockStatus.type = blockSeeds[curBlockCount];
        curBlock[i] = tetrisBlocks[blockStatus.type][i]; // resetBlock current blocks
    }
    curBlockCount++;
}

void eventBlockStatusReset()
{
    blockStatus.level = 0; // resetBlock and init new status of the block
    blockStatus.Xcor = DEFAULT_CENTRE_INIT_XCOR;
    blockStatus.speed = NORMAL_FALL_SPEED;
}

void eventGameReset()
{
    eventTetrisFrameReset();
    eventBlockStatusReset();
    eventResetRender();
    eventGenerateRandomSeeds(blockSeeds, BLOCK_NUMBER);
    score = 0;
}

void eventEliminateBlock()
{
    uint16_t tetrisEmptyBlock[BLOCK_NUMBER] = {0};
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
        gameStatus = EXIT;
    }
    else if (keyPress == 'R' || keyPress == 'r')
    {
        gameStatus = GAME;
    }
}
void eventGameBoot(int statusIndex)
{
    int showFlag = 0;
    while (1)
    {
        if (!showFlag)
        {
            showMessage(statusIndex);
            showFlag = 1;
        }

        if (gameStatus == GAME)
        {
            clearMessage();
            break;
        }
        if (gameStatus == EXIT)
        {
            break;
        }
        if (kbhit())
        {
            keyboardBootHandle();
        }
    }
}

void eventGameInit()
{
    gameStatus = INIT;
    SetConsoleTitle("Tetris");
    eventTetrisFrameReset();
    eventBlockStatusReset();
    eventResetRender();
    eventGenerateRandomSeeds(blockSeeds, BLOCK_NUMBER);

    score = 0;
    gameStatus = BOOT;
}

int main()
{
    eventGameInit();
    showScore();
    eventGameBoot(0);
    while (1)
    {

        if (gameStatus == EXIT)
        {
            break;
        }
        while (1)
        {
            if (gameStatus == EXIT)
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
                clearScore();
                showScore();
                break;
            }
            else
            {
                eventBlockStatusReset();
            }
        }
    }
}