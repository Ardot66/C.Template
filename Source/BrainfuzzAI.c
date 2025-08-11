#include "BrainfuzzAI.h"
#include "QuickMath.h"
#include "stdlib.h"
#include "time.h"
#include "string.h"

#include "assert.h"
#include "stdio.h"

enum ProgramState
{
    ProgramStateDead,
    ProgramStateAlive
};

typedef struct ProgramInfo 
{
    BrainfuzzToken *Program;
    enum ProgramState State;
    size_t Count;
    double Size;
    double Score;
    uint64_t Seed;
} ProgramInfo;

typedef struct EvolutionState
{
    void *Allocation;

    size_t GenerationSize;
    BrainfuzzToken *ProgramBuffer;
    ProgramInfo *ProgramInfo;

    size_t SurvivingProgramsCount;
    ProgramInfo **SurvivingPrograms;
    double SurvivingProgramsSquaredScoreTotal;

} EvolutionState;

double RandomFloat()
{
    return (double)GlobalRandom() / (double)UINT64_MAX;
}

EvolutionState AllocateEvolutionState(size_t generationSize, size_t programMaxCount)
{
    EvolutionState state;
    state.GenerationSize = generationSize;
    state.SurvivingProgramsCount = 0;

    const size_t programBufferSize = generationSize * programMaxCount * sizeof(*state.ProgramBuffer);
    const size_t programInfoSize = sizeof(*state.ProgramInfo) * generationSize;
    const size_t survivingProgramsSize = sizeof(*state.SurvivingPrograms) * (generationSize + 1);

    state.Allocation = malloc(programBufferSize + programInfoSize + survivingProgramsSize);

    {
        char *tempAllocation = state.Allocation;
        state.ProgramBuffer = (void *)tempAllocation; 
        state.ProgramInfo = (void *)(tempAllocation += programBufferSize);
        state.SurvivingPrograms = (void *)(tempAllocation += programInfoSize);
    }

    for(size_t x = 0; x < generationSize; x++)
    {
        state.ProgramInfo[x] = (ProgramInfo)
        {
            .Program = state.ProgramBuffer + programMaxCount * x,
            .State = ProgramStateDead,
            .Count = 0,
            .Score = 0,
            .Size = 0,
            .Seed = 0
        };
    };

    return state;
}

ProgramInfo *SelectParent(EvolutionState *state)
{
    double selection = RandomFloat() * state->SurvivingProgramsSquaredScoreTotal;

    for(size_t x = 0; x < state->SurvivingProgramsCount; x++)
    {
        ProgramInfo *survivingProgram = state->SurvivingPrograms[x];
        selection -= survivingProgram->Score * survivingProgram->Score;

        if(selection <= 0)
            return survivingProgram;
    }

    return state->SurvivingPrograms[state->SurvivingProgramsCount - 1];
}

void SetInstructionMagnitude(ProgramInfo *program, size_t index, double magnitude)
{
    double oldMagnitude = program->Program[index].Magnitude;
    program->Program[index].Magnitude = magnitude;
    program->Size += Abs(magnitude) - Abs(oldMagnitude);
}

enum BrainfuzzAIResult BrainfuzzAIMutate(ProgramInfo *program, size_t programMaxCount, double mutationStrength, double mutationRate)
{
    double mutationDouble = RandomFloat();
    mutationDouble *= mutationDouble;

    uint64_t mutations = (uint64_t)(mutationDouble * mutationRate) + 1;

    const double 
        mutateMagnitudeRate = 1,
        mutateReduceMagnitudeRate = 0.2,
        mutateInsertRate = 0.1,
        mutateRateSum = mutateMagnitudeRate + mutateReduceMagnitudeRate + mutateInsertRate;

    for(uint64_t x = 0; x < mutations; x++)
    {
        double mutationType = RandomFloat() * mutateRateSum;
        double currentMutationStrength = (RandomFloat() - 0.5) * 2.0;
        currentMutationStrength *= currentMutationStrength;
        currentMutationStrength *= mutationStrength;

        if((mutationType -= mutateMagnitudeRate) < 0.0 && program->Count > 0)
        {
            size_t index = (GlobalRandom() % program->Count);
            SetInstructionMagnitude(program, index, program->Program[index].Magnitude + currentMutationStrength);
        }
        else if((mutationType -= mutateReduceMagnitudeRate) < 0.0 && program->Count > 0)
        {
            size_t index = (GlobalRandom() % program->Count);
            double reduceMutationStrength = Abs(currentMutationStrength) + 1;
            
            float oldMagnitude = program->Program[index].Magnitude;
            float oldSize = program->Size;
            SetInstructionMagnitude(program, index, program->Program[index].Magnitude / reduceMutationStrength);

            if(program->Size < 0)
            {
                printf("%f, %f, %f, %f\n", oldMagnitude, program->Program[index].Magnitude, oldSize, program->Size);
                printf("It's %f\n", program->Size);
                assert(program->Size >= 0);
            }

            if(program->Program[index].Magnitude * RandomFloat() / reduceMutationStrength < 0.1)
            {
                // SetInstructionMagnitude(program, index, 0);
                // memmove(program->Program + index, program->Program + index + 1, (program->Count - index - 1) * sizeof(BrainfuzzToken));
                // program->Count -= 1;
            }
        }
        else if((mutationType -= mutateInsertRate) < 0.0)
        {
            if(programMaxCount <= program->Count)
                return BrainfuzzAIResultProgramMaxCountExceeded;
            
            size_t index = GlobalRandom() % (program->Count + 1);
            assert(index <= program->Count);

            enum BrainfuzzTokenType tokenType = (GlobalRandom() % BrainfuzzTokenMax) + 1;

            // if(index > 0 && program->Program[index - 1].Type == tokenType)
            //     SetInstructionMagnitude(program, index - 1, program->Program[index - 1].Magnitude + currentMutationStrength);
            // else if(program->Program[index].Type == tokenType)
            //     SetInstructionMagnitude(program, index, program->Program[index].Magnitude + currentMutationStrength);
            // else
            {
                memmove(program->Program + index + 1, program->Program + index, (program->Count - index) * sizeof(BrainfuzzToken));
                program->Count += 1;
                program->Program[index] = (BrainfuzzToken){.Type = tokenType, .Magnitude = 0};
                SetInstructionMagnitude(program, index, currentMutationStrength);
                if(program->Size < Abs(currentMutationStrength))
                {
                    printf("%f, %f", program->Size, currentMutationStrength);
                    assert(1 == 0);
                }
            }
        }
        else
            assert(1 == 0);
    }

