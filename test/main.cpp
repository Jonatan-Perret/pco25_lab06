#include <gtest/gtest.h>
#include <pcosynchro/pcotest.h>

#include "multipliertester.h"
#include "multiplierthreadedtester.h"
#include "threadedmatrixmultiplier.h"

#define ThreadedMultiplierType ThreadedMatrixMultiplier<float>

// Decommenting the next line allows to check for interlocking
#define CHECK_DURATION

TEST (Multiplier, SingleThread){

#ifdef CHECK_DURATION
  ASSERT_DURATION_LE (30, ({
#endif // CHECK_DURATION
                        constexpr int MATRIXSIZE = 500;
                        constexpr int NBTHREADS = 1;
                        constexpr int NBBLOCKSPERROW = 5;

                        MultiplierTester<ThreadedMultiplierType> tester;

                        tester.test (MATRIXSIZE, NBTHREADS, NBBLOCKSPERROW);

#ifdef CHECK_DURATION
                      }))
#endif // CHECK_DURATION

}

TEST (Multiplier, Simple){

#ifdef CHECK_DURATION
  ASSERT_DURATION_LE (30, ({
#endif // CHECK_DURATION
                        constexpr int MATRIXSIZE = 500;
                        constexpr int NBTHREADS = 4;
                        constexpr int NBBLOCKSPERROW = 5;

                        MultiplierTester<ThreadedMultiplierType> tester;

                        tester.test (MATRIXSIZE, NBTHREADS, NBBLOCKSPERROW);

#ifdef CHECK_DURATION
                      }))
#endif // CHECK_DURATION

}

TEST (Multiplier, Reentering){

#ifdef CHECK_DURATION
  ASSERT_DURATION_LE (
      30, ({
#endif // CHECK_DURATION
        constexpr int MATRIXSIZE = 500;
        constexpr int NBTHREADS = 4;
        constexpr int NBBLOCKSPERROW = 5;

        MultiplierThreadedTester<ThreadedMultiplierType> tester (2);

        tester.test (MATRIXSIZE, NBTHREADS, NBBLOCKSPERROW);

#ifdef CHECK_DURATION
      }))
#endif // CHECK_DURATION
}

// Additional test 1: Test with many threads (more threads than blocks)
TEST (Multiplier, ManyThreads){

#ifdef CHECK_DURATION
  ASSERT_DURATION_LE (30, ({
#endif // CHECK_DURATION
                        constexpr int MATRIXSIZE = 500;
                        constexpr int NBTHREADS
                            = 16; // More threads than blocks
                        constexpr int NBBLOCKSPERROW = 5;

                        MultiplierTester<ThreadedMultiplierType> tester;

                        tester.test (MATRIXSIZE, NBTHREADS, NBBLOCKSPERROW);

#ifdef CHECK_DURATION
                      }))
#endif // CHECK_DURATION

}

// Additional test 2: Test with large number of blocks
TEST (Multiplier, ManyBlocks){

#ifdef CHECK_DURATION
  ASSERT_DURATION_LE (30, ({
#endif // CHECK_DURATION
                        constexpr int MATRIXSIZE = 500;
                        constexpr int NBTHREADS = 4;
                        constexpr int NBBLOCKSPERROW
                            = 10; // More blocks for finer granularity

                        MultiplierTester<ThreadedMultiplierType> tester;

                        tester.test (MATRIXSIZE, NBTHREADS, NBBLOCKSPERROW);

#ifdef CHECK_DURATION
                      }))
#endif // CHECK_DURATION

}

// Additional test 3: Test reentrancy with more parallel multiplications
TEST (Multiplier, MultipleReentering){

#ifdef CHECK_DURATION
  ASSERT_DURATION_LE (30, ({
#endif // CHECK_DURATION
                        constexpr int MATRIXSIZE = 500;
                        constexpr int NBTHREADS = 4;
                        constexpr int NBBLOCKSPERROW = 5;

                        MultiplierThreadedTester<ThreadedMultiplierType>
                            tester (4); // 4 parallel computations

                        tester.test (MATRIXSIZE, NBTHREADS, NBBLOCKSPERROW);

#ifdef CHECK_DURATION
                      }))
#endif // CHECK_DURATION
}

// Additional test 4: Test with minimal blocks (2x2)
TEST (Multiplier, MinimalBlocks){

#ifdef CHECK_DURATION
  ASSERT_DURATION_LE (30, ({
#endif // CHECK_DURATION
                        constexpr int MATRIXSIZE = 500;
                        constexpr int NBTHREADS = 4;
                        constexpr int NBBLOCKSPERROW
                            = 2; // Just 4 blocks total

                        MultiplierTester<ThreadedMultiplierType> tester;

                        tester.test (MATRIXSIZE, NBTHREADS, NBBLOCKSPERROW);

#ifdef CHECK_DURATION
                      }))
#endif // CHECK_DURATION

}

