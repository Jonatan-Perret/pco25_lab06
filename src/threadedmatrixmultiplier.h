#ifndef THREADEDMATRIXMULTIPLIER_H
#define THREADEDMATRIXMULTIPLIER_H

///
/// Multithreaded Matrix Multiplication using Block Decomposition
/// ============================================================
///
/// This implementation optimizes matrix multiplication for multi-core architectures by:
/// 1. Decomposing matrices into blocks that can be computed independently
/// 2. Using a producer-consumer pattern with job delegation
/// 3. Synchronizing via a Hoare monitor (Buffer class)
/// 4. Supporting reentrant multiply() calls with per-computation tracking
///
/// Architecture:
///   Main Thread (multiply) -> [Creates jobs] -> Buffer (Monitor) -> Worker Threads
///
/// Key design decisions:
/// - Each job computes one complete block C[i][j] = Σ A[i][k] * B[k][j]
/// - No race conditions since each block is written by only one thread
/// - Computation IDs enable multiple concurrent multiply() calls (reentrancy)
/// - Thread pool is created once in constructor and reused across computations
///

#include <pcosynchro/pcoconditionvariable.h>
#include <pcosynchro/pcohoaremonitor.h>
#include <pcosynchro/pcomutex.h>
#include <pcosynchro/pcosemaphore.h>
#include <pcosynchro/pcothread.h>

#include <queue>
#include <map>
#include <memory>

#include "abstractmatrixmultiplier.h"
#include "matrix.h"


///
/// A class that holds the necessary parameters for a thread to do a job.
///
template<class T>
class ComputeParameters
{
public:
    const SquareMatrix<T>* A{nullptr};
    const SquareMatrix<T>* B{nullptr};
    SquareMatrix<T>* C{nullptr};
    
    // Block indices (i, j) representing which block of C to compute
    int blockI{0};
    int blockJ{0};
    
    // Number of blocks per row/column
    int nbBlocksPerRow{0};
    
    // Computation ID to track which multiply() call this belongs to
    int computationId{0};
};


/// As a suggestion, a buffer class that could be used to communicate between
/// the workers and the main thread...
///
/// Here we only wrote two potential methods, but there could be more at the end...
///
template<class T>
class Buffer : protected PcoHoareMonitor
{
public:
    int nbJobFinished{0}; // Keep this updated (for compatibility)
    
    Buffer() : isTerminating(false), nextComputationId(0) {}
    
    ///
    /// \brief Sends a job to the buffer
    /// \param params Reference to a ComputeParameters object which holds the necessary parameters to execute a job
    ///
    void sendJob(ComputeParameters<T> params) {
        monitorIn();
        jobQueue.push(params);
        signal(jobAvailable);
        monitorOut();
    }

    ///
    /// \brief Requests a job to the buffer
    /// \param parameters Reference to a ComputeParameters object which holds the necessary parameters to execute a job
    /// \return true if a job is available, false otherwise (when terminating)
    ///
    bool getJob(ComputeParameters<T>& parameters) {
        monitorIn();
        
        // Wait while no jobs available and not terminating
        while (jobQueue.empty() && !isTerminating) {
            wait(jobAvailable);
        }
        
        // If terminating and no jobs, return false
        if (isTerminating && jobQueue.empty()) {
            monitorOut();
            return false;
        }
        
        // Get job from queue
        parameters = jobQueue.front();
        jobQueue.pop();
        
        monitorOut();
        return true;
    }
    
    ///
    /// \brief Signals that a job has been completed
    /// \param computationId The ID of the computation this job belongs to
    ///
    void jobCompleted(int computationId) {
        monitorIn();
        nbJobFinished++; // Global counter for compatibility
        jobsFinishedPerComputation[computationId]++;
        signal(jobDone);
        monitorOut();
    }
    
    ///
    /// \brief Gets a new computation ID and initializes its counter
    /// \param totalJobs Total number of jobs for this computation
    /// \return The computation ID
    ///
    int startNewComputation(int totalJobs) {
        monitorIn();
        int id = nextComputationId++;
        jobsFinishedPerComputation[id] = 0;
        totalJobsPerComputation[id] = totalJobs;
        monitorOut();
        return id;
    }
    
    ///
    /// \brief Waits until all jobs for a specific computation are done
    /// \param computationId The ID of the computation to wait for
    ///
    void waitAllJobsDone(int computationId) {
        monitorIn();
        int totalJobs = totalJobsPerComputation[computationId];
        while (jobsFinishedPerComputation[computationId] < totalJobs) {
            wait(jobDone);
        }
        // Cleanup
        jobsFinishedPerComputation.erase(computationId);
        totalJobsPerComputation.erase(computationId);
        monitorOut();
    }
    
    ///
    /// \brief Signals termination to all worker threads
    ///
    void terminate() {
        monitorIn();
        isTerminating = true;
        // We need to wake all waiting threads
        // Since signal() in Hoare monitors wakes one thread at a time,
        // and we don't know how many are waiting, we keep the queue non-empty
        // by pushing dummy jobs, or just rely on the threads checking isTerminating
        // Actually, let's just use a simple approach: don't use signal here
        // The threads will check isTerminating when they next call getJob()
        // But some threads might be blocked in wait(). We need to signal them.
        // In a proper implementation, we'd use broadcast or signal in a loop
        monitorOut();
        
        // Push dummy jobs to wake threads (they'll check isTerminating and exit)
        for (int i = 0; i < 100; ++i) {  // More than enough to wake all possible threads
            monitorIn();
            signal(jobAvailable);
            monitorOut();
        }
    }

private:
    std::queue<ComputeParameters<T>> jobQueue;
    std::map<int, int> jobsFinishedPerComputation; // Track finished jobs per computation
    std::map<int, int> totalJobsPerComputation;     // Track total jobs per computation
    int nextComputationId;
    PcoHoareMonitor::Condition jobAvailable;
    PcoHoareMonitor::Condition jobDone;
    bool isTerminating;
};



