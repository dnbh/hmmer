#include "structs.h"
#include "funcs.h"

inline void AllocLogoddsShell(struct plan7_s *hmm){
  hmm->lom = (struct logodds_s *) MallocOrDie(sizeof(struct logodds_s));
}

void AllocLogoddsBody(struct plan7_s *hmm){
  int x;
  int M = hmm->M;
  struct logodds_s *lom = hmm->lom;

  lom->tsc    = MallocOrDie (7     *           sizeof(int *));
  lom->msc    = MallocOrDie (MAXCODE   *       sizeof(int *));
  lom->isc    = MallocOrDie (MAXCODE   *       sizeof(int *)); 

  /* Allocate extra memory so tsc[TMM,TIM,TDM,TMD,TDD] start on the 
   * 16-byte cache boundary, and tsc[TMI,TII] start
   * 12 bytes offset from the boundary. 
   */
  lom->tsc_mem = MallocOrDie (((7*(M+16)))  *   sizeof(int));
  /* Allocate extra mem. to make sure all members of msc,isc start
   * on 12-byte offsets from cache boundary.
   */
  lom->msc_mem = MallocOrDie ((MAXCODE*(M+1+16)) * sizeof(int));
  lom->isc_mem = MallocOrDie ((MAXCODE*(M+16)) *   sizeof(int));

  /* align tsc pointers */
  lom->tsc[TMM] = (int *) (((((size_t) lom->tsc_mem) + 15) & (~0xf)));
  lom->tsc[TMI] = (int *) (((((size_t) lom->tsc_mem) + (M+12)*sizeof(int) + 15) & (~0xf)) + 12);
  lom->tsc[TMD] = (int *) (((((size_t) lom->tsc_mem) + 2*(M+12)*sizeof(int) + 15) & (~0xf)));
  lom->tsc[TIM] = (int *) (((((size_t) lom->tsc_mem) + 3*(M+12)*sizeof(int) + 15) & (~0xf)));
  lom->tsc[TII] = (int *) (((((size_t) lom->tsc_mem) + 4*(M+12)*sizeof(int) + 15) & (~0xf)) + 12);
  lom->tsc[TDM] = (int *) (((((size_t) lom->tsc_mem) + 5*(M+12)*sizeof(int) + 15) & (~0xf)));
  lom->tsc[TDD] = (int *) (((((size_t) lom->tsc_mem) + 6*(M+12)*sizeof(int) + 15) & (~0xf)));

  for (x = 0; x < MAXCODE; x++) {
    lom->msc[x] = (int *) (((((size_t)lom->msc_mem) + x*(M+1+12)*sizeof(int) + 15) & (~0xf)) + 12);
    lom->isc[x] = (int *) (((((size_t)lom->isc_mem) + x*(M+12)*sizeof(int) + 15) & (~0xf)) + 12);
  }
  /* tsc[0] is used as a boundary condition sometimes [Viterbi()],
   * so set to -inf always.
   */
  for (x = 0; x < 7; x++)
    lom->tsc[x][0] = -INFTY;

  lom->bsc_mem= MallocOrDie  ((M+1+12) * sizeof(int));
  lom->esc_mem= MallocOrDie  ((M+1+12) * sizeof(int));

  lom->bsc = (int *) (((((size_t) lom->bsc_mem) + 15) & (~0xf)) + 12);
  lom->esc = (int *) (((((size_t) lom->esc_mem) + 15) & (~0xf)) + 12);
  
  return;
}

void FreeLogodds(struct plan7_s *hmm){
  struct logodds_s *lom = hmm->lom;

  if (lom->tsc_mem != NULL) { free(lom->tsc_mem); lom->tsc_mem = NULL; }
  if (lom->msc_mem != NULL) { free(lom->msc_mem); lom->msc_mem = NULL; }
  if (lom->isc_mem != NULL) { free(lom->isc_mem); lom->isc_mem = NULL; }

  if (lom->tsc != NULL)     { free(lom->tsc);     lom->tsc = NULL; }
  if (lom->msc != NULL)     { free(lom->msc);     lom->msc = NULL; }
  if (lom->isc != NULL)     { free(lom->isc);     lom->isc = NULL; }
}

