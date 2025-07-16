#include "BrainfuzzAI.h"
#include "QuickMath.h"
#include "stdlib.h"
#include "time.h"
#include "string.h"

#include "assert.h"
#include "stdio.h"

enum BrainfuzzAIResult BrainfuzzAIMutate(BrainfuzzToken *program, size_t *programCount, size_t programMaxCount, double mutationStrength, double mutationRate)
{
    uint64_t mutations = 0;
    while((double)(GlobalRandom() % 256) * mutationRate > 128.0 && mutations < mutationRate * 2)
    {
        mutations++;
    
        size_t mutationType = GlobalRandom() % 100;
        double currentMutationStrength = ((double)GlobalRandom() / (double)SIZE_MAX - 0.5) * mutationStrength;

        if(mutationType != 0 && *programCount > 0)
        {
            size_t index = (GlobalRandom() % *programCount);
            assert(index < *programCount);
            program[index].Magnitude += currentMutationStrength;

            if(program[index].Magnitude < 0.05 && program[index].Magnitude > 0.05)
            {
                memmove(program + index, program + index + 1, (*programCount - index - 1) * sizeof(BrainfuzzToken));
                *programCount -= 1;
            }
        }
        else 
        {
            if(programMaxCount <= *programCount)
                return BrainfuzzAIResultProgramMaxCountExceeded;
            
            size_t index = GlobalRandom() % (*programCount + 1);
            assert(index <= *programCount);
            enum BrainfuzzTokenType tokenType = GlobalRandom() % (BrainfuzzTokenMax + 1);

            if(index > 0 && program[index - 1].Type == tokenType)
                program[index - 1].Magnitude += currentMutationStrength;
            if(index < *programCount - 1 && program[index + 1].Type == tokenType)
                program[index + 1].Magnitude += currentMutationStrength; 
            else
            {
                memmove(program + index + 1, program + index, (*programCount - index) * sizeof(BrainfuzzToken));
                *programCount += 1;
                program[index] = (BrainfuzzToken){.Type = tokenType, .Magnitude = currentMutationStrength};
            }
        }
    }

    return BrainfuzzAIResultSuccess;
}



enum BrainfuzzAIResult BrainfuzzAIEvolve(BrainfuzzToken *program, size_t *programCount, size_t programMaxCount, uint64_t generations, double (*scoreProgram)(BrainfuzzToken *program, size_t programCount))
{   
    enum ProgramState
    {
        ProgramStateInvalid,
        ProgramStateValid,
        ProgramStateParent
    };

    struct ProgramInfo
    {
        enum ProgramState State;
        size_t Count;
        double Score;
    };

    double maxAllowedScoreDrop = 0.3f;
    double allowedScoreDrop = 0.1f;

    const size_t generationSize = 64;
    // Maybe change this out for a malloc to reduce chance of stack overflow
    BrainfuzzToken programs[generationSize][programMaxCount];
    struct ProgramInfo programInfo[generationSize];

    BrainfuzzToken *parentProgram = program;
    struct ProgramInfo *parentInfo = programInfo;
    parentInfo->State = ProgramStateParent;
    parentInfo->Count = *programCount;
    parentInfo->Score = scoreProgram(program, *programCount);

    printf("StartingScore: %f\n", parentInfo->Score);

    BrainfuzzToken bestProgram[programMaxCount];
    struct ProgramInfo bestProgramInfo;

    for(uint64_t x = 0; x < generations; x++)
    {
        ssize_t highestScorerIndex = -1;

        for(size_t y = 0; y < generationSize; y++)
        { 
            
            if(programInfo[y].State == ProgramStateParent)
                continue;

            BrainfuzzToken *childProgram = programs[y];
            for(size_t z = 0; z < parentInfo->Count; z++)
                childProgram[z] = parentProgram[z];

            struct ProgramInfo childInfo = {
                .Count = parentInfo->Count
            };

            enum BrainfuzzAIResult result = BrainfuzzAIMutate(childProgram, &childInfo.Count, programMaxCount, 2, 3);
            
            childInfo.State = result == BrainfuzzAIResultSuccess ? ProgramStateValid : ProgramStateInvalid; 
            if(childInfo.State == ProgramStateValid)
            {
                childInfo.Score = scoreProgram(childProgram, childInfo.Count);
                if(highestScorerIndex == -1 || childInfo.Score > programInfo[highestScorerIndex].Score)
                    highestScorerIndex = y;                    
                    
                programInfo[y] = childInfo;
            }
            
        }

        printf("Final parent score: %f (Size: %zu) (Parent: %f Best: %f)\n", programInfo[highestScorerIndex].Score, parentInfo->Count, parentInfo->Score, bestProgramInfo.Score);
        if(programInfo[highestScorerIndex].Score < parentInfo->Score - allowedScoreDrop || programInfo[highestScorerIndex].Score < 0.5)
            continue;
        if(programInfo[highestScorerIndex].Score < bestProgramInfo.Score - maxAllowedScoreDrop)
        {
            parentInfo->State = ProgramStateInvalid;
            parentInfo = &bestProgramInfo;
            parentProgram = bestProgram;
        }

        parentProgram = programs[highestScorerIndex];
        parentInfo->State = ProgramStateInvalid;

        parentInfo = programInfo + highestScorerIndex;
        parentInfo->State = ProgramStateParent;

        if(parentInfo->Score > bestProgramInfo.Score)
        {
            memcpy(bestProgram, parentProgram, parentInfo->Count * sizeof(BrainfuzzToken));
            bestProgramInfo = *parentInfo;
        }
    }

    memcpy(program, bestProgram, bestProgramInfo.Count * sizeof(BrainfuzzToken));
    *programCount = bestProgramInfo.Count;

    printf("Final score: %f\n", bestProgramInfo.Score.Primary);

    return BrainfuzzAIResultSuccess;
}