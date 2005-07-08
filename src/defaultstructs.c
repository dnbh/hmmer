#include "structs.h"

/*
 * Note:  The default implementation doesn't use any custom data
 *        structures, so we these function bodies are empty.  We
 *        still have to provide them, however, for the program to
 *        compile successfully.  - CRS 20 June 2005
 */
inline void AllocLogoddsShell(struct plan7_s *hmm){ hmm->lom = NULL; }
inline void AllocLogoddsBody(struct plan7_s *hmm){}
inline void FreeLogodds(struct plan7_s *hmm){}
inline void FillCustomLogodds(struct plan7_s *hmm){}


/* Function: CreateDPMatrix()
 *
 * Note:     This was originally defined as CreatePlan7Matrix() in 
 *           core_algorithms.c, but it is really implementation-specific, so I
 *           moved it here, and renamed it so that its name was consistent
 *           with the declarations under the new architecture.
 *              - CRS 21 June 2005
 *
 * Purpose:  Create a dynamic programming matrix for standard Forward,
 *           Backward, or Viterbi, with scores kept as scaled log-odds
 *           integers. Keeps 2D arrays compact in RAM in an attempt 
 *           to maximize cache hits. 
 *           
 *           The mx structure can be dynamically grown, if a new
 *           HMM or seq exceeds the currently allocated size. Dynamic
 *           growing is more efficient than an alloc/free of a whole
 *           matrix for every new target. The ResizePlan7Matrix()
 *           call does this reallocation, if needed. Here, in the
 *           creation step, we set up some pads - to inform the resizing
 *           call how much to overallocate when it realloc's. 
 *           
 * Args:     N     - N+1 rows are allocated, for sequence.  
 *           M     - size of model in nodes
 *           padN  - over-realloc in seq/row dimension, or 0
 *           padM  - over-realloc in HMM/column dimension, or 0
 *                 
 * Return:   mx
 *           mx is allocated here. Caller frees with FreeDPMatrix(mx).
 */
struct dpmatrix_s *
CreateDPMatrix(int N, int M, int padN, int padM)
{
  struct dpmatrix_s *mx;
  int i;

  mx          = (struct dpmatrix_s *) MallocOrDie (sizeof(struct dpmatrix_s));
  mx->xmx     = (int **) MallocOrDie (sizeof(int *) * (N+1));
  mx->mmx     = (int **) MallocOrDie (sizeof(int *) * (N+1));
  mx->imx     = (int **) MallocOrDie (sizeof(int *) * (N+1));
  mx->dmx     = (int **) MallocOrDie (sizeof(int *) * (N+1));
  
  /*
   * Note: I am commenting out or replacing the lines with the *_mem pointers,
   *       since the default implementation doesn't actually use them.  As 
   *       noted below, they existed for the altivec implementation, but this
   *       is now acheived by taking advantage of the new architecture.
   *        - CRS 6 July 2005
   mx->xmx_mem = (void *) MallocOrDie (sizeof(int) * ((N+1)*5));
   mx->mmx_mem = (void *) MallocOrDie (sizeof(int) * ((N+1)*(M+2)));
   mx->imx_mem = (void *) MallocOrDie (sizeof(int) * ((N+1)*(M+2)));
   mx->dmx_mem = (void *) MallocOrDie (sizeof(int) * ((N+1)*(M+2)));

   /\* The indirect assignment below looks wasteful; it's actually
   * used for aligning data on 16-byte boundaries as a cache 
   * optimization in the fast altivec implementation
   *\/
   mx->xmx[0] = (int *) mx->xmx_mem;
   mx->mmx[0] = (int *) mx->mmx_mem;
   mx->imx[0] = (int *) mx->imx_mem;
   mx->dmx[0] = (int *) mx->dmx_mem;
   *
   */

  mx->xmx[0] = (void *) MallocOrDie (sizeof(int) * ((N+1)*5));
  mx->mmx[0] = (void *) MallocOrDie (sizeof(int) * ((N+1)*(M+2)));
  mx->imx[0] = (void *) MallocOrDie (sizeof(int) * ((N+1)*(M+2)));
  mx->dmx[0] = (void *) MallocOrDie (sizeof(int) * ((N+1)*(M+2)));

  for (i = 1; i <= N; i++)
    {
      mx->xmx[i] = mx->xmx[0] + (i*5); 
      mx->mmx[i] = mx->mmx[0] + (i*(M+2));
      mx->imx[i] = mx->imx[0] + (i*(M+2));
      mx->dmx[i] = mx->dmx[0] + (i*(M+2));
    }

  mx->maxN = N;
  mx->maxM = M;
  mx->padN = padN;
  mx->padM = padM;
  
  return mx;
}