void FillP7Logodds(struct plan7_s *hmm){
  int x;

  /*
   * Note:  Here, we cheat a little bit.  The altivec version customizes
   *        the logodds structure so that the fields are aligned along
   *        the proper memory boundaries, but, beyond this, it doens't
   *        differ from the default logodds structure.  So, we reroute
   *        the default pointers (in p7lom) to the customized structure,
   *        and then call P7Logoddsify() to fill the fields in.
   *          - CRS 24 June 2005
   */
  hmm->p7lom->tsc = hmm->lom->tsc;
  hmm->p7lom->msc = hmm->lom->msc;
  hmm->p7lom->isc = hmm->lom->isc;

  /* align tsc pointers */
  hmm->p7lom->tsc[TMM] = hmm->lom->tsc[TMM];
  hmm->p7lom->tsc[TMI] = hmm->lom->tsc[TMI];
  hmm->p7lom->tsc[TMD] = hmm->lom->tsc[TMD];
  hmm->p7lom->tsc[TIM] = hmm->lom->tsc[TIM];
  hmm->p7lom->tsc[TII] = hmm->lom->tsc[TII];
  hmm->p7lom->tsc[TDM] = hmm->lom->tsc[TDM];
  hmm->p7lom->tsc[TDD] = hmm->lom->tsc[TDD];

  for (x = 0; x < MAXCODE; x++) {
    hmm->p7lom->msc[x] = hmm->lom->msc[x];
    hmm->p7lom->isc[x] = hmm->lom->isc[x];
  }
  /* tsc[0] is used as a boundary condition sometimes [Viterbi()],
   * so set to -inf always.
   */
  for (x = 0; x < 7; x++)
    hmm->p7lom->tsc[x][0] = -INFTY;

  hmm->p7lom->bsc = hmm->lom->bsc;
  hmm->p7lom->esc = hmm->lom->esc;

  P7Logoddsify(hmm);
}

void UnfillP7Logodds(struct plan7_s *hmm){
  /*
   * We don't have to free any memory, since we didn't allocate any
   * when we filled in the hmm->p7lom strucutre.  We should nullify
   * those pointers, though.
   */
  hmm->p7lom->tsc = NULL;
  hmm->p7lom->msc = NULL;
  hmm->p7lom->isc = NULL;
  hmm->p7lom->bsc = NULL;
  hmm->p7lom->esc = NULL;
}

/* Function: CreateDPMatrix()
 *
 * Note:     This was originally defined as CreatePlan7Matrix() in 
 *           fast_algorithms.c.  It was the Altivec-Specific 
 *           implementation of that function.  I moved it here, with the
 *           rest of the functions that are specific to the altivec
 *           data structures.  I also changed the name so that it was
 *           consistent with the new architecture.  - CRS 21 June 2005
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
 *           call how much to overallocate when it realloc's. If a pad
 *           is zero, we will not resize in that dimension.
 *           
 * Args:     N     - N+1 rows are allocated, for sequence.  
 *           M     - size of model in nodes
 *           padN  - over-realloc in seq/row dimension, or 0
 *           padM  - over-realloc in HMM/column dimension, or 0
 *                 
 * Return:   mx
 *           mx is allocated here. Caller frees with FreePlan7Matrix(mx).
 */
