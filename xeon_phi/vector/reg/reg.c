#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>     // uint32_t
#include <inttypes.h>   // %"PRIu32"
#include <unistd.h>     // Sleep
#include <time.h>       // Time
#include <omp.h>        // OpenMP

// Xeon Phi Configuration
#define MIC_CORES       (56)            // Max. 56 Cores (+1 core runs de OS)
#define MIC_THREADS     (4*MIC_CORES)   // Max. 4 Threads per Core.
#define MAX_ERROR       32              // Max. number of errors per repetition
#define LOG_SIZE        128             // Line size per error
#define BUSY            5000000         // Repetitions in the busy wait

#define ITEMS           16              // 64 bytes (512bits) ZMM register / element size

#define LOOP_BLOCK(V) \
        {\
            asm volatile("vmovdqa32 %%zmm"#V", %0" : "=m" (vec[0]) : : "zmm"#V); \
            for(j = 0; j < ITEMS; j++) { \
                if (vec[j] != refw) \
                    snprintf(log[th_id][errors++], LOG_SIZE, "%s IT:%"PRIu64" POS:%d TH:%d, REF:0x%08x WAS:0x%08x\n", time, i, j, th_id, refw, vec[j]); \
            } \
        }

// =============================================================================
uint64_t string_to_uint64(char *string) {
    uint64_t result = 0;
    char c;

    for (  ; (c = *string ^ '0') <= 9 && c >= 0; ++string) {
        result = result * 10 + c;
    }
    return result;
}

//======================================================================
int main(int argc, char *argv[]) {

    uint64_t repetitions = 0;

    if(argc != 2) {
        fprintf(stderr,"Please provide the number of <repetitions>.\n");
        exit(EXIT_FAILURE);
    }

    repetitions = string_to_uint64(argv[1]);
    if (repetitions == 0)       repetitions -= 1;   // MAX UINT64_T = 18446744073709551615
    omp_set_num_threads(MIC_THREADS);

    fprintf(stderr,"#HEADER Repetitions:%"PRIu64" ",    repetitions);
    fprintf(stderr,"Threads:%"PRIu32"\n",               MIC_THREADS);

    //==================================================================
    // Time stamp
    {
        time_t     now = time(0);
        struct tm  tstruct = *localtime(&now);
        char       time[64];
        strftime(time, sizeof(time), "#BEGIN Y:%Y M:%m D:%d Time:%X", &tstruct);
        fprintf(stderr,"%s\n", time);
    }

    //==================================================================
    // Benchmark variables
    double startTime,  duration;
    uint32_t th_id = 0;
    uint64_t i = 0;
    uint64_t j = 0;
    uint32_t errors = 0;

    uint32_t x;
    uint32_t y;
    char log[MIC_THREADS][MAX_ERROR][LOG_SIZE];

    //==================================================================
    // Benchmark
    for(i = 0; i < repetitions; i++) {

        //======================================================================
        // Prepare the log
        for (x = 0; x < MIC_THREADS; x++)
            for (y = 0; y < MAX_ERROR; y++)
                log[x][y][0] = '\0';

        errors = 0;

        time_t     now = time(0);
        struct tm  tstruct = *localtime(&now);
        char       time[64];
        strftime(time, sizeof(time), "#ERROR Y:%Y M:%m D:%d Time:%X", &tstruct);

        //======================================================================P
        // Parallel region
        #pragma offload target(mic) inout(log)
        {
            #pragma omp parallel for private(th_id, j) reduction(+:errors)
            for(th_id = 0; th_id < MIC_THREADS; th_id++)
            {
                asm volatile ("nop");
                asm volatile ("nop");

                // Portion of memory with 512 bits
                __declspec(aligned(64)) uint32_t vec[ITEMS];

                uint32_t refw = 0;
                //==============================================================
                // Initialize the variables with a new REFWORD
                if ((i % 3) == 0)
                    refw = 0x0;
                else if ((i % 3) == 1)
                    refw = 0xFFFFFFFF;
                else
                    refw = 0x55555555;

                asm volatile("vpbroadcastd %0, %%zmm0" :  : "m" (refw) : "zmm0");
                asm volatile("vpbroadcastd %0, %%zmm1" :  : "m" (refw) : "zmm1");
                asm volatile("vpbroadcastd %0, %%zmm2" :  : "m" (refw) : "zmm2");
                asm volatile("vpbroadcastd %0, %%zmm3" :  : "m" (refw) : "zmm3");
                asm volatile("vpbroadcastd %0, %%zmm4" :  : "m" (refw) : "zmm4");
                asm volatile("vpbroadcastd %0, %%zmm5" :  : "m" (refw) : "zmm5");
                asm volatile("vpbroadcastd %0, %%zmm6" :  : "m" (refw) : "zmm6");
                asm volatile("vpbroadcastd %0, %%zmm7" :  : "m" (refw) : "zmm7");

                asm volatile("vpbroadcastd %0, %%zmm8" :  : "m" (refw) : "zmm8");
                asm volatile("vpbroadcastd %0, %%zmm9" :  : "m" (refw) : "zmm9");
                asm volatile("vpbroadcastd %0, %%zmm10" :  : "m" (refw) : "zmm10");
                asm volatile("vpbroadcastd %0, %%zmm11" :  : "m" (refw) : "zmm11");
                asm volatile("vpbroadcastd %0, %%zmm12" :  : "m" (refw) : "zmm12");
                asm volatile("vpbroadcastd %0, %%zmm13" :  : "m" (refw) : "zmm13");
                asm volatile("vpbroadcastd %0, %%zmm14" :  : "m" (refw) : "zmm14");
                asm volatile("vpbroadcastd %0, %%zmm15" :  : "m" (refw) : "zmm15");

                asm volatile("vpbroadcastd %0, %%zmm15" :  : "m" (refw) : "zmm15");
                asm volatile("vpbroadcastd %0, %%zmm16" :  : "m" (refw) : "zmm16");
                asm volatile("vpbroadcastd %0, %%zmm17" :  : "m" (refw) : "zmm17");
                asm volatile("vpbroadcastd %0, %%zmm18" :  : "m" (refw) : "zmm18");
                asm volatile("vpbroadcastd %0, %%zmm19" :  : "m" (refw) : "zmm19");
                asm volatile("vpbroadcastd %0, %%zmm20" :  : "m" (refw) : "zmm20");
                asm volatile("vpbroadcastd %0, %%zmm21" :  : "m" (refw) : "zmm21");
                asm volatile("vpbroadcastd %0, %%zmm22" :  : "m" (refw) : "zmm22");
                asm volatile("vpbroadcastd %0, %%zmm23" :  : "m" (refw) : "zmm23");

                asm volatile("vpbroadcastd %0, %%zmm24" :  : "m" (refw) : "zmm24");
                asm volatile("vpbroadcastd %0, %%zmm25" :  : "m" (refw) : "zmm25");
                asm volatile("vpbroadcastd %0, %%zmm26" :  : "m" (refw) : "zmm26");
                asm volatile("vpbroadcastd %0, %%zmm27" :  : "m" (refw) : "zmm27");
                asm volatile("vpbroadcastd %0, %%zmm28" :  : "m" (refw) : "zmm28");
                asm volatile("vpbroadcastd %0, %%zmm29" :  : "m" (refw) : "zmm29");
                asm volatile("vpbroadcastd %0, %%zmm30" :  : "m" (refw) : "zmm30");
                asm volatile("vpbroadcastd %0, %%zmm31" :  : "m" (refw) : "zmm31");

                //==============================================================
                // Busy wait
                for(j = (repetitions == 0); j < BUSY; j++) {
                    asm volatile ("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
                    asm volatile ("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
                    asm volatile ("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
                    asm volatile ("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
                    asm volatile ("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;");
                }

                //==========================================================
                // DEBUG: injecting one error (Bit-wise not RefWord)
                //if(th_id == 0 && i == 0)
                    //asm volatile("vpbroadcastd %0, %%zmm0" :  : "m" (~refw) : "zmm0");

                LOOP_BLOCK(0)
                LOOP_BLOCK(1)
                LOOP_BLOCK(2)
                LOOP_BLOCK(3)
                LOOP_BLOCK(4)
                LOOP_BLOCK(5)
                LOOP_BLOCK(6)
                LOOP_BLOCK(7)

                LOOP_BLOCK(8)
                LOOP_BLOCK(9)
                LOOP_BLOCK(10)
                LOOP_BLOCK(11)
                LOOP_BLOCK(12)
                LOOP_BLOCK(13)
                LOOP_BLOCK(14)
                LOOP_BLOCK(15)

                LOOP_BLOCK(16)
                LOOP_BLOCK(17)
                LOOP_BLOCK(18)
                LOOP_BLOCK(19)
                LOOP_BLOCK(20)
                LOOP_BLOCK(21)
                LOOP_BLOCK(22)
                LOOP_BLOCK(23)

                LOOP_BLOCK(24)
                LOOP_BLOCK(25)
                LOOP_BLOCK(26)
                LOOP_BLOCK(27)
                LOOP_BLOCK(28)
                LOOP_BLOCK(29)
                LOOP_BLOCK(30)
                LOOP_BLOCK(31)

            }
        }

        //======================================================================
        // Write the log if exists
        for (x = 0; x < MIC_THREADS; x++)
            for (y = 0; y < MAX_ERROR; y++)
                if (log[x][y][0] != '\0')
                    fprintf(stderr,"%s", log[x][y]);

    }

    //==================================================================
    // Time stamp
    {
        time_t     now = time(0);
        struct tm  tstruct = *localtime(&now);
        char       time[64];
        strftime(time, sizeof(time), "#FINAL Y:%Y M:%m D:%d Time:%X", &tstruct);
        fprintf(stderr,"%s\n", time);
    }

    exit(EXIT_SUCCESS);
}