/* Function: ResizeDPMatrix()
 * 
 * Note:     This was originally defined as ResizePlan7Matrix() in 
 *           core_algorithms.c, but I moved it here since it is implementation-
 *           specific, and renamed it so that it is consistent with the names
 *           under the new architecture.  - CRS 21 June 2005
 * 
 * Purpose:  Reallocate a dynamic programming matrix, if necessary,
 *           for a problem of NxM: sequence length N, model size M.
 *           (N=1 for small memory score-only variants; we allocate
 *           N+1 rows in the DP matrix.) 
 *           
 *           We know (because of the way hmmsearch and hmmpfam are coded)
 *           that only one of the two dimensions is going to change
 *           in size after the first call to ResizePlan7Matrix();
 *           that is, for hmmsearch, we have one HMM of fixed size M
 *           and our target sequences may grow in N; for hmmpfam,
 *           we have one sequence of fixed size N and our target models
 *           may grow in M. What we have to watch out for is P7SmallViterbi()
 *           working on a divide and conquer problem and passing us N < maxN,
 *           M > maxM; we should definitely *not* reallocate a smaller N.
 *           Since we know that only one dimension is going to grow,
 *           we aren't scared of reallocating to maxN,maxM. (If both
 *           M and N could grow, we would be more worried.)
 *
 *           Returns individual ptrs to the four matrix components
 *           as a convenience.
 *           
 * Args:     mx    - an already allocated model to grow.
 *           N     - seq length to allocate for; N+1 rows
 *           M     - size of model
 *           xmx, mmx, imx, dmx 
 *                 - RETURN: ptrs to four mx components as a convenience
 *                   
 * Return:   (void)
 *           mx is (re)allocated here.
 */
void
ResizeDPMatrix(struct dpmatrix_s *mx, int N, int M, 
	       int ***xmx, int ***mmx, int ***imx, int ***dmx)
{
  int i;

  if (N <= mx->maxN && M <= mx->maxM) goto DONE;
  
  if (N > mx->maxN) {
    N          += mx->padN; 
    mx->maxN    = N; 
    mx->xmx     = (int **) ReallocOrDie (mx->xmx, sizeof(int *) * (mx->maxN+1));
    mx->mmx     = (int **) ReallocOrDie (mx->mmx, sizeof(int *) * (mx->maxN+1));
    mx->imx     = (int **) ReallocOrDie (mx->imx, sizeof(int *) * (mx->maxN+1));
    mx->dmx     = (int **) ReallocOrDie (mx->dmx, sizeof(int *) * (mx->maxN+1));
  }

  if (M > mx->maxM) {
    M += mx->padM; 
    mx->maxM = M; 
  }

  /* 
   *Note: Same deal with the *_mem pointers as before.  - CRS 6 July 2005
   *
   mx->xmx_mem = (void *) ReallocOrDie (mx->xmx_mem, sizeof(int) * ((mx->maxN+1)*5));
   mx->mmx_mem = (void *) ReallocOrDie (mx->mmx_mem, sizeof(int) * ((mx->maxN+1)*(mx->maxM+2)));
   mx->imx_mem = (void *) ReallocOrDie (mx->imx_mem, sizeof(int) * ((mx->maxN+1)*(mx->maxM+2)));
   mx->dmx_mem = (void *) ReallocOrDie (mx->dmx_mem, sizeof(int) * ((mx->maxN+1)*(mx->maxM+2)));

   mx->xmx[0] = (int *) mx->xmx_mem;
   mx->mmx[0] = (int *) mx->mmx_mem;
   mx->imx[0] = (int *) mx->imx_mem;
   mx->dmx[0] = (int *) mx->dmx_mem;
   *
   */

  mx->xmx[0] = (void *) ReallocOrDie (mx->xmx[0], sizeof(int) * ((mx->maxN+1)*5));
  mx->mmx[0] = (void *) ReallocOrDie (mx->mmx[0], sizeof(int) * ((mx->maxN+1)*(mx->maxM+2)));
  mx->imx[0] = (void *) ReallocOrDie (mx->imx[0], sizeof(int) * ((mx->maxN+1)*(mx->maxM+2)));
  mx->dmx[0] = (void *) ReallocOrDie (mx->dmx[0], sizeof(int) * ((mx->maxN+1)*(mx->maxM+2)));

  for (i = 1; i <= mx->maxN; i++)
    {
      mx->xmx[i] = mx->xmx[0] + (i*5); 
      mx->mmx[i] = mx->mmx[0] + (i*(mx->maxM+2));
      mx->imx[i] = mx->imx[0] + (i*(mx->maxM+2));
      mx->dmx[i] = mx->dmx[0] + (i*(mx->maxM+2));
    }

 DONE:
  if (xmx != NULL) *xmx = mx->xmx;
  if (mmx != NULL) *mmx = mx->mmx;
  if (imx != NULL) *imx = mx->imx;
  if (dmx != NULL) *dmx = mx->dmx;
}