struct dpmatrix_s *
CreateDPMatrix(int N, int M, int padN, int padM)
{
  struct dpmatrix_s *mx;
  int i,n;

  mx         = (struct dpmatrix_s *) MallocOrDie (sizeof(struct dpmatrix_s));
  mx->xmx    = (int **) MallocOrDie (sizeof(int *) * (N+1));
  mx->mmx    = (int **) MallocOrDie (sizeof(int *) * (N+1));
  mx->imx    = (int **) MallocOrDie (sizeof(int *) * (N+1));
  mx->dmx    = (int **) MallocOrDie (sizeof(int *) * (N+1));

  /* For the memory accessed by the altivec routines, we want to have
   * accesses aligned to 16-byte boundaries as far as possible.
   * To accomplish this, we align extra memory and then set the first
   * pointer on each row to point to 4 bytes before a boundary.
   * This means element 1, which is the first one we work on, will be
   * on a 16-byte boundary. We still make sure we 'own' the three bytes
   * before, though, so we can load the vector with element 0 cache-aligned too.
   * The real pointers to memory are kept in xmx_mem,mmx_mem,imx_mem,dmx_mem.
   */
  mx->xmx_mem = (void *) MallocOrDie (sizeof(int) * (N+1)*(5 + 16));
  mx->mmx_mem = (void *) MallocOrDie (sizeof(int) * (N+1)*(M+2+16));
  mx->imx_mem = (void *) MallocOrDie (sizeof(int) * (N+1)*(M+2+16));
  mx->dmx_mem = (void *) MallocOrDie (sizeof(int) * (N+1)*(M+2+16));
  
  mx->xmx[0] = (int *) (((((size_t) mx->xmx_mem) + 15) & (~0xf)) + 12);
  mx->mmx[0] = (int *) (((((size_t) mx->mmx_mem) + 15) & (~0xf)) + 12);
  mx->imx[0] = (int *) (((((size_t) mx->imx_mem) + 15) & (~0xf)) + 12);
  mx->dmx[0] = (int *) (((((size_t) mx->dmx_mem) + 15) & (~0xf)) + 12);
  
  /* And make sure the beginning of each row is aligned the same way */
  for (i = 1; i <= N; i++)
    {
      mx->xmx[i] = mx->xmx[0] + i*(5+11) ; /* add 11 bytes per row, making it divisible by 4 */
      n = 12 - (M+2)%4;
      mx->mmx[i] = mx->mmx[0] + i*(M+2+n);
      mx->imx[i] = mx->imx[0] + i*(M+2+n);
      mx->dmx[i] = mx->dmx[0] + i*(M+2+n);
    }
   
  mx->workspace_mem = (int *) MallocOrDie (sizeof(int) * (M+4) * 12 + 16);
  mx->workspace     = (int *) ((((size_t) mx->workspace_mem) + 15) & (~0xf));

  mx->maxN = N;
  mx->maxM = M;
  mx->padN = padN;
  mx->padM = padM;
  
  return mx;
}

