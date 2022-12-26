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

/*Define numeric mimic parameters for the scene*/
#define WALL ((uint16_t)0xe007)
#define FLOOR ((uint16_t)0xffff)
#define TETRIS_HEIGHT 27
#define CHUNK_HEIGHT 5

/*Define the block numbers*/
#define BLOCK_NUMBER 7

/*Define fall gravity in level 1*/
#define BASE_NORMAL_GRAVITY ((double)1 / (double)64)
#define BASE_ACE_GRAVITY ((double)1)

/*Define color parameters*/
#define WHITE 7
#define GREY 8

#define FPS (double)60

/*Define parametre for score count mechanism */
#define BASIC_BLOCK_LAND_SCORE 4
#define BASIC_ONE_LINE_ELIMINATE_SCORE 40
#define BASIC_TWO_LINE_ELIMINATE_SCORE 100
#define BASIC_THREE_LINE_ELIMINATE_SCORE 300
#define BASIC_FOUR_LINE_ELIMINATE_SCORE 1200
#define LINES_PER_LEVEL 10
#define MAX_LEVEL 30
/*A self-defined delay counter*/
#define DELAY_SEC(dly)                                                \
    {                                                                 \
        const clock_t start = clock();                                \
        clock_t current;                                              \
        do                                                            \
        {                                                             \
            current = clock();                                        \
        } while (((double)(current - start)) / CLOCKS_PER_SEC < dly); \
    }

#define ADD_ONE_ROUND_SCORE(count, pScore, level)                  \
    {                                                              \
        switch (count)                                             \
        {                                                          \
        case 1:                                                    \
            (*pScore) += BASIC_ONE_LINE_ELIMINATE_SCORE * level;   \
            break;                                                 \
        case 2:                                                    \
            (*pScore) += BASIC_TWO_LINE_ELIMINATE_SCORE * level;   \
            break;                                                 \
        case 3:                                                    \
            (*pScore) += BASIC_THREE_LINE_ELIMINATE_SCORE * level; \
            break;                                                 \
        case 4:                                                    \
            (*pScore) += BASIC_FOUR_LINE_ELIMINATE_SCORE * level;  \
            break;                                                 \
        default:                                                   \
            (*pScore) += 0;                                        \
        };                                                         \
    }

#define GET_LEVEL(totalEliminateCount, pLevel)                            \
    {                                                                     \
        if (totalEliminateCount < LINES_PER_LEVEL * MAX_LEVEL)            \
        {                                                                 \
            (*pLevel) = (int)(totalEliminateCount / LINES_PER_LEVEL) + 1; \
        }                                                                 \
        else                                                              \
        {                                                                 \
            (*pLevel) = 20;                                               \
        }                                                                 \
    }

#define GET_GRAVITY_MODIFIER(level) ((level - 1) * 0.25 + 1)

#define KEYBOARD_BUFSIZ 5
char inputBuf[KEYBOARD_BUFSIZ];

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

uint16_t curBlock[CHUNK_HEIGHT];

const char *messageBox[4] =
    {
        "Press Space R to start a game",
        "Game Over!",
        "Press R to restart a new game",
        "Press Esc to quit"};

struct
{
    int Ycor;
    int Xcor;
    int type;
    int count;
    float gravity;
} blockStatus;

struct
{
    int score;
    int eliminateCount;
    int level;
} gameData;

enum
{
    INIT = 1,
    BOOT = 2,
    RESET = 3,
    GAME = 3,
    EXIT = 4,
} gameStatus;