// Additional test 5: Test with larger matrix
TEST (Multiplier, LargeMatrix){

#ifdef CHECK_DURATION
  ASSERT_DURATION_LE (60, ({ // More time for larger matrix
#endif                       // CHECK_DURATION
                        constexpr int MATRIXSIZE = 800;
                        constexpr int NBTHREADS = 8;
                        constexpr int NBBLOCKSPERROW = 8;

                        MultiplierTester<ThreadedMultiplierType> tester;

                        tester.test (MATRIXSIZE, NBTHREADS, NBBLOCKSPERROW);

#ifdef CHECK_DURATION
                      }))
#endif // CHECK_DURATION

}

// Additional test 6: Test with single block (no decomposition)
TEST (Multiplier, NoDecomposition){

#ifdef CHECK_DURATION
  ASSERT_DURATION_LE (30, ({
#endif // CHECK_DURATION
                        constexpr int MATRIXSIZE = 500;
                        constexpr int NBTHREADS = 4;
                        constexpr int NBBLOCKSPERROW
                            = 1; // Single block = no parallelism

                        MultiplierTester<ThreadedMultiplierType> tester;

                        tester.test (MATRIXSIZE, NBTHREADS, NBBLOCKSPERROW);

#ifdef CHECK_DURATION
                      }))
#endif // CHECK_DURATION

}

// Additional test 7: Stress test with high thread count and many blocks
TEST (Multiplier, StressTest){

#ifdef CHECK_DURATION
  ASSERT_DURATION_LE (30, ({
#endif // CHECK_DURATION
                        constexpr int MATRIXSIZE = 500;
                        constexpr int NBTHREADS = 20;      // Many threads
                        constexpr int NBBLOCKSPERROW = 10; // Many small blocks

                        MultiplierTester<ThreadedMultiplierType> tester;

                        tester.test (MATRIXSIZE, NBTHREADS, NBBLOCKSPERROW);

#ifdef CHECK_DURATION
                      }))
#endif // CHECK_DURATION

}

// Additional test 8: Test reentrancy under stress
TEST (Multiplier, StressReentering){

#ifdef CHECK_DURATION
  ASSERT_DURATION_LE (45, ({
#endif // CHECK_DURATION
                        constexpr int MATRIXSIZE = 500;
                        constexpr int NBTHREADS = 8;
                        constexpr int NBBLOCKSPERROW = 5;

                        MultiplierThreadedTester<ThreadedMultiplierType>
                            tester (8); // 8 parallel computations

                        tester.test (MATRIXSIZE, NBTHREADS, NBBLOCKSPERROW);

#ifdef CHECK_DURATION
                      }))
#endif // CHECK_DURATION
}

// Additional test 9: Test with perfect square decomposition
TEST (Multiplier, PerfectSquare){

#ifdef CHECK_DURATION
  ASSERT_DURATION_LE (30, ({
#endif                                                  // CHECK_DURATION
                        constexpr int MATRIXSIZE = 400; // 400 = 20 * 20
                        constexpr int NBTHREADS = 4;
                        constexpr int NBBLOCKSPERROW
                            = 20; // 20x20 blocks of 20x20 each

                        MultiplierTester<ThreadedMultiplierType> tester;

                        tester.test (MATRIXSIZE, NBTHREADS, NBBLOCKSPERROW);

#ifdef CHECK_DURATION
                      }))
#endif // CHECK_DURATION

}

// Additional test 10: Test with few threads and many blocks
TEST (Multiplier, FewThreadsManyBlocks)
{

#ifdef CHECK_DURATION
  ASSERT_DURATION_LE (30, ({
#endif // CHECK_DURATION
                        constexpr int MATRIXSIZE = 500;
                        constexpr int NBTHREADS = 2;       // Only 2 threads
                        constexpr int NBBLOCKSPERROW = 10; // But many blocks

                        MultiplierTester<ThreadedMultiplierType> tester;

                        tester.test (MATRIXSIZE, NBTHREADS, NBBLOCKSPERROW);

#ifdef CHECK_DURATION
                      }))
#endif // CHECK_DURATION
}

int
main (int argc, char **argv)
{
  testing::InitGoogleTest (&argc, argv);

  return RUN_ALL_TESTS ();
}