    return BrainfuzzAIResultSuccess;
}

enum BrainfuzzAIResult BrainfuzzAIEvolve(BrainfuzzToken *program, size_t *programCount, size_t programMaxCount, uint64_t generations, double (*scoreProgram)(BrainfuzzToken *program, size_t programCount, double programSize, uint64_t precision))
{   
    const double survivalRate = 0.9;
    const double survivalFalloffMultiplier = 0.9;

    EvolutionState state = AllocateEvolutionState(32, programMaxCount);

    ProgramInfo parentInfo = {
        .Program = program,
        .State = ProgramStateAlive,
        .Count = *programCount,
        .Score = scoreProgram(program, *programCount, 0, 10),
        .Size = 0,
        .Seed = 0
    };

    state.SurvivingProgramsCount = 1;
    state.SurvivingPrograms[0] = &parentInfo;

    ProgramInfo *sortedPrograms[state.GenerationSize];

    for(uint64_t x = 0; x < generations; x++)
    {
        for(size_t y = 0; y < state.GenerationSize; y++)
        { 
            ProgramInfo *programInfo = state.ProgramInfo + y;


            if(programInfo->State == ProgramStateDead)
            {
                ProgramInfo *parentInfo = SelectParent(&state);
                *programInfo = *parentInfo;            

                for(size_t z = 0; z < parentInfo->Count; z++)
                    programInfo->Program[z] = parentInfo->Program[z];

                enum BrainfuzzAIResult result = BrainfuzzAIMutate(programInfo, programMaxCount, 1, 1);
                programInfo->State = result == BrainfuzzAIResultSuccess ? ProgramStateAlive : ProgramStateDead; 
            }

            if(programInfo->State == ProgramStateAlive)
                programInfo->Score = scoreProgram(programInfo->Program, programInfo->Count, programInfo->Size, 5);
        }

        for(size_t y = 0; y < state.GenerationSize; y++)
            sortedPrograms[y] = state.ProgramInfo + y;

        for(size_t y = 0; y < state.GenerationSize; y++)
        {
            size_t highestScorerIndex = y;
            ProgramInfo *highestScorer = sortedPrograms[y];

            for(size_t z = y + 1; z < state.GenerationSize; z++)
            {
                if(highestScorer->Score < sortedPrograms[z]->Score && sortedPrograms[z]->State == ProgramStateAlive)
                {
                    highestScorer = sortedPrograms[z];
                    highestScorerIndex = z;
                }
            }

            ProgramInfo *temp = sortedPrograms[y];
            sortedPrograms[y] = highestScorer;
            sortedPrograms[highestScorerIndex] = temp;
        }

        double highestScore = sortedPrograms[0]->Score;
        double survivalMultiplier = 1;

        state.SurvivingProgramsSquaredScoreTotal = sortedPrograms[0]->Score * sortedPrograms[0]->Score;
        state.SurvivingProgramsCount = 1;
        state.SurvivingPrograms[0] = sortedPrograms[0];

        for(size_t y = 1; y < state.GenerationSize; y++)
        {
            ProgramInfo *programInfo = sortedPrograms[y];
            double scoreMultiplier = programInfo->Score / highestScore;
            scoreMultiplier *= scoreMultiplier;

            if(survivalMultiplier * scoreMultiplier * RandomFloat() > (1 - survivalRate))
            {
                state.SurvivingPrograms[state.SurvivingProgramsCount] = programInfo;
                state.SurvivingProgramsCount++;
                state.SurvivingProgramsSquaredScoreTotal += programInfo->Score * programInfo->Score;
            }
            else
                programInfo->State = ProgramStateDead;

            survivalMultiplier *= survivalFalloffMultiplier;
        }

        printf("%zu Best score: %f (Size: %zu, %f) %d\n", x, sortedPrograms[0]->Score, sortedPrograms[0]->Count, sortedPrograms[0]->Size, sortedPrograms[0]->State);
    }

    memcpy(program, sortedPrograms[0]->Program, sortedPrograms[0]->Count * sizeof(BrainfuzzToken));
    *programCount = sortedPrograms[0]->Count;

    printf("Final score: %f\n", sortedPrograms[0]->Score);

    free(state.Allocation);
    return BrainfuzzAIResultSuccess;
}