/* Function: ResizeDPMatrix()
 *
 * Note:     This was originally defined as ResizePlan7Matrix() in 
 *           fast_algorithms.c.  It was the Altivec-Specific 
 *           implementation of that function.  I moved it here, with the
 *           rest of the functions that are specific to the altivec
 *           data structures.  I also changed the name so that it was
 *           consistent with the new architecture.  - CRS 21 June 2005
 * 
 * Purpose:  Reallocate a dynamic programming matrix, if necessary,
 *           for a problem of NxM: sequence length N, model size M.
 *           (N=1 for small memory score-only variants; we allocate
 *           N+1 rows in the DP matrix.) 
 * 
 *           See additional comments in 
 *           core_algorithms.c:ResizePlan7Matrix(), the normal version
 *           of this function. This version is only used in the
 *           Altivec (--enable-altivec) port.
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
  int i,n;

  if (N <= mx->maxN && M <= mx->maxM) 
    {
      if (xmx != NULL) *xmx = mx->xmx;
      if (mmx != NULL) *mmx = mx->mmx;
      if (imx != NULL) *imx = mx->imx;
      if (dmx != NULL) *dmx = mx->dmx;
      return; 
    }

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

  mx->xmx_mem = ReallocOrDie (mx->xmx_mem, sizeof(int) * (mx->maxN+1)*(5 + 16));
  mx->mmx_mem = ReallocOrDie (mx->mmx_mem, sizeof(int) * (mx->maxN+1)*(mx->maxM+2+16));
  mx->imx_mem = ReallocOrDie (mx->imx_mem, sizeof(int) * (mx->maxN+1)*(mx->maxM+2+16));
  mx->dmx_mem = ReallocOrDie (mx->dmx_mem, sizeof(int) * (mx->maxN+1)*(mx->maxM+2+16));
  
  mx->xmx[0] = (int *) (((((size_t) mx->xmx_mem) + 15) & (~0xf)) + 12);
  mx->mmx[0] = (int *) (((((size_t) mx->mmx_mem) + 15) & (~0xf)) + 12);
  mx->imx[0] = (int *) (((((size_t) mx->imx_mem) + 15) & (~0xf)) + 12);
  mx->dmx[0] = (int *) (((((size_t) mx->dmx_mem) + 15) & (~0xf)) + 12);
  
  /* And make sure the beginning of each row is aligned the same way */
  for (i = 1; i <= mx->maxN; i++)
    {
      mx->xmx[i] = mx->xmx[0] + i*(5+11) ; /* add 11 bytes per row, making it divisible by 4 */
      n = 12 - (mx->maxM+2)%4;
      mx->mmx[i] = mx->mmx[0] + i*(mx->maxM+2+n);
      mx->imx[i] = mx->imx[0] + i*(mx->maxM+2+n);
      mx->dmx[i] = mx->dmx[0] + i*(mx->maxM+2+n);
    }
 
  if (xmx != NULL) *xmx = mx->xmx;
  if (mmx != NULL) *mmx = mx->mmx;
  if (imx != NULL) *imx = mx->imx;
  if (dmx != NULL) *dmx = mx->dmx;
}

/* Function: AllocDPMatrix()
 * Date:     SRE, Tue Nov 19 07:14:47 2002 [St. Louis]
 *
 * Note:     This was originally defined as AllocPlan7Matrix in 
 *           core_algorithms.c, which was then moved to AllocDPMatrix in
 *           defaultstructs.c.  The altivec version used the original
 *           version of this method however, so I also placed a copy
 *           here.
 *             - CRS 21 June 2005
 *
 * !!UNRESOLVED!! - Adding redundancy again.  Is there a better way to do
 *                  this?  - CRS 21 June 2005
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
 *           Caller free's w/ FreePlan7Matrix()
 */
struct dpmatrix_s *
AllocDPMatrix(int rows, int M, int ***xmx, int ***mmx, int ***imx, int ***dmx)
{
  struct dpmatrix_s *mx;
  mx = CreatePlan7Matrix(rows-1, M, 0, 0);
  if (xmx != NULL) *xmx = mx->xmx;
  if (mmx != NULL) *mmx = mx->mmx;
  if (imx != NULL) *imx = mx->imx;
  if (dmx != NULL) *dmx = mx->dmx;
  return mx;
}


/* Function: FreeDPMatrix()
 *
 * Note:     This was originally defined as FreePlan7Matrix in 
 *           core_algorithms.c, which was then moved to FreeDPMatrix in
 *           defaultstructs.c.  The altivec version used the original
 *           version of this method however, so I also placed a copy
 *           here.
 *             - CRS 21 June 2005
 *
 * !!UNRESOLVED!! - Adding redundancy again.  Is there a better way to do
 *                  this?  - CRS 21 June 2005
 * 
 * Purpose:  Free a dynamic programming matrix allocated by CreatePlan7Matrix().
 * 
 * Return:   (void)
 */
void
FreeDPMatrix(struct dpmatrix_s *mx)
{
  free (mx->xmx_mem);
  free (mx->mmx_mem);
  free (mx->imx_mem);
  free (mx->dmx_mem);
  free (mx->xmx);
  free (mx->mmx);
  free (mx->imx);
  free (mx->dmx);
  free (mx);
}
