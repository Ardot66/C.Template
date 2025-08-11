#ifndef __BRAINFUZZ_AI__
#define __BRAINFUZZ_AI__

#include "Brainfuzz.h"

enum BrainfuzzAIResult
{
    BrainfuzzAIResultSuccess = 0,
    BrainfuzzAIResultProgramMaxCountExceeded
};

enum BrainfuzzAIResult BrainfuzzAIEvolve(BrainfuzzToken *program, size_t *programCount, size_t programMaxCount, uint64_t generations, double (*scoreProgram)(BrainfuzzToken *program, size_t programCount, double programSize, uint64_t precision));

#endif