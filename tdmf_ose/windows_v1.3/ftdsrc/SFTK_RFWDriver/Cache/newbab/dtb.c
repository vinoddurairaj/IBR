/*HDR************************************************************************
 *                                                                         
 * Softek -                                   
 *
 *===========================================================================
 *
 * N dtb.c
 * P Replicator 
 * S Database
 * V Generic
 * A J. Christatos - jchristatos@softek.fujitsu.com
 * D 03.08.2004
 * O Search Blocks functions for the cache
 * T Implementation of interval trees with in memory red/black binary trees.
 *  
 *   Ref. Chap 14 & 15 in 'Algorithms' Rivest & al. Addison-Wesley &
 *        'Art of programming' - Searching and sorting - vol. 3 - Knuth
 *
 *   Because of kernel space, recursion is forbidden.
 *   Comments are scarce so far, sorry, too complex to describe,
 *   read reference.
 *
 * C DBG - _KERNEL_
 * H 03.08.2004 - Creation - 
 *
 *===========================================================================
 *
 * rcsid[]="@(#) $Id: dtb.c,v 1.1 2004/05/29 00:45:29 vqba0 Exp $"
 *
 *HDR************************************************************************/

#include "common.h"

#ifdef   WIN32
#ifdef   _KERNEL
#include <ntddk.h>
#include "mmgr_ntkrnl.h"
#else          /* !_KERNEL*/
#include <windows.h>
#include <stdio.h>
#include "mmgr_ntusr.h"
#endif         /* _KERNEL  */
#endif         /* _WINDOWS */

#include "slab.h"
#include "mmg.h"
#include "dtb.h"


/*
 * Internal node root of RB database for each device
 * Lives in a Logical group.
 */
typedef struct _dtbroot_
{
	
	signatures_e      sigtype;
	OS_LIST_ENTRY     lnk;        // Next entry for this logical group.
	OS_LIST_ENTRY     db_root;    // The root of the database
	PVOID             DevicePtr;  // The key for this list

} dtbroot_t;

/*
 * Macro to check if node X overlapp range with node Y
 */
#define RPLCC_OVERLAPP(X, Y) \
							(X->IntervalLow  <= Y->IntervalHigh  && \
							 Y->IntervalLow  <= X->IntervalHigh )

/* private interface to the cache manager */
MMG_PUBLIC PVOID       RplCCAllocDbNode();
MMG_PUBLIC OS_NTSTATUS RplCCDelDbNode();


/*
 * BALANCED BINARY INTERVAL TREE USING RED/BACK TREE METHOD
 *===========================================================================
 *
 * All these functions assumes:
 *
 *  lnk.Blink : Left successor
 *  lnk.Flink : Right successor
 *
 *  All keys are distinct no duplicate.
 */

/*B**************************************************************************
 * _RplDtbRotateRight - 
 * 
 * Rebalance the subtree around right of p_node. O(1)
 * Warning: we can modify the root of the subtree.
 *
 *E==========================================================================
 */

MMG_PRIVATE
dtbnode_t *
_RplDtbRotateRight(dtbnode_t *p_subtree, dtbnode_t *p_node)

