#include "BrainfuzzAI.h"
#include "QuickMath.h"
#include "stdlib.h"
#include "time.h"
#include "string.h"

#include "assert.h"
#include "stdio.h"

double RandomFloat(RandomState *state)
{
    return (double)Random(state) / (double)UINT64_MAX;
}

enum BrainfuzzAIResult BrainfuzzAIMutate(BrainfuzzToken *program, size_t *programCount, size_t programMaxCount, double mutationStrength, double mutationRate, uint64_t seed)
{
    RandomState randomState = {.State[0] = (seed >> 32) * seed, .State[1] = (seed << 32) * seed};

    uint64_t mutations = 0;
    while((double)(Random(&randomState) % 256) * mutationRate > 128.0 && mutations < mutationRate * 2)
    {
        mutations++;
    
        size_t mutationType = Random(&randomState) % 300;
        double currentMutationStrength = (RandomFloat(&randomState) - 0.5) * mutationStrength;

        if(mutationType > 100 && *programCount > 0)
        {
            size_t index = (Random(&randomState) % *programCount);
            program[index].Magnitude += currentMutationStrength;
        }
        else if(mutationType > 50 && *programCount > 0)
        {
            size_t index = (Random(&randomState) % *programCount);
            program[index].Magnitude /= currentMutationStrength;

            if(program[index].Magnitude * RandomFloat(&randomState) / mutationStrength < 1)
            {
                memmove(program + index, program + index + 1, (*programCount - index - 1) * sizeof(BrainfuzzToken));
                *programCount -= 1;
            }
        }
        else 
        {
            if(programMaxCount <= *programCount)
                return BrainfuzzAIResultProgramMaxCountExceeded;
            
            size_t index = Random(&randomState) % (*programCount + 1);
            assert(index <= *programCount);
            enum BrainfuzzTokenType tokenType = Random(&randomState) % (BrainfuzzTokenMax + 1);

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

enum BrainfuzzAIResult BrainfuzzAIEvolve(BrainfuzzToken *program, size_t *programCount, size_t programMaxCount, uint64_t generations, double (*scoreProgram)(BrainfuzzToken *program, size_t programCount, double precision))
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
        uint64_t Seed;
    };

    double maxAllowedScoreDrop = 0.1f;
    double allowedScoreDrop = 0.00f;

    const size_t maxGenerationSize = 16;

    BrainfuzzToken *programs = malloc(maxGenerationSize * programMaxCount * sizeof(BrainfuzzToken));

    // Maybe change this out for a malloc to reduce chance of stack overflow
    struct ProgramInfo programInfo[maxGenerationSize];

    BrainfuzzToken *parentProgram = program;
    struct ProgramInfo *parentInfo = programInfo;
    parentInfo->State = ProgramStateParent;
    parentInfo->Count = *programCount;
    parentInfo->Score = scoreProgram(program, *programCount, 100);

    printf("StartingScore: %f\n", parentInfo->Score);

    BrainfuzzToken bestProgram[programMaxCount];
    struct ProgramInfo bestProgramInfo = {.State = ProgramStateInvalid};

    for(uint64_t x = 0; x < generations; x++)
    {
        if(parentInfo->Score < bestProgramInfo.Score - maxAllowedScoreDrop)
        {
            parentInfo->State = ProgramStateInvalid;
            parentInfo = &bestProgramInfo;
            parentProgram = bestProgram;
        }

        parentInfo->Score = (parentInfo->Score * 9 + scoreProgram(parentProgram, parentInfo->Count, 5)) / 10;
        if(bestProgramInfo.State != ProgramStateInvalid)
            bestProgramInfo.Score = (bestProgramInfo.Score * 9 + scoreProgram(bestProgram, bestProgramInfo.Count, 5)) / 10;
        
        ssize_t highestScorerIndex = -1;

        for(int r = 0; r < 2; r++)
        {
            uint64_t seed;
            double mutationStrength;
            ssize_t seederIndex = -1;

            switch(r)
            {
                case 0:
                {
                    seed = GlobalRandom();
                    if(bestProgramInfo.Score < 9.2)
                        mutationStrength = 2;
                    else
                        mutationStrength = 0.1;
                    break;
                }
                case 1:
                {
                    // printf("Highest scorer: %f\n", programInfo[highestScorerIndex].Score);
                    seed = programInfo[highestScorerIndex].Seed;
                    programInfo[highestScorerIndex].State = ProgramStateParent;
                    seederIndex = highestScorerIndex;
                    mutationStrength = 5;
                    break;
                }
            }

            for(size_t y = 0; y < maxGenerationSize; y++)
            { 
                if(programInfo[y].State == ProgramStateParent)
                    continue;

                BrainfuzzToken *childProgram = programs + y * programMaxCount;
                for(size_t z = 0; z < parentInfo->Count; z++)
                    childProgram[z] = parentProgram[z];

                struct ProgramInfo childInfo = {
                    .Count = parentInfo->Count,
                    .Seed = seed
                };

                enum BrainfuzzAIResult result = BrainfuzzAIMutate(childProgram, &childInfo.Count, programMaxCount, mutationStrength, 3, seed);
                
                childInfo.State = result == BrainfuzzAIResultSuccess ? ProgramStateValid : ProgramStateInvalid; 
                if(childInfo.State == ProgramStateValid)
                {
                    childInfo.Score = scoreProgram(childProgram, childInfo.Count, 1);
                    if(highestScorerIndex == -1 || childInfo.Score > programInfo[highestScorerIndex].Score)
                        highestScorerIndex = y;                    
                        
                    programInfo[y] = childInfo;
                }
            }

            if(seederIndex != -1)
                programInfo[seederIndex].State = ProgramStateValid;
        }

        programInfo[highestScorerIndex].Score = scoreProgram(programs + highestScorerIndex * programMaxCount, programInfo[highestScorerIndex].Count, 20);

        printf("%zu Best score: %f (Size: %zu) (Parent: %f Best: %f)\n", x, programInfo[highestScorerIndex].Score, parentInfo->Count, parentInfo->Score, bestProgramInfo.Score);
        if(programInfo[highestScorerIndex].Score < parentInfo->Score - allowedScoreDrop || programInfo[highestScorerIndex].Score < 0.5)
            continue;
        if(programInfo[highestScorerIndex].Score < bestProgramInfo.Score - maxAllowedScoreDrop)
        {
            parentInfo->State = ProgramStateInvalid;
            parentInfo = &bestProgramInfo;
            parentProgram = bestProgram;
        }

        parentProgram = programs + highestScorerIndex * programMaxCount;
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

    printf("Final score: %f\n", bestProgramInfo.Score);

    free(programs);
    return BrainfuzzAIResultSuccess;
}