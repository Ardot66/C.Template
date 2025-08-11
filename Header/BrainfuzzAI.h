#ifndef __BRAINFUZZ_AI__
#define __BRAINFUZZ_AI__

#include "Brainfuzz.h"

enum BrainfuzzAIResult
{
    BrainfuzzAIResultSuccess = 0,
    BrainfuzzAIResultProgramMaxCountExceeded
};

enum BrainfuzzAIResult BrainfuzzAIMutate(BrainfuzzToken *program, size_t *programCount, size_t programMaxCount, double mutationStrength, double mutationRate, uint64_t seed);
enum BrainfuzzAIResult BrainfuzzAIEvolve(BrainfuzzToken *program, size_t *programCount, size_t programMaxCount, uint64_t generations, double (*scoreProgram)(BrainfuzzToken *program, size_t programCount, double precision));

#endif