/**/
{
	dtbnode_t *p_newroot    = p_subtree;
	dtbnode_t *p_insertnode = NULL;
	dtbnode_t *p_temp       = NULL;
	dtbnode_t *p_left       = NULL;
	dtbnode_t *p_right      = NULL;

	ASSERT(p_subtree);
	ASSERT(p_node);
	ASSERT(p_subtree->sigtype == RPLCC_DTB);
	ASSERT(p_node->sigtype == RPLCC_DTB);

	MMGDEBUG(MMGDBG_LOW, ("Entering _RplDtbRotateRight \n"));

	try {

		p_insertnode = CONTAINING_RECORD(p_node->lnk.Blink, dtbnode_t, lnk);
		ASSERT(p_insertnode);
		ASSERT(p_insertnode->sigtype == RPLCC_DTB);
		p_node->lnk.Blink = p_insertnode->lnk.Flink;
		if (p_insertnode->lnk.Flink != NULL)
		{
			p_temp = CONTAINING_RECORD(p_insertnode->lnk.Flink, dtbnode_t, lnk);
			ASSERT(p_temp);
			ASSERT(p_temp->sigtype == RPLCC_DTB);
			p_temp->p_parent = p_node;
		}
		p_insertnode->p_parent = p_node->p_parent;
		if (p_node->p_parent == NULL)
		{
			p_newroot = p_insertnode;
		} else
		{
			if (p_node->p_parent->lnk.Flink == &p_node->lnk)
			{
				p_node->p_parent->lnk.Flink	= &p_insertnode->lnk;
			} else 
			{
				p_node->p_parent->lnk.Blink	= &p_insertnode->lnk;
			}
		}

		p_insertnode->lnk.Flink = &p_node->lnk;
		p_node->p_parent = p_insertnode;

		/*
		 * recompute max values of both nodes 
		 */
		p_insertnode->MaximumHigh  = p_insertnode->IntervalHigh;

		if (p_insertnode->lnk.Blink != NULL)
		{
			p_left = CONTAINING_RECORD(p_insertnode->lnk.Blink, dtbnode_t, lnk);
			ASSERT(p_left->sigtype == RPLCC_DTB);
			if (p_left->MaximumHigh  > p_insertnode->MaximumHigh )
			{
				p_insertnode->MaximumHigh  = p_left->MaximumHigh;
			}
		}

		if (p_insertnode->lnk.Flink != NULL)
		{
			p_right = CONTAINING_RECORD(p_insertnode->lnk.Flink, dtbnode_t, lnk);
			ASSERT(p_right->sigtype == RPLCC_DTB);
			if (p_right->MaximumHigh  > p_insertnode->MaximumHigh )
			{
				p_insertnode->MaximumHigh  = p_right->MaximumHigh;
			}
		}

		p_node->MaximumHigh  = p_node->IntervalHigh;
		if (p_node->lnk.Blink != NULL)
		{
			p_left = CONTAINING_RECORD(p_node->lnk.Blink, dtbnode_t, lnk);
			ASSERT(p_left->sigtype == RPLCC_DTB);
			if (p_left->MaximumHigh  > p_node->MaximumHigh )
			{
				p_node->MaximumHigh  = p_left->MaximumHigh;
			}
		}

		if (p_node->lnk.Flink != NULL)
		{
			p_right = CONTAINING_RECORD(p_node->lnk.Flink, dtbnode_t, lnk);
			ASSERT(p_right->sigtype == RPLCC_DTB);
			if (p_right->MaximumHigh  > p_node->MaximumHigh )
			{
				p_node->MaximumHigh  = p_right->MaximumHigh;
			}
		}
		

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving _RplDtbRotateRight \n"));

return(p_newroot);
} /* _RplDtbRotateRight */

/*B**************************************************************************
 * _RplDtbRotateLeft - 
 * 
 * Rebalance the subtree around right of p_node.O(1)
 * Warning: we can modify the root of the subtree.
 *
 * Assume that p_node->lnk.Flink == &p_node->lnk (Nil)
 *E==========================================================================
 */

MMG_PRIVATE
dtbnode_t *
_RplDtbRotateLeft(dtbnode_t *p_subtree, dtbnode_t *p_node)

/**/
{
	dtbnode_t *p_newroot     = p_subtree;
	dtbnode_t *p_insertnode  = NULL;
	dtbnode_t *p_temp        = NULL;
	dtbnode_t *p_left        = NULL;
	dtbnode_t *p_right       = NULL;


	ASSERT(p_subtree);
	ASSERT(p_node);
	ASSERT(p_subtree->sigtype == RPLCC_DTB);
	ASSERT(p_node->sigtype == RPLCC_DTB);

	MMGDEBUG(MMGDBG_LOW, ("Entering _RplDtbRotateLeft \n"));

	try {
		
		p_insertnode = CONTAINING_RECORD(p_node->lnk.Flink, dtbnode_t, lnk);
		ASSERT(p_insertnode);
		ASSERT(p_insertnode->sigtype == RPLCC_DTB);
		p_node->lnk.Flink = p_insertnode->lnk.Blink;
		if (p_insertnode->lnk.Blink != NULL)
		{
			p_temp = CONTAINING_RECORD(p_insertnode->lnk.Blink, dtbnode_t, lnk);
			ASSERT(p_temp);
			ASSERT(p_temp->sigtype == RPLCC_DTB);
			p_temp->p_parent = p_node;
		}
		p_insertnode->p_parent = p_node->p_parent;
		if (p_node->p_parent == NULL)
		{
			p_newroot = p_insertnode;
		} else
		{
			if (p_node->p_parent->lnk.Blink == &p_node->lnk)
			{
				p_node->p_parent->lnk.Blink	= &p_insertnode->lnk;
			} else 
			{
				p_node->p_parent->lnk.Flink	= &p_insertnode->lnk;
			}
		}

		p_insertnode->lnk.Blink = &p_node->lnk;
		p_node->p_parent = p_insertnode;
			
		/*
		 * recompute max values of both nodes 
		 */
		p_insertnode->MaximumHigh  = p_insertnode->IntervalHigh;

		if (p_insertnode->lnk.Blink != NULL)
		{
			p_left = CONTAINING_RECORD(p_insertnode->lnk.Blink, dtbnode_t, lnk);
			ASSERT(p_left->sigtype == RPLCC_DTB);
			if (p_left->MaximumHigh  > p_insertnode->MaximumHigh )
			{
				p_insertnode->MaximumHigh  = p_left->MaximumHigh;
			}
		}

		if (p_insertnode->lnk.Flink != NULL)
		{
			p_right = CONTAINING_RECORD(p_insertnode->lnk.Flink, dtbnode_t, lnk);
			ASSERT(p_right->sigtype == RPLCC_DTB);
			if (p_right->MaximumHigh  > p_insertnode->MaximumHigh )
			{
				p_insertnode->MaximumHigh  = p_right->MaximumHigh;
			}
		}

		p_node->MaximumHigh  = p_node->IntervalHigh;
		if (p_node->lnk.Blink != NULL)
		{
			p_left = CONTAINING_RECORD(p_node->lnk.Blink, dtbnode_t, lnk);
			ASSERT(p_left->sigtype == RPLCC_DTB);
			if (p_left->MaximumHigh  > p_node->MaximumHigh )
			{
				p_node->MaximumHigh  = p_left->MaximumHigh;
			}
		}

		if (p_node->lnk.Flink != NULL)
		{
			p_right = CONTAINING_RECORD(p_node->lnk.Flink, dtbnode_t, lnk);
			ASSERT(p_right->sigtype == RPLCC_DTB);
			if (p_right->MaximumHigh  > p_node->MaximumHigh )
			{
				p_node->MaximumHigh  = p_right->MaximumHigh;
			}
		}

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving _RplDtbRotateLeft \n"));

return(p_newroot);
} /* _RplDtbRotateLeft */

/*B**************************************************************************
 * _RplDtbBinaryInsert - 
 * 
 * Insert a new node in the database using binary tree method.
 * no balancing here.
 *
 * Assume a leaf is detected by the Nil condition 
 * 
 *E==========================================================================
 */

MMG_PRIVATE
VOID
_RplDtbBinaryInsert(dtbnode_t *p_root, dtbnode_t *p_node)

/**/
{	
	OS_LIST_ENTRY *p_templnk;
	dtbnode_t     *p_insertnode  = NULL;
	dtbnode_t     *p_currentnode = NULL;

	ASSERT(p_root);
	ASSERT(p_node);
	ASSERT(p_root->sigtype == RPLCC_DTB);
	ASSERT(p_node->sigtype == RPLCC_DTB);

	MMGDEBUG(MMGDBG_LOW, ("Entering _RplDtbBinaryInsert \n"));
	
	try {

		p_currentnode = p_root;
		p_insertnode  = p_currentnode;
		while (p_currentnode != NULL)
		{
			ASSERT(p_currentnode->sigtype == RPLCC_DTB);
			p_insertnode = p_currentnode;
			p_templnk = 
				(p_node->IntervalLow  < p_currentnode->IntervalLow )?
					p_currentnode->lnk.Blink : p_currentnode->lnk.Flink;

			p_currentnode = NULL;
			if (p_templnk) 
			{
				p_currentnode = CONTAINING_RECORD(p_templnk, dtbnode_t, lnk);		
			}
		} /* while */

		p_node->p_parent = p_insertnode;
		p_node->MaximumHigh  = p_node->IntervalHigh;
		
		if (p_node->IntervalLow  < p_insertnode->IntervalLow )
		{
			p_insertnode->lnk.Blink = &p_node->lnk;
		} else 
		{
			p_insertnode->lnk.Flink = &p_node->lnk;
		}

		p_currentnode = p_node;
		while (p_currentnode != NULL && p_currentnode->p_parent != NULL)
		{
			if (p_currentnode->MaximumHigh  > p_currentnode->p_parent->MaximumHigh )
			{
				p_currentnode->p_parent->MaximumHigh  = p_currentnode->MaximumHigh;
				p_currentnode = p_currentnode->p_parent;
			} else 
			{
				p_currentnode = NULL;
				break;
			}
		}

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving _RplDtbBinaryInsert \n"));

} /* _RplDtbBinaryInsert */

/*B**************************************************************************
 * _RplDtbRBInsert - 
 * 
 * Insert a new node in the database using Red/Black tree rebalancing. O(lg n)
 *
 * Assume a leaf is detected by the Nil condition 
 * 
 *E==========================================================================
 */

MMG_PRIVATE
OS_NTSTATUS
_RplDtbRBInsert(OS_LIST_ENTRY *p_rootlnk, dtbnode_t *p_node)

/**/
{
	OS_NTSTATUS ret = STATUS_SUCCESS;
	dtbnode_t   *p_root        = NULL;
	dtbnode_t   *p_insertnode  = NULL;
	
	int o;

	ASSERT(p_node);
	MMGDEBUG(MMGDBG_LOW, ("Entering _RplDtbRBInsert \n"));
	o = sizeof(dtbnode_t);
	try {

		/*
	     * Special degenerate case where root is empty
		 */
		if (p_rootlnk->Flink == p_rootlnk)
		{
			p_rootlnk->Flink = &p_node->lnk;
			p_rootlnk->Blink = &p_node->lnk;
			p_node->MaximumHigh  = 0; 
			p_node->p_parent = NULL;
			p_node->lnk.Blink =
			p_node->lnk.Flink = NULL;
			p_node->color = BLACK;
			leave;
			/* NOTREACHED */
		}

		p_root = CONTAINING_RECORD(p_rootlnk->Flink, dtbnode_t, lnk);
		ASSERT(p_root->sigtype == RPLCC_DTB);

		_RplDtbBinaryInsert(p_root, p_node);
		p_node->color = RED;
		while (p_node != p_root &&
			   p_node->p_parent != NULL &&
			   p_node->p_parent->color == RED)
		{
			p_insertnode = NULL;
			if (p_node->p_parent->p_parent->lnk.Blink == &p_node->p_parent->lnk)
			{
				if (p_node->p_parent->p_parent->lnk.Flink != NULL)
				{
					p_insertnode = CONTAINING_RECORD(p_node->p_parent->p_parent->lnk.Flink, dtbnode_t, lnk);
		
					ASSERT(p_insertnode);
					ASSERT(p_insertnode->sigtype == RPLCC_DTB);
				}
				if (p_insertnode != NULL && p_insertnode->color == RED)
				{
					MMGDEBUG(MMGDBG_LOW, ("case 1.1 \n"));
					ASSERT(p_node->p_parent);
					ASSERT(p_node->p_parent->p_parent);
					p_node->p_parent->color = BLACK;
					p_insertnode->color     = BLACK;
					p_node->p_parent->p_parent->color = RED;
					p_node = p_node->p_parent->p_parent;
				} else
				{
					ASSERT(p_node->p_parent);
					if (p_node->p_parent->lnk.Flink == &p_node->lnk)
					{
						MMGDEBUG(MMGDBG_LOW, ("case 1.2 \n"));
						p_node = p_node->p_parent;
						p_root = _RplDtbRotateLeft(p_root, p_node);
					} 
					MMGDEBUG(MMGDBG_LOW, ("case 1.3 \n"));
					ASSERT(p_node->p_parent->p_parent);
					p_node->p_parent->color = BLACK;
					p_node->p_parent->p_parent->color = RED;
					p_root = _RplDtbRotateRight(p_root, p_node->p_parent->p_parent);
				}
			} else 
			{
				if (!(p_node->p_parent->p_parent->lnk.Flink == &p_node->p_parent->lnk))
				{
					break;
				}
				if (p_node->p_parent->p_parent->lnk.Blink != NULL)
				{
					p_insertnode = CONTAINING_RECORD(p_node->p_parent->p_parent->lnk.Blink, dtbnode_t, lnk);
					ASSERT(p_insertnode);
					ASSERT(p_insertnode->sigtype == RPLCC_DTB);
				}
				if (p_insertnode != NULL && p_insertnode->color == RED)
				{
					MMGDEBUG(MMGDBG_LOW, ("case 2.1 \n"));
					ASSERT(p_node->p_parent);
					ASSERT(p_node->p_parent->p_parent);
					p_node->p_parent->color = BLACK;
					p_insertnode->color     = BLACK;
					p_node->p_parent->p_parent->color = RED;
					p_node = p_node->p_parent->p_parent;
				} else
				{
					ASSERT(p_node->p_parent);
					if (p_node->p_parent->lnk.Blink == &p_node->lnk)
					{
						MMGDEBUG(MMGDBG_LOW, ("case 2.2 \n"));
						p_node = p_node->p_parent;
						p_root = _RplDtbRotateRight(p_root, p_node);
					} 
					MMGDEBUG(MMGDBG_LOW, ("case 2.3 \n"));
					ASSERT(p_node->p_parent->p_parent);
					p_node->p_parent->color = BLACK;
					p_node->p_parent->p_parent->color = RED;
					p_root = _RplDtbRotateLeft(p_root, p_node->p_parent->p_parent);
				}
			}
		} /* while */

		/*
		 * The root node may have changed 
		 * due to rebalancing.
		 */
		p_root->color = BLACK;
		p_rootlnk->Flink = &p_root->lnk;
		p_rootlnk->Blink = &p_root->lnk;

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving _RplDtbRBInsert \n"));

return(ret);
} /* _RplDtbRBInsert */

/*B**************************************************************************
 * _RplDtbRBSearch - 
 * 
 * Search if interval in p_hdr intercept nodes in the tree.
 * 
 * For the interval to overlap the condition is:
 * p_hdr->IntervalLow <= p_treenode->IntervalHigh &&
 * p_hdr->IntervalHigh > p_treenode->IntervalLow 
 * 
 *E==========================================================================
 */

MMG_PUBLIC
BOOLEAN
_RplDtbRBSearch(OS_LIST_ENTRY *p_rootlnk, 
				dtbnode_t     *p_searchnode, 
				dtbnode_t     **p_result)

/**/
{
	BOOLEAN     ret            = TRUE;
	dtbnode_t   *p_currentnode = NULL;
	dtbnode_t   *p_root        = NULL;
	
	MMGDEBUG(MMGDBG_LOW, ("Entering _RplDtbRBSearch \n"));
	
	try {

		p_root = CONTAINING_RECORD(p_rootlnk->Flink, dtbnode_t, lnk);
		ASSERT(p_root->sigtype == RPLCC_DTB);

		p_currentnode = p_root;
		while (p_currentnode != NULL && !RPLCC_OVERLAPP(p_searchnode, p_currentnode))
		{
			ASSERT(p_currentnode->sigtype == RPLCC_DTB);
			if (p_currentnode->MaximumHigh  >= p_searchnode->IntervalLow)
			{
				p_currentnode = (p_currentnode->lnk.Blink != NULL)?
					CONTAINING_RECORD(p_currentnode->lnk.Blink, dtbnode_t, lnk):
					NULL;
			} else 
			{
				p_currentnode = (p_currentnode->lnk.Flink != NULL)?
					CONTAINING_RECORD(p_currentnode->lnk.Flink, dtbnode_t, lnk):
					NULL;
			}
		} /* while */

		*p_result = p_currentnode;
		if (p_currentnode == NULL)
		{
			ret = FALSE;
		}

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving _RplDtbRBSearch \n"));

return(ret);
} /* _RplDtbRBSearch */

/*B**************************************************************************
 * _RplDtbTreeSuccessor - 
 * 
 * Find p_node successor in the sorted order in the tree.
 *
 *E==========================================================================
 */
MMG_PRIVATE
dtbnode_t *
_RplDtbTreeSuccessor(dtbnode_t *p_node)

/**/
{
	dtbnode_t      *p_currentnode = NULL;
	OS_LIST_ENTRY  *p_lnk = NULL;
	dtbnode_t      *p_tmp = NULL;

	MMGDEBUG(MMGDBG_LOW, ("Entering _RplDtbTreeSuccessor \n"));
	
	try {

		if (p_node->lnk.Flink != NULL)
		{
			/*
			 * find the minimum node (bottom left)
			 */
			p_lnk = p_node->lnk.Flink;
			while (p_lnk != NULL)
			{
				p_currentnode = CONTAINING_RECORD(p_lnk, dtbnode_t, lnk);
				ASSERT(p_currentnode->sigtype == RPLCC_DTB);
				p_lnk = p_currentnode->lnk.Blink;
			}

			/* p_currentnode is our result */
			leave;
		}

		p_currentnode = p_node->p_parent;
        p_tmp = p_node;
		while (p_currentnode != NULL &&
			   p_currentnode->lnk.Flink == &p_tmp->lnk)
		{
			p_tmp = p_currentnode;
			p_currentnode = p_currentnode->p_parent;
		}

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving _RplDtbTreeSuccessor \n"));

return(p_currentnode);
} /* _RplDtbTreeSuccessor */

/*B**************************************************************************
 * _RplDtbRebalance - 
 * 
 * Rebalance tree p_rootlnk around p_node, after a node deletion.
 *
 *E==========================================================================
 */

MMG_PRIVATE
OS_NTSTATUS
_RplDtbRebalance(OS_LIST_ENTRY *p_rootlnk, dtbnode_t *p_node)

/**/
{
	OS_NTSTATUS   ret      = STATUS_SUCCESS;
	dtbnode_t     *p_root  = NULL;
	dtbnode_t     *p_tmp   = NULL;
	dtbnode_t     *p_left  = NULL;
	dtbnode_t     *p_right = NULL;
	

	MMGDEBUG(MMGDBG_LOW, ("Entering _RplDtbRebalance \n"));
	
	try {
		p_root = CONTAINING_RECORD(p_rootlnk->Flink, dtbnode_t, lnk);
		ASSERT(p_root->sigtype == RPLCC_DTB);

		while (p_node != p_root && p_node->color == BLACK)
		{
			if (&p_node->lnk == p_node->p_parent->lnk.Blink)
			{
				if (p_node->p_parent->lnk.Flink != NULL)
				{
					p_tmp = CONTAINING_RECORD(p_node->p_parent->lnk.Flink, dtbnode_t, lnk);
					ASSERT(p_tmp->sigtype == RPLCC_DTB);
					if (p_tmp->color == RED)
					{
						MMGDEBUG(MMGDBG_LOW, ("case 1.1 \n"));
						p_tmp->color = BLACK;
						p_node->p_parent->color = RED;
						p_root = _RplDtbRotateLeft(p_root, p_node->p_parent);
						p_tmp = CONTAINING_RECORD(p_node->p_parent->lnk.Flink, dtbnode_t, lnk);
					    ASSERT(p_tmp->sigtype == RPLCC_DTB);
					}
					if (p_tmp->p_parent->lnk.Blink != NULL)
					{
						p_left = CONTAINING_RECORD(p_tmp->p_parent->lnk.Blink, dtbnode_t, lnk);
						ASSERT(p_left->sigtype == RPLCC_DTB);
					}
					if (p_tmp->p_parent->lnk.Flink != NULL)
					{
						p_right = CONTAINING_RECORD(p_tmp->p_parent->lnk.Flink, dtbnode_t, lnk);
						ASSERT(p_right->sigtype == RPLCC_DTB);
					}
					if (p_left != NULL && p_right != NULL &&
						p_left->color == BLACK && p_right->color == BLACK)
					{
						MMGDEBUG(MMGDBG_LOW, ("case 1.2 \n"));
						p_tmp->color = RED;
						p_node = p_node->p_parent;
					} else 
					{
						if (p_right->color == BLACK)
						{
							MMGDEBUG(MMGDBG_LOW, ("case 1.3 \n"));
							p_left->color = BLACK;
							p_tmp->color  = RED;
							p_root = _RplDtbRotateRight(p_root, p_tmp);
							p_tmp  = p_right; 
						}
						MMGDEBUG(MMGDBG_LOW, ("case 1.4 \n"));
						p_tmp->color = p_node->p_parent->color;
						p_node->p_parent->color = BLACK;
						p_right->color          = BLACK;
						p_root = _RplDtbRotateLeft(p_root, p_node->p_parent);
						p_node = p_root;
					}
				}					
			} else 
			{
				if (p_node->p_parent->lnk.Blink != NULL)
				{
					p_tmp = CONTAINING_RECORD(p_node->p_parent->lnk.Blink, dtbnode_t, lnk);
					ASSERT(p_tmp->sigtype == RPLCC_DTB);
					if (p_tmp->color == RED)
					{
						MMGDEBUG(MMGDBG_LOW, ("case 2.1 \n"));
						p_tmp->color = BLACK;
						p_node->p_parent->color = RED;
						p_root = _RplDtbRotateRight(p_root, p_node->p_parent);
						p_tmp = CONTAINING_RECORD(p_node->p_parent->lnk.Blink, dtbnode_t, lnk);
					    ASSERT(p_tmp->sigtype == RPLCC_DTB);
					}
					if (p_tmp->p_parent->lnk.Flink != NULL)
					{
						p_right = CONTAINING_RECORD(p_tmp->p_parent->lnk.Flink, dtbnode_t, lnk);
						ASSERT(p_right->sigtype == RPLCC_DTB);
					}
					if (p_tmp->p_parent->lnk.Blink != NULL)
					{
						p_left = CONTAINING_RECORD(p_tmp->p_parent->lnk.Blink, dtbnode_t, lnk);
						ASSERT(p_left->sigtype == RPLCC_DTB);
					}
					if (p_left != NULL && p_right != NULL &&
						p_left->color == BLACK && p_right->color == BLACK)
					{
						MMGDEBUG(MMGDBG_LOW, ("case 2.2 \n"));
						p_tmp->color = RED;
						p_node = p_node->p_parent;
					} else 
					{
						if (p_left->color == BLACK)
						{
							MMGDEBUG(MMGDBG_LOW, ("case 2.3 \n"));
							p_right->color = BLACK;
							p_tmp->color  = RED;
							p_root = _RplDtbRotateLeft(p_root, p_tmp);
							p_tmp  = p_left; 
						}
						MMGDEBUG(MMGDBG_LOW, ("case 2.4 \n"));
						p_tmp->color = p_node->p_parent->color;
						p_node->p_parent->color = BLACK;
						p_left->color          = BLACK;
						p_root = _RplDtbRotateRight(p_root, p_node->p_parent);
						p_node = p_root;
					}
				}
			}
		} /* while */

		p_node->color = BLACK;

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Entering _RplDtbRebalance \n"));

return(ret);
} /* _RplDtbRebalance */

/*B**************************************************************************
 * _RplDtbRBDelete - 
 * 
 * Delete a node from a Red/Black Tree and rebalance
 *
 *E==========================================================================
 */

MMG_PRIVATE
OS_NTSTATUS
_RplDtbRBDelete(OS_LIST_ENTRY *p_rootlnk, dtbnode_t *p_node)

/**/
{
	OS_NTSTATUS    ret = STATUS_SUCCESS;
	OS_LIST_ENTRY  *p_curlnk      = NULL;
	dtbnode_t      *p_deletenode  = NULL;
	dtbnode_t      *p_currentnode = NULL;


	ASSERT(p_node);
	ASSERT(p_node->sigtype == RPLCC_DTB);
	MMGDEBUG(MMGDBG_LOW, ("Entering _RplDtbRBDelete \n"));
	
	try {

		p_deletenode = (p_node->lnk.Blink == NULL || p_node->lnk.Flink == NULL)?
			           p_node:
		               _RplDtbTreeSuccessor(p_node);
			
		p_curlnk = (p_node->lnk.Blink != NULL) ?
			        p_deletenode->lnk.Blink:
		            p_deletenode->lnk.Flink;

		if (p_curlnk != NULL)
		{
			p_currentnode = CONTAINING_RECORD(p_curlnk, dtbnode_t, lnk);
			ASSERT(p_currentnode->sigtype == RPLCC_DTB);
			ASSERT(p_deletenode);
			p_currentnode->p_parent = p_deletenode->p_parent;
		}
		
		/*
		 * If we need to change the root ?
		 */
		if (p_deletenode->p_parent == NULL)
		{
			p_rootlnk->Flink = (p_currentnode != NULL)?
								&p_currentnode->lnk:
								p_rootlnk;
			p_rootlnk->Blink = (p_currentnode != NULL)?
				                &p_currentnode->lnk:
			                    p_rootlnk;

		} else 
		{
			if (p_deletenode->p_parent->lnk.Blink != NULL &&
				p_deletenode->p_parent->lnk.Blink == &p_deletenode->lnk)
			{
				p_deletenode->p_parent->lnk.Blink = 
					(p_currentnode == NULL)? NULL:&p_currentnode->lnk;
			} else 
			{
				p_deletenode->p_parent->lnk.Flink = 
					(p_currentnode == NULL)? NULL:&p_currentnode->lnk;
			}
		}

		if (p_deletenode != p_node)
		{
			 p_deletenode->lnk      = p_node->lnk;
			 p_deletenode->p_parent = p_node->p_parent;
		}

		/*
		 * Do we need to rebalance now ?
		 */
		if (p_deletenode->color == BLACK && p_currentnode != NULL)
		{
			_RplDtbRebalance(p_rootlnk, p_currentnode);
		}

	} finally {
	
	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving _RplDtbRBDelete \n"));

return(ret);
} /* _RplDtbRBDelete */

/* FRONT END FUNCTIONS */

/*B**************************************************************************
 * RplCCDtbInit - 
 * 
 * Initialize Block IO search engine per device.
 * ASSUME : Logical group is locked.
 *
 * 
 *E==========================================================================
 */

MMG_PUBLIC
OS_NTSTATUS
RplCCDtbInit(OS_LIST_ENTRY rootnode_lnk)

/**/
{
	OS_NTSTATUS ret     = STATUS_SUCCESS;
	dtbnode_t   *p_root = NULL;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCDtbInit \n"));

	try {
#ifdef NOP
		/*
		 * Allocate a database node from cache lookaside list
		 */
		p_root = RplCCAllocDbNode();
		if (p_root == NULL)
		{
			MMGDEBUG(MMGDBG_FATAL, ("Cannot allocate database root node from lookaside list \n"));
			ret = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		/*
		 * Populate the node 
		 */
		p_root->sigtype = RPLCC_DTB;
		p_root->IntervalLow  = 0;

		/*
		 * Insert ourself in the LG link
		 */
		InsertTailList(&rootnode_lnk, &p_root->lnk);
#endif
	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCDtbInit \n"));

return(ret);
} /* RplCCDtbInit */

/*B**************************************************************************
 * RplCCDtbDel - 
 * 
 * Release a Block IO search engine per device.
 *
 * 
 *E==========================================================================
 */

MMG_PUBLIC
OS_NTSTATUS
RplCCDtbDel(OS_LIST_ENTRY rootnode_lnk)

/**/
{
	OS_NTSTATUS ret     = STATUS_SUCCESS;
	dtbnode_t   *p_root = NULL;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplCCDtbInit \n"));

	try {

		/*
		 * Go through all the nodes (must be 0)
		 * and remove them
		 * TBD...
		 */

		/*
		 * Now remove root node
		 */
		p_root = CONTAINING_RECORD(&rootnode_lnk, dtbnode_t, lnk);
		ASSERT(p_root->sigtype == RPLCC_DTB);

		/*
		 * Allocate a database node from cache lookaside list
		 */
		RplCCDelDbNode(p_root);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplCCDtbInit \n"));

return(ret);
}	/* RplCCDtbDel */

/*B**************************************************************************
 * IsValidDtbNode - 
 * 
 * Check if this is a valid database root node
 * 
 *E==========================================================================
 */

MMG_PUBLIC
BOOLEAN
IsValidDtbNode(PVOID p_node)

/**/
{
	BOOLEAN     ret     = TRUE;

	ASSERT(p_node);
	MMGDEBUG(MMGDBG_LOW, ("Entering IsValidDtbNode \n"));

	if (((dtbroot_t *)p_node)->sigtype != RPLCC_DBROOT)
	{
		ret = FALSE;
	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving IsValidDtbNode \n"));

return(ret);
} /* IsValidDtbNode */

/*B**************************************************************************
 * SizeOfDtbNode - 
 * 
 * Return the size of a root database node
 * 
 *E==========================================================================
 */

MMG_PUBLIC
SIZE_T
SizeOfDtbNode()

/**/
{
	SIZE_T     ret;
	MMGDEBUG(MMGDBG_LOW, ("Entering SizeOfDtbNode \n"));

	ret = sizeof(dtbroot_t);

	MMGDEBUG(MMGDBG_LOW, ("Leaving SizeOfDtbNode \n"));

return(ret);
} /* SizeOfDtbNode */

/*B**************************************************************************
 * RplDtbInsertNode - 
 * 
 * Insert a new node in the database 
 * front end interface. Search the correct database from Device ID.
 * ASSUME : Logical group caller is locked.
 *
 * 
 *E==========================================================================
 */

MMG_PUBLIC
OS_NTSTATUS
RplDtbInsertNode(OS_LIST_ENTRY *p_rootlnk, dtbnode_t *p_node)

/**/
{
	OS_NTSTATUS    ret = STATUS_SUCCESS;
	OS_LIST_ENTRY  *p_dtbroot_lnk = NULL;
	dtbroot_t      *p_dtbroot     = NULL;
	BOOLEAN        NeedNewNode    = TRUE;

	ASSERT(p_rootlnk);
	ASSERT(p_node);
	ASSERT(p_node->sigtype == RPLCC_DTB);

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplDtbInsertNode \n"));

	try {

		/*
		 * if we have a valid linked list of databases for
		 * this logical group search for the device ID.
		 */
		
		if (IsListEmpty(p_rootlnk)==FALSE)
		{
			p_dtbroot_lnk = p_rootlnk->Flink;
			while (p_dtbroot_lnk != p_rootlnk)
			{
				p_dtbroot = CONTAINING_RECORD(p_dtbroot_lnk, dtbroot_t, lnk);
				ASSERT(p_dtbroot->sigtype == RPLCC_DBROOT);
				if (p_dtbroot->DevicePtr  == p_node->DeviceId)
				{
					/*
					 * We found a DeviceID will use
					 * the database pointed by p_dtbroot
					 */
					NeedNewNode = FALSE;
					break;
				}
				p_dtbroot_lnk = p_dtbroot_lnk->Flink;
			}
		}

		if (NeedNewNode == TRUE)
		{
			/*
			 * DeviceID not found.
			 * Just allocate a new database root
			 */
		
			p_dtbroot = (dtbroot_t *)RplCCAllocDbNode();
			if (p_dtbroot == NULL)
			{
				MMGDEBUG(MMGDBG_LOW, ("Entering RplCCAllocateSpecialPool \n"));
				ret = STATUS_INSUFFICIENT_RESOURCES;
				leave;
			}

			p_dtbroot->sigtype    = RPLCC_DBROOT;
			p_dtbroot->DevicePtr  = p_node->DeviceId;
			InitializeListHead(&p_dtbroot->lnk);
			InitializeListHead(&p_dtbroot->db_root);
			InsertTailList(p_rootlnk, &p_dtbroot->lnk);
		}

		/*
		 * 'normalize' the node - create the interval.
		 */
		p_node->IntervalLow   = p_node->Blk_Start;
		p_node->IntervalHigh  = p_node->IntervalLow  + p_node->Size;
		p_node->MaximumHigh   = 0;

		/*
		 * Insert the node in the matching database.
		 */
		ret = _RplDtbRBInsert(&p_dtbroot->db_root, p_node);

	} finally {

		if (!OS_NT_SUCCESS(ret))
		{
			if (NeedNewNode == TRUE && p_dtbroot != NULL)
			{
				/* 
				 * On error we remove ourselve from 
				 * database list and free
				 */
				RemoveEntryList(&p_dtbroot->lnk);
				RplCCDelDbNode((PVOID) p_dtbroot);
			}
		}

	}

return(ret);
} /* RplDtbInsertNode */


/*B**************************************************************************
 * RplDtbDeleteNode - 
 * 
 * Remove a node from the database
 * ASSUME p_hdr is a legitimate database formatted node.
 * 
 *E==========================================================================
 */

MMG_PUBLIC
OS_NTSTATUS
RplDtbDeleteNode(OS_LIST_ENTRY *p_rootlnk, dtbnode_t *p_node)

/**/
{
	OS_NTSTATUS    ret            = STATUS_SUCCESS;
	dtbroot_t      *p_dtbroot     = NULL;
	OS_LIST_ENTRY  *p_dtbroot_lnk = NULL;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplDtbDeleteNode \n"));
	
	try {

		/*
		 * If we have a valid linked list of databases for
		 * this logical group search for the device ID.
		 */
		
		if (IsListEmpty(p_rootlnk)==FALSE)
		{
			p_dtbroot_lnk = p_rootlnk->Flink;
			while (p_dtbroot_lnk != p_rootlnk)
			{
				p_dtbroot = CONTAINING_RECORD(p_dtbroot_lnk, dtbroot_t, lnk);
				ASSERT(p_dtbroot->sigtype == RPLCC_DBROOT);
				if (p_dtbroot->DevicePtr  == p_node->DeviceId)
				{
					/* delete the node and rebalance the tree */
					ret = _RplDtbRBDelete(&p_dtbroot->db_root, p_node);

					if (p_dtbroot->db_root.Flink == &p_dtbroot->db_root)
					{
						/*
						 * The tree has been deleted 
						 */
						MMGDEBUG(MMGDBG_LOW, ("Search Tree Deleted \n"));
						RemoveEntryList(&p_dtbroot->lnk);
						RplCCDelDbNode(p_dtbroot);
					}
					break;
					/* NOTREACHED we're done */
				}
				p_dtbroot_lnk = p_dtbroot_lnk->Flink;
			}
		}

	} finally {
	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplDtbDeleteNode \n"));

return(ret);
} /* RplDtbDeleteNode */


/*B**************************************************************************
 * RplDtbSearch - 
 * 
 * Front End, First search the database from Device ID.
 * Search if interval in p_hdr intercept nodes in the tree.
 *  
 *E==========================================================================
 */

MMG_PUBLIC
BOOLEAN
RplDtbSearch(OS_LIST_ENTRY *p_rootlnk, dtbnode_t *p_searchnode)

/**/
{
	BOOLEAN        ret            = FALSE;
	OS_LIST_ENTRY  *p_dtbroot_lnk = NULL;
	dtbroot_t      *p_dtbroot     = NULL;
	dtbnode_t      *p_dummy       = NULL;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplDtbSearch \n"));
	
	try {

		if (IsListEmpty(p_rootlnk)==FALSE)
		{
			p_dtbroot_lnk = p_rootlnk->Flink;
			while (p_dtbroot_lnk != p_rootlnk)
			{
				p_dtbroot = CONTAINING_RECORD(p_dtbroot_lnk, dtbroot_t, lnk);
				ASSERT(p_dtbroot->sigtype == RPLCC_DBROOT);
				if (p_dtbroot->DevicePtr  == p_searchnode->DeviceId)
				{
					/*
					 * We found a DeviceID will use
					 * the database pointed by p_dtbroot
					 */
					ret = _RplDtbRBSearch(&p_dtbroot->db_root, 
						                  p_searchnode, &p_dummy);
					break;
				}
				p_dtbroot_lnk = p_dtbroot_lnk->Flink;
			}
		}

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplDtbSearch %s \n", (ret==TRUE)?"TRUE":"FALSE"));

return(ret);
} /* RplDtbSearch */

/*
 * PRIVATE SMALL STACK IMPLEMENTATION LIBRARY FOR EMULATING RECURSION
 * This only use for printing a debug trace of the tree so it shouldn't
 * be too optimised.
 */

typedef struct __stack_node
{
	OS_LIST_ENTRY lnk;			//  Next stack node
	
	OS_LIST_ENTRY *p_datalnk;	// The data (dont confuse the 2 entries)

} stack_node_t;

/*B**************************************************************************
 * _RplCCCreateStack - 
 * 
 * 
 *E==========================================================================
 */
MMG_PRIVATE
BOOLEAN
_RplCCCreateStack(stack_node_t **p_stack)

/**/
{
		
	MMGDEBUG(MMGDBG_LOW, ("Entering _RplCCCreateStack \n"));
	
	try {

		*p_stack = NULL;

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving _RplCCCreateStack \n"));

return(TRUE);
} /* _RplCCCreateStack */

/*B**************************************************************************
 * _RplCCDeleteStack - 
 * 
 * 
 *E==========================================================================
 */
MMG_PRIVATE
VOID
_RplCCDeleteStack(stack_node_t *p_stack)

/**/
{
		
	MMGDEBUG(MMGDBG_LOW, ("Entering _RplCCDeleteStack \n"));
	
	try {

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving _RplCCDeleteStack \n"));

} /* _RplCCDeleteStack */

/*B**************************************************************************
 * _RplCCPush - 
 * 
 * 
 *E==========================================================================
 */
MMG_PUBLIC
BOOLEAN
_RplCCPush(stack_node_t **p_stack, OS_LIST_ENTRY *p_data)

/**/
{
	stack_node_t *p_elem = NULL;
		
	MMGDEBUG(MMGDBG_LOW, ("Entering _RplCCPush \n"));
	
	try {
		if (p_data == NULL)
		{
			MMGDEBUG(MMGDBG_INFO, ("Empty data \n"));
			leave;
		}

		p_elem = (stack_node_t *)OS_ExAllocatePoolWithTag(PagedPool,
			                                              sizeof(stack_node_t), 
														  'kats');
		if (p_elem == NULL)
		{
			MMGDEBUG(MMGDBG_FATAL, ("Cannot allocate node \n"));
			return FALSE;
		}

		InitializeListHead(&p_elem->lnk);
		p_elem->p_datalnk  = p_data;
		p_elem->lnk.Flink  = &(*p_stack)->lnk;
		*p_stack           = p_elem;

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving _RplCCPush \n"));

return(TRUE);
} /* _RplCCPush */

/*B**************************************************************************
 * _RplCCPop - 
 * 
 * 
 *E==========================================================================
 */

MMG_PUBLIC
BOOLEAN
_RplCCPop(stack_node_t **p_stack, OS_LIST_ENTRY **p_data)

/**/
{
	stack_node_t *p_elem = NULL;
		
	MMGDEBUG(MMGDBG_LOW, ("Entering _RplCCPop \n"));
	
	try {

		p_elem = *p_stack;
		if (p_elem == NULL)
		{
			return FALSE;
		}
		*p_data = p_elem->p_datalnk;
		*p_stack = CONTAINING_RECORD(p_elem->lnk.Flink, stack_node_t, lnk);
		OS_ExFreePool(p_elem);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving _RplCCPop \n"));

return(TRUE);
} /* _RplCCPop */

/**/

/*B**************************************************************************
 * _RplDtbDisplayNode - 
 * 
 * Print a node of the binary tree
 * 
 *E==========================================================================
 */
MMG_PRIVATE
VOID
_RplDtbDisplayNode(dtbnode_t *p_node, PVOID notused)

/**/
{
	dtbnode_t      *p_left = NULL,
				   *p_right = NULL;

	MMGDEBUG(MMGDBG_LOW, ("Entering _RplDtbDisplayNode \n"));
	
	try {

		MMGDEBUG(MMGDBG_INFO, ("p_node: %lx \n", (unsigned long long)p_node));
		MMGDEBUG(MMGDBG_INFO, ("color %s \n", (p_node->color==RED)?"RED":"BLACK"));
		MMGDEBUG(MMGDBG_INFO, ("DeviceId %lx \n", (ULONGLONG )p_node->DeviceId));
		MMGDEBUG(MMGDBG_INFO, ("[KEY]IntervalLow  %d \n", p_node->IntervalLow ));
		MMGDEBUG(MMGDBG_INFO, ("IntervalHigh   %d \n", p_node->IntervalHigh ));
		MMGDEBUG(MMGDBG_INFO, ("MaximumHigh  %d \n", p_node->MaximumHigh ));
		MMGDEBUG(MMGDBG_INFO, ("p_parent  %x \n", (unsigned long long)p_node->p_parent));
		MMGDEBUG(MMGDBG_INFO, ("Right child %x \n", 
			(p_node->lnk.Flink==NULL)?0:
		    (unsigned long long)CONTAINING_RECORD(p_node->lnk.Flink, dtbnode_t, lnk)));
		MMGDEBUG(MMGDBG_INFO, ("Left  child %x \n", 
			(p_node->lnk.Blink==NULL)?0:
		    (unsigned long long)CONTAINING_RECORD(p_node->lnk.Blink, dtbnode_t, lnk)));
		MMGDEBUG(MMGDBG_INFO, ("size  %x \n\n", p_node->Size ));

		/*
		 * some rudimentary checking
		 */
		if (p_node->color == RED && p_node->p_parent != NULL &&
			p_node->p_parent->color == RED)
		{
			MMGDEBUG(MMGDBG_FATAL, ("Tree imbalanced R/R \n"));
			ASSERT(TRUE==FALSE);
		}
		if (p_node->color == BLACK && p_node->p_parent != NULL &&
			p_node->p_parent->color == BLACK)
		{
			MMGDEBUG(MMGDBG_FATAL, ("Tree imbalanced B/B \n"));
			ASSERT(TRUE==FALSE);
		}

		/*
	 	 * Get the next 2 children if any and check the sort order
		 */
		p_left = NULL;
		p_right = NULL;
		if (p_node->lnk.Blink != NULL)
		{
			p_left = CONTAINING_RECORD(p_node->lnk.Blink, dtbnode_t, lnk);
		}
		if (p_node->lnk.Flink != NULL)
		{
			p_right = CONTAINING_RECORD(p_node->lnk.Flink, dtbnode_t, lnk);
		}
		if (p_left != NULL && 
			p_left->IntervalLow  > p_node->IntervalLow )
		{
			MMGDEBUG(MMGDBG_FATAL, ("Tree not sorted L>P \n"));
			ASSERT(TRUE==FALSE);
		}
		if (p_right != NULL &&
			p_right->IntervalLow  <= p_node->IntervalLow )
		{
			MMGDEBUG(MMGDBG_FATAL, ("Tree not sorted R<P \n"));
			ASSERT(TRUE==FALSE);
		}
		
	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving _RplDtbDisplayNode \n"));

} /* _RplDtbDisplayNode */


/*B**************************************************************************
 * _RplDtbDumpTree - 
 * 
 * Traverse the binary tree depth first and print the nodes.
 * This is a non recursive algorithm so we'll use our stack.
 * 
 *E==========================================================================
 */

MMG_PRIVATE
VOID
_RplDtbDumpTree(OS_LIST_ENTRY p_rootlnk)

/**/
{
	dtbnode_t      *p_node;
	stack_node_t   *p_MyStack;
	OS_LIST_ENTRY  *p_next_lnk= NULL;

	MMGDEBUG(MMGDBG_LOW, ("Entering _RplDtbDumpTree \n"));
	
	try {

		_RplCCCreateStack(&p_MyStack);
		_RplCCPush(&p_MyStack, p_rootlnk.Blink);

		while (_RplCCPop(&p_MyStack, &p_next_lnk)==TRUE)
		{
			p_node = CONTAINING_RECORD(p_next_lnk, dtbnode_t, lnk);
			ASSERT(p_node);
			ASSERT(p_node->sigtype == RPLCC_DTB);
			_RplDtbDisplayNode(p_node, NULL);

			/*
			 * Push next nodes on the stack
			 */
			_RplCCPush(&p_MyStack, p_node->lnk.Flink);
			_RplCCPush(&p_MyStack, p_node->lnk.Blink);
		}
		_RplCCDeleteStack(p_MyStack);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving _RplDtbDumpTree \n"));

} /* _RplDtbDumpTree */

/*B**************************************************************************
 * RplDtbDump - 
 * 
 * Traverse The database linked list for the group and print the tree.
 * 
 *E==========================================================================
 */

MMG_PUBLIC
VOID
RplDtbDump(OS_LIST_ENTRY *p_rootlnk)

/**/
{
	OS_LIST_ENTRY *p_dtbroot_lnk  = NULL;
	dtbroot_t     *p_dtbroot      = NULL;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplDtbDump \n"));
	
	try {

		if (IsListEmpty(p_rootlnk)==FALSE)
		{
			p_dtbroot_lnk = p_rootlnk->Flink;
			while (p_dtbroot_lnk != p_rootlnk)
			{
				p_dtbroot = CONTAINING_RECORD(p_dtbroot_lnk, dtbroot_t, lnk);
				ASSERT(p_dtbroot->sigtype == RPLCC_DBROOT);
				_RplDtbDumpTree(p_dtbroot->db_root);
				p_dtbroot_lnk = p_dtbroot_lnk->Flink;
			}
		}

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplDtbDump \n"));

} /* RplDtbDump */


/*B**************************************************************************
 * _RplDtbDeviceTree - 
 * 
 * For this device tree, traverse the binary tree depth first and
 * apply the callback.
 * This is a non recursive algorithm so we'll use our stack.
 * 
 *E==========================================================================
 */

MMG_PRIVATE
VOID
_RplDtbDeviceTree(OS_LIST_ENTRY    p_rootlnk,
				  CC_nodeaction_t  (__fastcall *callback)(PVOID, rplcc_iorp_t *),
				  PVOID            p_ctxt
				  )

/**/
{
	dtbnode_t      *p_node,
			       *p_left = NULL,
				   *p_right = NULL;
	stack_node_t   *p_MyStack;
	OS_LIST_ENTRY  *p_next_lnk= NULL;
	rplcc_iorp_t    Callback_param;
	CC_nodeaction_t action;


	MMGDEBUG(MMGDBG_LOW, ("Entering _RplDtbDeviceTree \n"));
	
	try {

		_RplCCCreateStack(&p_MyStack);
		_RplCCPush(&p_MyStack, p_rootlnk.Blink);

		while (_RplCCPop(&p_MyStack, &p_next_lnk)==TRUE)
		{
			p_node = CONTAINING_RECORD(p_next_lnk, dtbnode_t, lnk);
			ASSERT(p_node);
			ASSERT(p_node->sigtype == RPLCC_DTB);

			/*
			 * build the parameters
			 */
			Callback_param.DevicePtr = p_node->DeviceId;
			Callback_param.DeviceId  = 0;
			Callback_param.Blk_Start = p_node->Blk_Start;
			Callback_param.Size      = p_node->Size;
			Callback_param.pBuffer   = NULL;

			action = callback(p_ctxt, &Callback_param);

			/*
			 * Push next nodes on the stack
			 */
			_RplCCPush(&p_MyStack, p_node->lnk.Flink);
			_RplCCPush(&p_MyStack, p_node->lnk.Blink);
		}
		_RplCCDeleteStack(p_MyStack);

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving _RplDtbDeviceTree \n"));

} /* _RplDtbDeviceTree */


/*B**************************************************************************
 * RplDtbDump - 
 * 
 * Traverse The database linked list for the group and 
 * do the call back.
 * 
 *E==========================================================================
 */

MMG_PUBLIC
VOID
RplDtbTraverseTree(OS_LIST_ENTRY *p_rootlnk, 
				   CC_nodeaction_t (__fastcall *callback)(PVOID, rplcc_iorp_t *),
				   PVOID p_ctxt
				   )

/**/
{
	OS_LIST_ENTRY *p_dtbroot_lnk  = NULL;
	dtbroot_t     *p_dtbroot      = NULL;

	MMGDEBUG(MMGDBG_LOW, ("Entering RplDtbTraverseTree \n"));
	
	try {

		if (IsListEmpty(p_rootlnk)==FALSE)
		{
			p_dtbroot_lnk = p_rootlnk->Flink;
			while (p_dtbroot_lnk != p_rootlnk)
			{
				p_dtbroot = CONTAINING_RECORD(p_dtbroot_lnk, dtbroot_t, lnk);
				ASSERT(p_dtbroot->sigtype == RPLCC_DBROOT);
				_RplDtbDeviceTree(p_dtbroot->db_root, 
					              callback, 
								  p_ctxt);
				p_dtbroot_lnk = p_dtbroot_lnk->Flink;
			}
		}

	} finally {

	}

	MMGDEBUG(MMGDBG_LOW, ("Leaving RplDtbTraverseTree \n"));

} /* RplDtbTraverseTree */

/* EOF */