/* Function: AllocDPMatrix()
 * Date:     SRE, Tue Nov 19 07:14:47 2002 [St. Louis]
 *
 * Note:     This was originally defined as AllocPlan7Matrix in 
 *           core_algorithms.c, but I moved it here since it is 
 *           implementation-specific, and renamed it so that its name is
 *           consistent with the names under the new architecture.
 *             - CRS 21 June 2005
 *
 * Purpose:  Used to be the main allocator for dp matrices; we used to
 *           allocate, calculate, free. But this spent a lot of time
 *           in malloc(). Replaced with Create..() and Resize..() to
 *           allow matrix reuse in P7Viterbi(), the main alignment 
 *           engine. But matrices are alloc'ed by other alignment engines
 *           too, ones that are less frequently called and less 
 *           important to optimization of cpu performance. Instead of
 *           tracking changes through them, for now, provide
 *           an Alloc...() call with the same API that's just a wrapper.
 *
 * Args:     rows  - generally L+1, or 2; # of DP rows in seq dimension to alloc
 *           M     - size of model, in nodes
 *           xmx, mmx, imx, dmx 
 *                 - RETURN: ptrs to four mx components as a convenience
 *
 * Returns:  mx
 *           Caller free's w/ FreeDPMatrix()
 */
struct dpmatrix_s *
AllocDPMatrix(int rows, int M, int ***xmx, int ***mmx, int ***imx, int ***dmx)
{
  struct dpmatrix_s *mx;
  mx = CreateDPMatrix(rows-1, M, 0, 0);
  if (xmx != NULL) *xmx = mx->xmx;
  if (mmx != NULL) *mmx = mx->mmx;
  if (imx != NULL) *imx = mx->imx;
  if (dmx != NULL) *dmx = mx->dmx;
  return mx;
}


/* Function: FreeDPMatrix()
 *
 * Note:     Originally defined as FreePlan7Matrix() in core_algorithms.c,
 *           but was moved here to match the new architecture.
 *             - CRS 21 June 2005
 * 
 * Purpose:  Free a dynamic programming matrix allocated by CreateDPMatrix().
 * 
 * Return:   (void)
 */
void
FreeDPMatrix(struct dpmatrix_s *mx)
{
  /* 
   *Note: Same deal with the *_mem pointers as before.  - CRS 6 July 2005
   *
   free (mx->xmx_mem);
   free (mx->mmx_mem);
   free (mx->imx_mem);
   free (mx->dmx_mem);
   *
   */
  free (mx->xmx[0]);
  free (mx->mmx[0]);
  free (mx->imx[0]);
  free (mx->dmx[0]);
  free (mx->xmx);
  free (mx->mmx);
  free (mx->imx);
  free (mx->dmx);
  free (mx);
}