int getBit(int x, int y)
{
    /* If the bit is outside chunk, only need to consider single frame*/
    if ((y - blockStatus.Ycor > CHUNK_HEIGHT - 1 || y - blockStatus.Ycor < 0))
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
        if ((((tetrisFrame[y] | curBlock[y - blockStatus.Ycor]) & (uint16_t)1 << (15 - x)) != 0))
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
void render(int startYcor, int endYcor, int startCol, int endCol, int color)

{
    for (int y = startYcor; y < endYcor; y++)
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
    /*Set the pointer to the COORD (0, 0) in case of mixture of rendering*/
    setConsoleStatus(0, 0, WHITE);
}
/*Render and update the chunk*/
void eventRenderChunk()
{
    render(blockStatus.Ycor - 1, blockStatus.Ycor + CHUNK_HEIGHT, blockStatus.Xcor - 3, blockStatus.Xcor + 4, WHITE);
}
/*Reset and render the whole frame*/
void eventRenderAll()
{
    render(0, TETRIS_HEIGHT, 0, 16, WHITE);
}

void showMessage(int x, int y, int messageIndex)
{
    setConsoleStatus(x, y, WHITE);
    printf("%s", messageBox[messageIndex]);
    setConsoleStatus(0, 0, WHITE);
}

void showData()
{
    setConsoleStatus(20, 14, WHITE);
    printf("Score:%d", gameData.score);
    setConsoleStatus(20, 16, WHITE);
    printf("Level:%d", gameData.level);
    setConsoleStatus(0, 0, WHITE);
}

void showNextBlock()
{
    setConsoleStatus(20, 2, WHITE);
    printf("Next Block:");
    setConsoleStatus(20, 4, WHITE);
    setConsoleStatus(0, 0, WHITE);
}

void clearNextBlock()
{
    setConsoleStatus(20, 4, WHITE)
}
void clearData()
{
    setConsoleStatus(20, 14, WHITE);
    printf("                    ");
    setConsoleStatus(20, 16, WHITE);
    printf("                    ");
    setConsoleStatus(0, 0, WHITE);
}

void clearMessage()
{
    setConsoleStatus(20, 8, WHITE);
    printf("                                                               ");
    setConsoleStatus(20, 10, WHITE);
    printf("                                                               ");
    setConsoleStatus(20, 12, WHITE);
    printf("                                                               ");
    setConsoleStatus(0, 0, WHITE);
}

int isHit(uint16_t *chunk)
{
    for (int i = 0; i < CHUNK_HEIGHT; i++)
    {
        if ((chunk[i] & tetrisFrame[i + blockStatus.Ycor]) != 0)
        {
            return 1;
        }
    }
    return 0;
}

int isLose()
{
    return (tetrisFrame[3] != WALL);
}
/*Set the bit based on relative frame with the centre of the current block centre;*/
void setAbsBit(uint16_t *matrix, int xAbsCor, int yAbsCor)
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
        return;
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
        blockStatus.gravity = BASE_ACE_GRAVITY;
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
    int counterFall = 0;
    DELAY_SEC(1);
    while (1) // the block falls every frame
    {
        DELAY_SEC(1 / FPS);
        if (blockStatus.Ycor >= TETRIS_HEIGHT - 3)
        {
            break;
        }
        if (gameStatus == EXIT)
        {
            break;
        }

        if (++counterFall > 1 / blockStatus.gravity) // the blocks fall
        {
            counterFall = 0;
            blockStatus.Ycor++;
            eventRenderChunk();
        }
        if (isHit(curBlock)) // if the block is falling on the terrain
        {
            blockStatus.Ycor--;
            break; // upshift the position by one
        }
        if (kbhit()) // detect input
        {
            keyboardGameHandle();
        }
        else
        {
            blockStatus.gravity = BASE_NORMAL_GRAVITY * GET_GRAVITY_MODIFIER(gameData.level); // no 's' is pressed down, resetBlock the gravity
        }
    }
    for (int i = 0; i < CHUNK_HEIGHT; i++)
    {
        tetrisFrame[i + blockStatus.Ycor] |= curBlock[i]; // add the pile-up block into the terrain!
    }
    eventRenderChunk();
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

void eventChooseBlock()
{
    if (blockStatus.count >= 7)
    {
        blockStatus.count = 0;
        eventGenerateRandomSeeds();
    }

    for (int i = 0; i < CHUNK_HEIGHT; i++)
    {
        blockStatus.type = blockSeeds[blockStatus.count];
        curBlock[i] = tetrisBlocks[blockStatus.type][i]; // resetBlock current blocks
    }
    blockStatus.count++;
}

void blockStatusReset()
{
    blockStatus.Ycor = 0; // resetBlock and init new status of the block
    blockStatus.Xcor = DEFAULT_CENTRE_INIT_XCOR;
    blockStatus.gravity = BASE_NORMAL_GRAVITY;
}

void gameDataReset()
{
    gameData.score = 0;
    gameData.eliminateCount = 0;
    gameData.level = 1;
}

void eventGameReset()
{
    eventTetrisFrameReset();
    eventRenderAll();
    eventGenerateRandomSeeds(blockSeeds, BLOCK_NUMBER);

    blockStatusReset();
    gameDataReset();
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
    /*Caculate the score added in current round*/
    if (eliminateCount > 0)
    {
        ADD_ONE_ROUND_SCORE(eliminateCount, &(gameData.score), gameData.level);
    }
    /*Update the whole frame*/
    eventRenderAll();
    /*Update the score*/
    clearData();
    showData();
    /*Update the total eliminated block count*/
    gameData.eliminateCount += eliminateCount;
    GET_LEVEL(gameData.eliminateCount, &(gameData.level));
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

enum _showIdentifier_
{
    START = 1,
    RESTART = 2
};

void eventGameBoot(enum _showIdentifier_ showIdentifier)
{
    gameStatus = BOOT;
    int showFlag = 0;
    while (1)
    {
        if (!showFlag)
        {
            if (showIdentifier == RESTART)
            {
                showMessage(20, 8, 1);
                showMessage(20, 10, 2);
                showMessage(20, 12, 3);
            }
            else if (showIdentifier == START)
            {
                showMessage(20, 10, 0);
            }
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

    setbuf(stdin, inputBuf);
    SetConsoleTitle("Tetris");
    system("@chcp 65001>nul");

    eventTetrisFrameReset();
    eventRenderAll();
    eventGenerateRandomSeeds(blockSeeds, BLOCK_NUMBER);

    blockStatusReset();
    gameDataReset();

    gameStatus = BOOT;
}

int main()
{
    eventGameInit();
    showData();
    eventGameBoot(START);
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
                if (gameStatus == EXIT)
                {
                    break;
                }
                eventGameBoot(RESTART);
                eventGameReset();
                clearData();
                showData();
                break;
            }
            else
            {
                blockStatusReset();
            }
        }
    }
    return 0;
}