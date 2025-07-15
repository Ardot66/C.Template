#include "BrainfuzzAI.h"
#include "QuickMath.h"
#include "stdio.h"

const size_t programMaxCount = 1024;

double ScoreProgram(BrainfuzzToken *program, size_t programCount)
{
    double averageScore = 10;
    
    for(size_t x = 0; x < 20; x++)
    {
        BrainfuzzExecutionState executionState = BrainfuzzExecutionStateDefault;
        executionState.Program = program;
        executionState.ProgramCount = programCount;

        executionState.DataBufferCount = 256;
        double dataBuffer[executionState.DataBufferCount];
        executionState.DataBuffer = dataBuffer;

        executionState.ExecutionLimit = 512;

        for(size_t x = 0; x < executionState.DataBufferCount; x++)
            dataBuffer[x] = 0;
        
        double inputValue = (GlobalRandom() % 64) + 1;
        double inputValueTwo = (GlobalRandom() % 64) + 1;
        dataBuffer[0] = inputValue;
        dataBuffer[1] = inputValueTwo;

        enum BrainfuzzResult result = BrainfuzzExecute(&executionState);
        double score = 0;

        if(result != BrainfuzzResultExecutionLimitReached)
        {
            double outputValue = dataBuffer[2];
            score = 10 - Abs(inputValue + inputValueTwo - outputValue) / (inputValue + inputValueTwo);    

            // score *= executionState.ExecutionLimit - executionState.Executed;
            // score *= programCount - programMaxCount;
        }

        averageScore = (averageScore + score) / 2;
    }

    // printf("Score: %f\n", averageScore);

    return averageScore;
}

int main()
{
    BrainfuzzToken program[programMaxCount];

    size_t programCount = 0;
    BrainfuzzAIEvolve(program, &programCount, programMaxCount, 1000, ScoreProgram);
}