///
/// A multi-threaded multiplicator. multiply() should at least be reentrant.
/// It is up to you to offer a very good parallelism.
///
template<class T>
class ThreadedMatrixMultiplier : public AbstractMatrixMultiplier<T>
{

public:
    ///
    /// \brief ThreadedMatrixMultiplier
    /// \param nbThreads Number of threads to start
    /// \param nbBlocksPerRow Default number of blocks per row, for compatibility with SimpleMatrixMultiplier
    ///
    /// The threads shall be started from the constructor
    ///
    ThreadedMatrixMultiplier(int nbThreads, int nbBlocksPerRow = 0)
        : nbThreads(nbThreads), nbBlocksPerRow(nbBlocksPerRow)
    {
        buffer = std::make_unique<Buffer<T>>();
        
        // Create and start worker threads
        for (int i = 0; i < nbThreads; ++i) {
            threads.push_back(std::make_unique<PcoThread>(&ThreadedMatrixMultiplier::workerThread, this));
        }
    }

    ///
    /// In this destructor we should ask for the termination of the computations. They could be aborted without
    /// ending into completion.
    /// All threads have to be joined.
    ///
    ~ThreadedMatrixMultiplier()
    {
        // Signal termination to all threads
        buffer->terminate();
        
        // Wait for all threads to finish
        for (auto& thread : threads) {
            thread->join();
        }
    }

    ///
    /// \brief multiply
    /// \param A First matrix
    /// \param B Second matrix
    /// \param C Result of AxB
    ///
    /// For compatibility reason with SimpleMatrixMultiplier
    void multiply(const SquareMatrix<T>& A, const SquareMatrix<T>& B, SquareMatrix<T>& C) override
    {
        multiply(A, B, C, nbBlocksPerRow);
    }

    ///
    /// \brief multiply
    /// \param A First matrix
    /// \param B Second matrix
    /// \param C Result of AxB
    /// \param nbBlocksPerRow Number of blocks per row (or columns)
    ///
    /// Executes the multithreaded computation, by decomposing the matrices into blocks.
    /// nbBlocksPerRow must divide the size of the matrix.
    ///
    void multiply(const SquareMatrix<T>& A, const SquareMatrix<T>& B, SquareMatrix<T>& C, int nbBlocksPerRow)
    {
        int n = A.size();
        int blockSize = n / nbBlocksPerRow;
        int totalBlocks = nbBlocksPerRow * nbBlocksPerRow;
        
        // Initialize result matrix to zero
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                C.setElement(i, j, 0);
            }
        }
        
        // Start a new computation and get its ID
        int computationId = buffer->startNewComputation(totalBlocks);
        
        // Create and send all jobs to the buffer
        for (int blockI = 0; blockI < nbBlocksPerRow; ++blockI) {
            for (int blockJ = 0; blockJ < nbBlocksPerRow; ++blockJ) {
                ComputeParameters<T> params;
                params.A = &A;
                params.B = &B;
                params.C = &C;
                params.blockI = blockI;
                params.blockJ = blockJ;
                params.nbBlocksPerRow = nbBlocksPerRow;
                params.computationId = computationId;
                
                buffer->sendJob(params);
            }
        }
        
        // Wait for all jobs of this computation to complete
        buffer->waitAllJobsDone(computationId);
    }

protected:
    int nbThreads;
    int nbBlocksPerRow;
    
    std::unique_ptr<Buffer<T>> buffer;
    std::vector<std::unique_ptr<PcoThread>> threads;
    
    ///
    /// \brief Worker thread function
    /// Continuously retrieves and processes jobs from the buffer
    ///
    void workerThread()
    {
        while (true) {
            ComputeParameters<T> params;
            
            // Get a job from the buffer
            if (!buffer->getJob(params)) {
                // No more jobs and terminating, exit thread
                break;
            }
            
            // Compute the block multiplication
            computeBlock(params);
            
            // Signal job completion for this computation
            buffer->jobCompleted(params.computationId);
        }
    }
    
    ///
    /// \brief Computes a single block of the matrix multiplication
    /// \param params Parameters containing the matrices and block indices
    ///
    /// Computes C[blockI][blockJ] = sum_k A[blockI][k] * B[k][blockJ]
    /// Each thread computes one complete block, so no race conditions on C elements
    ///
    void computeBlock(const ComputeParameters<T>& params)
    {
        const SquareMatrix<T>* A = params.A;
        const SquareMatrix<T>* B = params.B;
        SquareMatrix<T>* C = params.C;
        
        int n = A->size();
        int blockSize = n / params.nbBlocksPerRow;
        int nbBlocksPerRow = params.nbBlocksPerRow;
        
        int blockI = params.blockI;
        int blockJ = params.blockJ;
        
        // Compute the complete block C[blockI][blockJ]
        // For each element (i,j) in the block
        for (int i = blockI * blockSize; i < (blockI + 1) * blockSize; ++i) {
            for (int j = blockJ * blockSize; j < (blockJ + 1) * blockSize; ++j) {
                T sum = 0;
                
                // Sum over all K blocks: sum_k A[blockI][k] * B[k][blockJ]
                for (int blockK = 0; blockK < nbBlocksPerRow; ++blockK) {
                    for (int k = blockK * blockSize; k < (blockK + 1) * blockSize; ++k) {
                        sum += A->element(k, i) * B->element(j, k);
                    }
                }
                
                // Set the result (no race condition as each block is computed by one thread only)
                C->setElement(j, i, sum);
            }
        }
    }
};




#endif // THREADEDMATRIXMULTIPLIER_H
