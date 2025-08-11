#include "BrainfuzzAI.h"
#include "QuickMath.h"
#include "stdio.h"

const size_t programMaxCount = 320;

double ScoreProgram(BrainfuzzToken *program, size_t programCount, double programSize, uint64_t precision)
{
    double totalScore = 0;
    
    for(size_t x = 0; x < precision * 10; x++)
    {
        BrainfuzzExecutionState executionState = BrainfuzzExecutionStateDefault;
        executionState.Program = program;
        executionState.ProgramCount = programCount;

        executionState.DataBufferCount = 256;
        double dataBuffer[executionState.DataBufferCount];
        executionState.DataBuffer = dataBuffer;

        executionState.ExecutionLimit = 2048;

        for(size_t x = 0; x < executionState.DataBufferCount; x++)
            dataBuffer[x] = 0;
        
        double inputValue = (GlobalRandom() % 16) + 1;
        double inputValueTwo = (GlobalRandom() % 16) + 1;
        dataBuffer[0] = inputValue;
        dataBuffer[1] = inputValueTwo;

        enum BrainfuzzResult result = BrainfuzzExecute(&executionState);
        double score = 0;

        if(result == BrainfuzzResultSuccess)
        {
            double outputValue = dataBuffer[2];
            score = 2 - Abs(inputValue + inputValueTwo - outputValue) / (inputValue + inputValueTwo);    
            score *= 1 - (double)programCount / 10;
            score *= 1 - programSize / 1000;
            score *= 1 - Abs(executionState.DataPointer - 2) / 10;
            score *= 1 - executionState.Executed / (executionState.ExecutionLimit * 100);

            // score *= programCount - programMaxCount;
        }

        totalScore += score;
    }

    // printf("Score: %f\n", averageScore);

    return totalScore / (double)(precision * 10);
}

int main()
{
    BrainfuzzToken program[programMaxCount];
    // FILE *input =     


    size_t programCount = 0;
    BrainfuzzAIEvolve(program, &programCount, programMaxCount, 100000, ScoreProgram);

    FILE *output = fopen("Bin/BFProgram.bfz", "w");
    for(size_t x = 0; x < programCount; x++)
    {
        char tokenBuffer[256];
        BrainfuzzWrite(tokenBuffer, sizeof(tokenBuffer), program[x]);
        fprintf(output, "%s ", tokenBuffer);
        printf("%s ", tokenBuffer);
        printf("(%d, %f) ", program[x].Type, program[x].Magnitude);
    }

    fclose(output);
    printf("\n");

    double programSize = 0;
    for(size_t x = 0; x < programCount; x++)
        programSize += Abs(program[x].Magnitude);

    double confirmationScore = ScoreProgram(program, programCount, programSize, 1);
    printf("Confirmation score: %lf\n", confirmationScore);
}