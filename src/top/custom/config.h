// +FHDR========================================================================
//  License:
//
//  =============================================================================
//  File Name:      config.h
//                  Rocky (luoqi754@gmail.com)
//  Organization:
//  Description:
//      Common configuration parameters for the GEMM C model.
//      All shared macros are defined here to avoid duplication.
// -FHDR========================================================================
#ifndef CONFIG_H
#define CONFIG_H

#ifndef EXP_WIDTH
#define EXP_WIDTH  8    // Exponent width
#endif

#ifndef MAT_WIDTH
#define MAT_WIDTH  24   // Mantissa width
#endif

#ifndef EXT_WIDTH
#define EXT_WIDTH  4    // Extension width for accumulation
#endif

#ifndef ROW_SIZE
#define ROW_SIZE   16   // Number of rows in activation/weight tile
#endif

#ifndef COL_SIZE
#define COL_SIZE   16   // Number of columns in weight/output tile
#endif

#ifndef BATCH_SIZE
#define BATCH_SIZE 1    // Batch size for parallel processing (must match main_gemm.c!)
#endif


#define CLOG2(x) \
    ((x) <= 1 ? 0 : \
     (sizeof(unsigned long long) * 8 - __builtin_clzll((unsigned long long)((x) - 1))))

#define MAT_SHIFT CLOG2(MAT_WIDTH-2)

#endif // CONFIG_H
