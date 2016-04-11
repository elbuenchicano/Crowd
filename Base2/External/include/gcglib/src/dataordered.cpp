/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

    DATAORDERED.CPP: classes for data structures of ordered elements

    Marcelo Bernardes Vieira

**************************************************************************************/

#include "system.h"



////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
/////// AVL TREE CLASS
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////


// Minimum allocation blocks for the path
#define DEFAULT_ARRAY_BLOCK  32

// Defines node balances
#define UNBALANCED_LEFT   -1
#define BALANCED           0
#define UNBALANCED_RIGHT   1

// Keeps one level in a path from root to leaf in an AVL tree
typedef struct _AVL_LEVEL_PATH {
	gcgORDEREDNODE  *node;      // Current node in AVL tree
	signed char     direction;	// New node was inserted in left or right child?
} AVL_LEVEL_PATH;



//////////////////////////////////////
// Macros for single AVL rotations
//////////////////////////////////////

//       a *                  b *
//        /        LL          / \           //
//     b *         =>     ... *   * a
//      / \                      /
// ... *   * aux            aux *
#define ROTATE_LL(a, b)  aux = (gcgORDEREDNODE*) b->right;   \
	                       b->right = a;     \
	                       a->left = aux;

//   a *                   b *
//      \         RR        / \              //
//     b *        =>     a *   * ...
//      / \                 \                //
// aux *   * ...         aux *
#define ROTATE_RR(a, b)  aux = (gcgORDEREDNODE*) b->left;    \
	                       b->left = a;      \
	                       a->right = aux;

//////////////////////////////////////
// Macros for double AVL rotations
//////////////////////////////////////

//     a *                  a *               c *
//      /         L          /     R           /  \        //
//   b *          =>      c *      =>         /    \       //
//      \                  /               b *      * a    //
//       * c            b *                   \    /
//      / \                                aux *  * aux2
// aux *   * aux2
#define ROTATE_LR(a, b, c)  aux = (gcgORDEREDNODE*) c->left;    \
	                          c->left = b;      \
	                          b->right = aux;   \
	                          aux = (gcgORDEREDNODE*) c->right;   \
	                          c->right = a;     \
	                          a->left = aux;

//      a *               a *                   c *
//         \        R        \       L           /  \       //
//       b  *       =>      c *      =>         /    \      //
//         /                   \             a *      * b
//      c *                   b *               \    /
//       / \                                 aux *  * aux2
//  aux *   * aux2
#define ROTATE_RL(a, b, c)  aux  = (gcgORDEREDNODE*) c->left;   \
	                          c->left = a;      \
	                          a->right = aux;   \
	                          aux = (gcgORDEREDNODE*) c->right;   \
	                          c->right = b;     \
	                          b->left = aux;


//////////////////////////////////////
// Macros for updating root pointers
//////////////////////////////////////
#define UPDATE_ROOT(a, b) if(b > this->path) {                                                                                    \
                            AVL_LEVEL_PATH *parent = b - 1; /* Retrieve the PARENT of the subtree BEFORE rotation */              \
		                        if(parent->direction == UNBALANCED_LEFT) parent->node->left = a; /* LEFT: adjust subtree's new root */   \
		                        else parent->node->right = a; /* RIGHT: adjust subtree's new root */                                     \
	                        } else this->root = a; /* Subtree's root node became the global root */


 gcgAVLTREE::gcgAVLTREE() {
  this->root      = NULL;
  this->counter   = 0;
  // These itens are private
  this->path      = NULL; // Dynamic array to keep the path from root to leaf when adding/deleting
  this->levels    = 0;    // Current number of levels = path depth
  this->maxlevels = 0;    // Maximum number of levels that can be used in the array
}

gcgAVLTREE::~gcgAVLTREE() {
  if(!this->deleteAll())
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_REMOVALERROR), "~gcgAVLTREE(): could not remove all entries. (%s:%d)", basename((char*)__FILE__), __LINE__);
}

gcgORDEREDNODE  *gcgAVLTREE::insert(gcgORDEREDNODE  *newnode) {
	gcgORDEREDNODE *subroot, *firstchild, *aux;
	AVL_LEVEL_PATH *subrootentry, *lastentry, *firstchildentry, *grandchildentry;
  int            deepest = -1;

  // Check parameter
  if(newnode == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "insert(): the new node is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  // If NULL this is the first element of the tree
	if(this->root == NULL) {
    // Init node
    newnode->left = newnode->right = 0;
    newnode->balance = BALANCED;
    this->counter++;
    return this->root = newnode;
  }

  // Initialize the path: start from the top
	this->levels = 0;

  // Search position for new element and build path: O(log n)
	aux = this->root;
	do {
    AVL_LEVEL_PATH  *step;

    // Check current array size
    CHECK_DYNAMIC_ARRAY_CAPACITY(this->path, this->maxlevels, this->levels + 1, AVL_LEVEL_PATH, NULL);

    // Get the next free position in the path array
    step = (AVL_LEVEL_PATH*) &this->path[(this->levels)++];

    // Set current node in the path array
		step->node = aux;

    // Pick this deeper node as subroot, if its balance is not ZERO
    if(aux->balance != BALANCED) deepest = this->levels; // Find smallest (and deepest) unbalanced subtree

    // Choosing child
    int res = this->compare(aux, newnode);
		if(res > 0) {
      // New value is smaller: go to the left node
			step->direction = UNBALANCED_LEFT; // Mark entry in the array as -1 = LEFT
			aux = (gcgORDEREDNODE*) aux->left;  // Go to left
		} else
        if(res < 0) {
          // New value is greater: go to the right node
			    step->direction = UNBALANCED_RIGHT; // Mark entry in the array as 1 = RIGHT
			    aux = (gcgORDEREDNODE*) aux->right;
		    } else {
              gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INSERTIONFAILED), "insert(): the key node already exists in the tree. (%s:%d)", basename((char*)__FILE__), __LINE__);
              return aux;  // Key already exists in the tree
            }
	} while(aux != NULL);

  // The path from root to the leaf is registered in the array
	lastentry = &this->path[this->levels - 1]; // Last node in the path = PARENT of the new node

  // Insert node in the tree, checking the position
  newnode->left = newnode->right = 0;
  newnode->balance = BALANCED;
	if(lastentry->direction == UNBALANCED_LEFT) lastentry->node->left = newnode;  // Insert LEFT child
	else lastentry->node->right = newnode;                                        // Insert RIGHT child

	// Check if all subtrees are balanced
	if(deepest < 0) {
    // No unbalanced node found: current balances are ZERO
    // Update all balances from subtree's root to the inserted node
    for(AVL_LEVEL_PATH *g = this->path; g <= lastentry; g++) g->node->balance = g->direction;
    this->counter++;
		return newnode;
	}

  // Root of the unbalanced subtree
  subrootentry = &this->path[deepest - 1];
	subroot = subrootentry->node;

  // An unbalanced node was found
  // Check if the insertion made its subtree balanced
	if(subrootentry->direction + subroot->balance == BALANCED) {
    // The inserted node balanced the unbalanced subtree´s root
		subroot->balance = BALANCED;
		// Update all balances from subtree's root to the inserted node
    for(AVL_LEVEL_PATH *g = subrootentry + 1; g <= lastentry; g++) g->node->balance = g->direction;
    this->counter++;
		return newnode;
	}

	firstchildentry = subrootentry + 1; // Reference of the child of the unbalanced root subtree
  firstchild = firstchildentry->node; // First child of the root subtree

  // Check if a LL rotation is needed
	if (subrootentry->direction == UNBALANCED_LEFT && firstchildentry->direction == UNBALANCED_LEFT) {
    // In this case we have    subroot *                  firstchild *
    //                                /         LL                  / \              //
    //                    firstchild *          =>             ... *   * subroot
    //                              /                                                //
    //                         ... *
		ROTATE_LL(subroot, firstchild);
    UPDATE_ROOT(firstchild, subrootentry);
		subroot->balance = BALANCED;
    // Update all balances from subtree's first child to the inserted node
    for(AVL_LEVEL_PATH *g = firstchildentry + 1; g <= lastentry; g++) g->node->balance = g->direction;
    this->counter++;
		return newnode;
	}

	// Check if a RR rotation is needed
	if (subrootentry->direction == UNBALANCED_RIGHT && firstchildentry->direction == UNBALANCED_RIGHT) {
    // In this case we have       subroot *                   r1 *
    //                                     \         RR         / \                   //
    //                                   r1 *        =>     pp *   * ...
    //                                       \                                        //
    //                                        * ...
		ROTATE_RR(subroot, firstchild);
    UPDATE_ROOT(firstchild, subrootentry);
		subroot->balance = BALANCED;
    // Update all balances from subtree's first child to the inserted node
    for(AVL_LEVEL_PATH *g = firstchildentry + 1; g <= lastentry; g++) g->node->balance = g->direction;
    this->counter++;
		return newnode;
	}

  ////////////////////////////////////
  // DOUBLE ROTATIONS
  ////////////////////////////////////

	grandchildentry = subrootentry + 2;  // Entry of the grandchild of the root subtree

	// Simplest cases for LR/RL rotation: inserted node is the grandchild of the subtree's root (small subtree)
	if(grandchildentry > lastentry) { // There is a grandchild?
    // In this case we have     subroot *            newnode *                         subroot *               newnode *
    //                                 /    LR              / \                                 \     RL              / \                     //
    //                     firstchild *     =>  firstchild *   * subroot      OR      firstchild *    =>     subroot *   * firstchild
    //                                 \                                                        /                                             //
    //                                  * newnode                                              * newnode
		if(subrootentry->direction == UNBALANCED_LEFT) {
      ROTATE_LR(subroot, firstchild, newnode);
      UPDATE_ROOT(newnode, subrootentry);
    } else {
        ROTATE_RL(subroot, firstchild, newnode);
        UPDATE_ROOT(newnode, subrootentry);
      }
		subroot->balance = firstchild->balance = BALANCED; // All nodes balanced
    this->counter++;
		return newnode;
	}

	// Check if a LR rotation is needed internally in the tree
	if(subrootentry->direction == UNBALANCED_LEFT) {
    // In this case we have       subroot *                grandchildentry *
    //                                   /        LR                      / \                      //
    //                       firstchild *         =>          firstchild *   * subroot
    //                                   \                              /     \                    //
    //                                    * grandchildentry
    //                                   / \                                                       //
    //
		ROTATE_LR(subroot, firstchild, grandchildentry->node);
    UPDATE_ROOT(grandchildentry->node, subrootentry);
    // Update balances of original subtree's root and its first child
		if (grandchildentry->direction == UNBALANCED_LEFT) {
			firstchild->balance = BALANCED;
			subroot->balance    = UNBALANCED_RIGHT;
		} else {
			firstchild->balance = UNBALANCED_LEFT;
			subroot->balance    = BALANCED;
		}
    // Now update all balances from subtree's grandchild until the inserted node
    for(AVL_LEVEL_PATH *g = grandchildentry + 1; g <= lastentry; g++) g->node->balance = g->direction;
    this->counter++;
		return newnode;
	}

	// Here a LR rotation is needed internally in the tree

  // In this case we have   subroot *                grandchildentry *
  //                                 \       RL                     / \                  //
  //                       firstchild *      =>            subroot *   * firstchild
  //                                   \                          /     \                //
  //                    grandchildentry *
  //                                   / \                                               //
	ROTATE_RL(subroot, firstchild, grandchildentry->node);
  UPDATE_ROOT(grandchildentry->node, subrootentry);
  // Update balances of original subtree's root and its first child
	if(grandchildentry->direction == UNBALANCED_LEFT) {
		subroot->balance    = BALANCED;
		firstchild->balance = UNBALANCED_RIGHT;
	} else {
		subroot->balance    = UNBALANCED_LEFT;
		firstchild->balance = BALANCED;
	}
	// Now update all balances from subtree's grandchild until the inserted node
  for(AVL_LEVEL_PATH  *g = grandchildentry + 1; g <= lastentry; g++) g->node->balance = g->direction;
  this->counter++;
	return newnode;
}


//  SAMPLE CODE FOR TESTING RANDOM INSERTIONS AND DELETIONS USING gcgAVLTREE
//
//  for(int i = 0; i < 1000000000; i++) {
//    gcgORDEREDNODE  *p = new gcgORDEREDNODE (r.intRandom(0,10000000));
//      if(p != t.insert(p)) delete p;
//    for(int j = 0; j < 100; j++) {
//      p = t.remove(r.intRandom(0,10000000));
//      if(p) delete p;
//    }
//  }
//
// TOTAL OF DELETES: 990003812
// Number of levels, beginning in the leaves, enough to make the tree balanced.
//Level 1 = 522096675  => 52.74%                      Level 2 = 247847780 => 25.04%
//Level 3 = 111232857  => 11.24%                      Level 4 = 53810227  => 5.44%
//Level 5 = 26509832   => 2.68%                       Level 6 = 13439871  => 1.36%
//Level 7 = 7044159    => 0.71%                       Level 8 = 3809043   => 0.38%
//Level 9 = 2059993    => 0.21%                       Level 10 = 1074565  => 0.11%
//Level 11 = 541590    => 0.05%                       Level 12 = 272074   => 0.03%
//Level 13 = 124057    => 0.01%                       Level 14 = 57553    => 0.01%
//Level 15 = 38778     => 0.00%                       Level 16 = 27154    => 0.00%
//Level 17 = 12509     => 0.00%                       Level 18 = 4247     => 0.00%
//Level 19 = 848       => 0.00%
//
gcgORDEREDNODE  *gcgAVLTREE::remove(gcgORDEREDNODE  *key) {
	AVL_LEVEL_PATH    *entry;
	gcgORDEREDNODE *node, *aux;
  int               pos = -1;

  // Check parameter
  if(key == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "remove(): the key node is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  // Search element to be removed and build path: O(log n)
	this->levels = 0;
	node = this->root;
	while(node != NULL) {
    // Check current array size
    CHECK_DYNAMIC_ARRAY_CAPACITY(this->path, this->maxlevels, this->levels + 1, AVL_LEVEL_PATH, NULL);

    // Get the next free position in the path array
    entry = (AVL_LEVEL_PATH*) &this->path[(this->levels)++];

    // Check if found the node to be removed
    int res = this->compare(key, node);
		if(res == 0) pos = this->levels - 1; // Node found. But continue until its sucessor is reached

    // Fill in the entry in the array of the path
		entry->node = node;
		if(res < 0) {
			entry->direction = UNBALANCED_LEFT;
			node = (gcgORDEREDNODE*) node->left;  // Go left to find the node
		} else {
			entry->direction = UNBALANCED_RIGHT;
			node = (gcgORDEREDNODE*) node->right; // Go right to find the node
		}
	}

  // Check if the tree is empty, or if the key is not in the tree
  if(pos == -1) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "remove(): could not find the node to be removed. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  // Here we know the node exists and must be removed

  // Keep target node
  node = this->path[pos].node;
  if(this->levels == 1) {
    // Only root is present
    this->root = (this->path[pos].direction == UNBALANCED_LEFT) ? (gcgORDEREDNODE*) node->right : (gcgORDEREDNODE*) node->left;
    this->counter--;
    return node;
  }

  // Here we know the tree is deep enough to see its bottom
  aux = (this->path[this->levels - 1].direction == UNBALANCED_LEFT) ? (gcgORDEREDNODE*) this->path[this->levels - 1].node->right : (gcgORDEREDNODE*) this->path[this->levels - 1].node->left;

  // Disconnect bottom node from its parent
  if(this->path[this->levels - 2].direction == UNBALANCED_LEFT) this->path[this->levels - 2].node->left = aux;
  else this->path[this->levels - 2].node->right = aux;

  // The target node is not in the bottom? Same as: already removed?
  if(pos < this->levels - 1) {
    // Replace entry
    this->path[this->levels - 1].node->right = node->right;
    this->path[this->levels - 1].node->left = node->left;

    // Connect the leaf to the entry's parent
    if(pos == 0) this->root = this->path[this->levels - 1].node; // Its is a root node now
    else
      if(this->path[pos - 1].direction == UNBALANCED_LEFT) this->path[pos - 1].node->left = this->path[this->levels-1].node;
      else this->path[pos - 1].node->right = this->path[this->levels-1].node;
  }

  // Update path before balancing
  this->path[pos].node = this->path[this->levels - 1].node;
  this->path[pos].node->balance = node->balance;

  // Check balances from bottom to the top
  // Using http://www.slideshare.net/ecomputernotes/computer-notes-data-structures-22
  for(entry = &this->path[levels - 2]; entry >= this->path; entry--) {
    gcgORDEREDNODE  *subnode = entry->node;

    if(entry->direction == UNBALANCED_LEFT) {
      if(subnode->balance == BALANCED) {
        subnode->balance = UNBALANCED_RIGHT;
        this->counter--;
        return node;
      }
      if(subnode->balance == UNBALANCED_LEFT) {
        subnode->balance = BALANCED;
        continue;
      }
      //UNBALANCED_RIGHT
      gcgORDEREDNODE  *aux2 = (gcgORDEREDNODE*) subnode->right;

      if(aux2->balance == BALANCED) {
        ROTATE_RR(subnode, aux2);
        UPDATE_ROOT(aux2, entry);

        subnode->balance = UNBALANCED_RIGHT;
        aux2->balance = UNBALANCED_LEFT;
        this->counter--;
        return node;
      }

      if(aux2->balance == UNBALANCED_LEFT) {
        gcgORDEREDNODE  *aux3 = (gcgORDEREDNODE*) aux2->left;
        ROTATE_RL(subnode, aux2, aux3);
        UPDATE_ROOT(aux3, entry);

        if(aux3->balance == UNBALANCED_LEFT) {
          subnode->balance = BALANCED;
          aux2->balance = UNBALANCED_RIGHT;
        } else
          if(aux3->balance == UNBALANCED_RIGHT) {
            subnode->balance = UNBALANCED_LEFT;
            aux2->balance = BALANCED;
          } else aux2->balance = subnode->balance = BALANCED;
        aux3->balance = BALANCED;
        continue;
      }

      //UNBALANCED_RIGHT
      ROTATE_RR(subnode, aux2);
      UPDATE_ROOT(aux2, entry);
      subnode->balance = aux2->balance = BALANCED;
      continue;
    }

    if(entry->direction == UNBALANCED_RIGHT) {
      if(subnode->balance == BALANCED) {
        subnode->balance = UNBALANCED_LEFT;
        this->counter--;
        return node;
      }
      if(subnode->balance == UNBALANCED_RIGHT) {
        subnode->balance = BALANCED;
        continue;
      }
      // UNBALANCED_LEFT
      gcgORDEREDNODE  *aux2 = (gcgORDEREDNODE*) subnode->left;

      if(aux2->balance == BALANCED) {
        ROTATE_LL(subnode, aux2);
        UPDATE_ROOT(aux2, entry);

        subnode->balance = UNBALANCED_LEFT;
        aux2->balance = UNBALANCED_RIGHT;
        this->counter--;
        return node;
      }

      if(aux2->balance == UNBALANCED_RIGHT) {
        gcgORDEREDNODE  *aux3 = (gcgORDEREDNODE*) aux2->right;
        ROTATE_LR(subnode, aux2, aux3);
        UPDATE_ROOT(aux3, entry);

        if(aux3->balance == UNBALANCED_LEFT) {
          subnode->balance = UNBALANCED_RIGHT;
          aux2->balance = BALANCED;
        } else
          if(aux3->balance == UNBALANCED_RIGHT) {
            subnode->balance = BALANCED;
            aux2->balance = UNBALANCED_LEFT;
          } else aux2->balance = subnode->balance = BALANCED;

        aux3->balance = BALANCED;
        continue;
      }

      //UNBALANCED_LEFT
      ROTATE_LL(subnode, aux2);
      UPDATE_ROOT(aux2, entry);
      subnode->balance = aux2->balance = BALANCED;
    }
  }

  this->counter--;
  return node;
}

gcgORDEREDNODE *gcgAVLTREE::search(gcgORDEREDNODE  *key) {
	gcgORDEREDNODE *aux;

  // Check parameter
  if(key == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "search(): the key node is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  // Search position for new element and build path: O(log n)
	aux = this->root;
	while(aux != NULL) {
    int res = this->compare(aux, key);
    if(res == 0) return aux;  // Key found. Return pointer to the node

    // Search in binary tree
		if(res > 0) aux = (gcgORDEREDNODE*) aux->left;  // Reference value is smaller: go to the left node
		else aux = (gcgORDEREDNODE*) aux->right;        // Reference value is greater: go to the right node
	}

  // Node was not found
  gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "search(): could not find the key node. (%s:%d)", basename((char*)__FILE__), __LINE__);
  return NULL;
}

bool gcgAVLTREE::deleteAll() {
	AVL_LEVEL_PATH *entry;
	gcgORDEREDNODE *node;

  // Stack of nodes to be deleted: its size is O(log n)
	this->levels = 0;
	node = this->root;
	while(node != NULL) {
    // Check current array size: we need at least two new positions
    CHECK_DYNAMIC_ARRAY_CAPACITY(this->path, this->maxlevels, this->levels + 2, AVL_LEVEL_PATH, false);

    // Check left child
    if(node->left != NULL) {
      // Get the next free position in the path array
      entry = (AVL_LEVEL_PATH*) &this->path[(this->levels)++];
		  entry->node = (gcgORDEREDNODE*) node->left; // Fill in the entry with left child
    }

    // Check right child
    if(node->right != NULL) {
      // Get the next free position in the path array
      entry = (AVL_LEVEL_PATH*) &this->path[(this->levels)++];
		  entry->node = (gcgORDEREDNODE*) node->right; // Fill in the entry with right child
    }

    // With the children saved in the stack, we can delete the node
    SAFE_DELETE(node);

    // Get the next node from the stack
    if(this->levels > 0) {
      node = this->path[--(this->levels)].node;
    } else break;
  }

  // Reset the AVL tree
  this->root   = NULL;
  this->counter = 0;
  SAFE_FREE(this->path);
  this->levels = 0;
  this->maxlevels = 0;
  return true;
}

gcgITERATOR *gcgAVLTREE::detach(int traversemode) {
  gcgITERATOR *it = getIterator(traversemode);
  this->root   = NULL;
  this->counter = 0;
  SAFE_FREE(this->path);
  this->levels = 0;
  this->maxlevels = 0;
  return it;
}


uintptr_t gcgAVLTREE::getCounter() {
  return this->counter;
}

// Class for AVL TREE iterations
class gcgAVLTREEITERATOR : public gcgITERATOR {
  private:
    int     traversemode;
    gcgORDEREDNODE **stack;  // Stack to hold nodes to be returned.
	  int     top;
	  int     maxnodes;

  public:
    gcgAVLTREEITERATOR(int mode, gcgORDEREDNODE *root) : traversemode(mode) {
      this->stack = (gcgORDEREDNODE**) ALLOC(sizeof(gcgORDEREDNODE*) * DEFAULT_ARRAY_BLOCK);
      if(this->stack != NULL) {
	      gcgORDEREDNODE *node;

        this->top = 0;
        this->maxnodes = DEFAULT_ARRAY_BLOCK;

        // Fill in stack of nodes: its size is O(log n)
        node = root;
	      while(node != NULL) {
          CHECK_DYNAMIC_ARRAY_CAPACITY_VOID(this->stack, this->maxnodes, this->top + 1, gcgORDEREDNODE*);

          // Put current node in the stack
          this->stack[(this->top)++] = node;
          if(mode == 1) node = (gcgORDEREDNODE*) node->right; // Descending order
          else node = (gcgORDEREDNODE*) node->left; // Ascending order (default)
        }
      } else {
        this->top = 0;
        this->maxnodes = 0;
        this->traversemode = 0;
        gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgAVLTREEITERATOR(): stack allocation error. (%s:%d)", basename((char*)__FILE__), __LINE__);
      }
    }

    ~gcgAVLTREEITERATOR() {
      SAFE_FREE(this->stack);
      this->top = 0;
      this->maxnodes = 0;
      this->traversemode = 0;
    }

    // Interface
    gcgDATA *next() {
      if(this->stack != NULL) {
         if(this->top > 0) {
          // Retrieve top of the stack
          gcgORDEREDNODE *node = this->stack[--this->top], *child;

          if(traversemode == 1) {
            //// DESCENDING ORDER
            // Fill the stack with rightmost children of the left child
            child = (gcgORDEREDNODE*) node->left;
	          while(child != NULL) {
              // Check current array size
              CHECK_DYNAMIC_ARRAY_CAPACITY(this->stack, this->maxnodes, this->top + 1, gcgORDEREDNODE*, NULL);

              // Put current node in the stack
              this->stack[(this->top)++] = child;
              child = (gcgORDEREDNODE*) child->right;
            }
          } else {
            //// ASCENDING ORDER (DEFAULT)
            // Fill the stack with leftmost children of the right child
            child = (gcgORDEREDNODE*) node->right;
	          while(child != NULL) {
              // Check current array size
              CHECK_DYNAMIC_ARRAY_CAPACITY(this->stack, this->maxnodes, this->top + 1, gcgORDEREDNODE*, NULL);

              // Put current node in the stack
              this->stack[(this->top)++] = child;
              child = (gcgORDEREDNODE*) child->left;
            }
          }

          // Return next node
          return node;
        }

        // Release stack
        SAFE_FREE(this->stack);
        this->top = 0;
        this->maxnodes = 0;
      }

      // No more nodes available
      return NULL;
    }
};

gcgITERATOR *gcgAVLTREE::getIterator(int traversemode) {
  gcgAVLTREEITERATOR *iterator = new gcgAVLTREEITERATOR(traversemode, this->root);
  // Check allocation
  if(iterator == NULL) gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "getIterator(): could not allocate iterator. (%s:%d)", basename((char*)__FILE__), __LINE__);
  return (gcgITERATOR*) iterator;
}



/*
// This is the version without any optimization: it is important for future checks
gcgORDEREDNODE  *gcgAVLTREE::remove(int key) {
	AVL_LEVEL_PATH *entry;
	gcgORDEREDNODE      *node, *aux;
  int             pos = -1;

  // Search element to be removed and build path: O(log n)
	this->levels = 0;
	node = this->root;
	while(node != NULL) {
    // Check current array size
    if(this->levels >= this->maxlevels) {
	    int max = this->maxlevels + DEFAULT_ARRAY_BLOCK;
      AVL_LEVEL_PATH *na = (AVL_LEVEL_PATH*) REALLOC(this->path, sizeof(AVL_LEVEL_PATH) * max);
	    if(na == NULL) return NULL; // Memory allocation error
      // Successfull allocation: commit changes
	    this->path = na;
	    this->maxlevels = max;
    }

    // Get the next free position in the path array
    entry = (AVL_LEVEL_PATH*) &this->path[(this->levels)++];

    // Check if found the node to be removed
		if (node->val == key) pos = this->levels - 1; // Node found. But continue until its sucessor is reached

    // Fill in the entry in the array of the path
		entry->node = node;
		if(key < node->val) {
			entry->direction = UNBALANCED_LEFT;
			node = node->left;  // Go left to find the node
		} else {
			entry->direction = UNBALANCED_RIGHT;
			node = node->right; // Go right to find the node
		}
	}

  // Check if the tree is empty, or if the key is not in the tree
  if(pos == -1) return NULL;

  // Here we know the node exists and must be removed

  // Keep target node
  node = this->path[pos].node;
  if(this->levels == 1) {
    // Only root is present
    this->root = (this->path[pos].direction == UNBALANCED_LEFT) ? node->right : node->left;
    return node;
  }

  // Here we know the tree is deep enough to see its bottom
  aux = (this->path[this->levels - 1].direction == UNBALANCED_LEFT) ? this->path[this->levels - 1].node->right : this->path[this->levels - 1].node->left;

  // Disconnect bottom node from its parent
  if(this->path[this->levels - 2].direction == UNBALANCED_LEFT) this->path[this->levels - 2].node->left = aux;
  else this->path[this->levels - 2].node->right = aux;

  // The target node is not in the bottom? Same as: already removed?
  if(pos < this->levels - 1) {
    // Replace entry
    this->path[this->levels - 1].node->right = node->right;
    this->path[this->levels - 1].node->left = node->left;

    // Connect the leaf to the entry's parent
    if(pos == 0) this->root = this->path[this->levels - 1].node; // Its is a root node now
    else
      if(this->path[pos - 1].direction == UNBALANCED_LEFT) this->path[pos - 1].node->left = this->path[this->levels-1].node;
      else this->path[pos - 1].node->right = this->path[this->levels-1].node;
  }

  // Update path before balancing
  this->path[pos].node = this->path[this->levels - 1].node;
  this->path[pos].node->balance = node->balance;

  // Check balances from bottom to the top
  // Using http://www.slideshare.net/ecomputernotes/computer-notes-data-structures-22
  for(entry = &this->path[levels - 2]; entry >= this->path; entry--) {
    gcgORDEREDNODE  *subnode = entry->node;

    // Check case 1A: the parent of the deleted node had a balance of 0 and the node was deleted in the
    // parent's left subtree. Change the parent's balance to RIGHT and stop. No further effect on balance of any higher node
    if(entry->direction == UNBALANCED_LEFT && subnode->balance == BALANCED) {
      subnode->balance = UNBALANCED_RIGHT;
      return node;
    }

    // Check case 1B: the parent of the deleted node had a balance of 0 and the node was deleted in the
    // parent's right subtree. Change the parent's balance to LEFT and stop. No further effect on balance of any higher node
    if(entry->direction == UNBALANCED_RIGHT && subnode->balance == BALANCED) {
      subnode->balance = UNBALANCED_LEFT;
      return node;
    }

    // Check case 2A: the parent of the deleted node had a balance of 1 and the node was deleted in the
    // parent's right subtree. Change the parent's balance to BALANCED. It may have caused imbalances in higher nodes.
    if(entry->direction == UNBALANCED_RIGHT && subnode->balance == UNBALANCED_RIGHT) {
      subnode->balance = BALANCED;
      continue;
    }

    // Check case 2B: the parent of the deleted node had a balance of -1 and the node was deleted in the
    // parent's left subtree. Change the parent's balance to BALANCED. It may have caused imbalances in higher nodes.
    if(entry->direction == UNBALANCED_LEFT && subnode->balance == UNBALANCED_LEFT) {
      subnode->balance = BALANCED;
      continue;
    }

    // Check case 3A: the parent had balance of -1 and the node was deleted in parent's right subtree, its left subtree was balanced.
    // Balancing it does not affect the balance of upper nodes.
    if(entry->direction == UNBALANCED_RIGHT && subnode->balance == UNBALANCED_LEFT) {
      if(subnode->left->balance == BALANCED) {
        //  Perfom a left rotation
        gcgORDEREDNODE  *aux2 = subnode->left;
        ROTATE_LL(subnode, aux2);
        UPDATE_ROOT(aux2, entry);

        // Update balances
        subnode->balance = UNBALANCED_LEFT;
        aux2->balance = UNBALANCED_RIGHT;

        // It's done
        return node;
      }
    }

    // Check case 3B: the parent had balance of 1 and the node was deleted in parent's left subtree, its right subtree was balanced.
    // Balancing it does not affect the balance of upper nodes.
    if(entry->direction == UNBALANCED_LEFT && subnode->balance == UNBALANCED_RIGHT) {
      if(subnode->right->balance == BALANCED) {
        //  Perfom a right rotation
        gcgORDEREDNODE  *aux2 = subnode->right;
        ROTATE_RR(subnode, aux2);
        UPDATE_ROOT(aux2, entry);

        // Update balances
        subnode->balance = UNBALANCED_RIGHT;
        aux2->balance = UNBALANCED_LEFT;

        // It's done
        return node;
      }
    }

    // Check case 4A: the parent had balance of -1 and the node was deleted in parent's left subtree, its right subtree was unbalanced
    // to left. Balancing it may affect the balance of upper nodes.
    if(entry->direction == UNBALANCED_RIGHT && subnode->balance == UNBALANCED_LEFT) {
      if(subnode->left->balance == UNBALANCED_RIGHT) {
        //  Perfom a double left/right rotation
        gcgORDEREDNODE  *aux2 = subnode->left, *aux3 = aux2->right;
        ROTATE_LR(subnode, aux2, aux3);
        UPDATE_ROOT(aux3, entry);

        // Update balances
        if(aux3->balance == UNBALANCED_LEFT) {
          subnode->balance = UNBALANCED_RIGHT;
          aux2->balance = BALANCED;
        } else
          if(aux3->balance == UNBALANCED_RIGHT) {
            subnode->balance = BALANCED;
            aux2->balance = UNBALANCED_LEFT;
          } else aux2->balance = subnode->balance = BALANCED;

//        if(subnode->balance != subnode->computebalance()) {
//          printf("\nSubnode %d   %d",subnode->balance, subnode->computebalance());
//          getchar();
//        }

//        if(aux2->balance != aux2->computebalance()) {
//          printf("\nAux2 %d   %d",aux2->balance, aux2->computebalance());
//          getchar();
//        }

//        subnode->balance = subnode->computebalance();
//        aux2->balance = aux2->computebalance();

        aux3->balance = BALANCED;
        continue;
      }
    }

    // Check case 4B: the parent had balance of 1 and the node was deleted in parent's right subtree, its left subtree was unbalanced
    // to right. Balancing it may affect the balance of upper nodes.
    if(entry->direction == UNBALANCED_LEFT && subnode->balance == UNBALANCED_RIGHT) {
      if(subnode->right->balance == UNBALANCED_LEFT) {
        //  Perfom a double left/right rotation
        gcgORDEREDNODE  *aux2 = subnode->right, *aux3 = aux2->left;
        ROTATE_RL(subnode, aux2, aux3);
        UPDATE_ROOT(aux3, entry);

        // Update balances
        if(aux3->balance == UNBALANCED_LEFT) {
          subnode->balance = BALANCED;
          aux2->balance = UNBALANCED_RIGHT;
        } else
          if(aux3->balance == UNBALANCED_RIGHT) {
            subnode->balance = UNBALANCED_LEFT;
            aux2->balance = BALANCED;
          } else aux2->balance = subnode->balance = BALANCED;


//        if(subnode->balance != subnode->computebalance()) {
//          printf("\nSubnode %d   %d",subnode->balance, subnode->computebalance());
//          getchar();
//        }

//        if(aux2->balance != aux2->computebalance()) {
//          printf("\nAux2 %d   %d",aux2->balance, aux2->computebalance());
//          getchar();
//        }

//        subnode->balance = subnode->computebalance();
//        aux2->balance = aux2->computebalance();

        aux3->balance = BALANCED;
        continue;
      }
    }



    // Check case 5A: the parent had balance of -1 and the node was deleted in parent's right subtree, its left subtree was unbalanced
    // to left. Balancing it may affect the balance of upper nodes.
    if(entry->direction == UNBALANCED_RIGHT && subnode->balance == UNBALANCED_LEFT) {
      if(subnode->left->balance == UNBALANCED_LEFT) {
        //  Perfom a left rotation
        gcgORDEREDNODE  *aux2 = subnode->left;
        ROTATE_LL(subnode, aux2);
        UPDATE_ROOT(aux2, entry);

        // Update balances
        subnode->balance = aux2->balance = BALANCED;
        continue;
      }
    }


    // Check case 5B: the parent had balance of 1 and the node was deleted in parent's left subtree, its right subtree was unbalanced
    // to right. Balancing it may affect the balance of upper nodes.
    if(entry->direction == UNBALANCED_LEFT && subnode->balance == UNBALANCED_RIGHT) {
      if(subnode->right->balance == UNBALANCED_RIGHT) {
        //  Perfom a right rotation
        gcgORDEREDNODE  *aux2 = subnode->right;
        ROTATE_RR(subnode, aux2);
        UPDATE_ROOT(aux2, entry);

        // Update balances
        subnode->balance = aux2->balance = BALANCED;
        continue;
      }
    }
  }

  return node;
}
*/


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
/////// HASH TABLE CLASS
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#define MINIMUM_TABLE_SIZE  20
#define MINIMUM_TABLE_LOAD  (1.0f / MINIMUM_TABLE_SIZE)

class gcgAVLHASH : public gcgAVLTREE {
  public:
    gcgHASHTABLE  *table;

  public:
    // Constructor need table pointer
    gcgAVLHASH(gcgHASHTABLE *t) : table(t) {}

    // Compare just call hash table compare()
    int compare(gcgORDEREDNODE *rf1, gcgORDEREDNODE *rf2) {
      if(table) return table->compare(rf1, rf2);
      return 0;
    }
};

gcgHASHTABLE::gcgHASHTABLE(uintptr_t icapacity, float minload, float maxload) {
  this->table = NULL;
  this->counter = 0;
  this->capacity = 0;
  this->setLoadLimits(minload, maxload);
  this->setCapacity(icapacity);
}

gcgHASHTABLE::~gcgHASHTABLE() {
  if(!this->deleteAll())
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_REMOVALERROR), "~gcgHASHTABLE(): could not remove all entries. (%s:%d)", basename((char*)__FILE__), __LINE__);
}

bool gcgHASHTABLE::setLoadLimits(float minload, float maxload) {
  // Check parameters
  if(minload > 0.0f && maxload > 0.0f && (minload > maxload || FEQUAL(minload, maxload))) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "setLoadLimits(): invalid minimum and maximum values. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Set limits
  this->minimumload = (minload <= 0.0f || minload > MINIMUM_TABLE_LOAD) ? minload : MINIMUM_TABLE_LOAD;
  this->maximumload = (maxload <= 0.0f || maxload > this->minimumload) ? maxload : this->minimumload + 1.0f;

  // Check current load
  if((this->minimumload > 0.0f || this->maximumload > 0.0f) && this->capacity > 0 && this->counter > 0) {
    float currentload = (float) this->counter / (float) this->capacity;
    if((this->minimumload > 0.0f && currentload < this->minimumload) ||
       (this->maximumload > 0.0f && currentload > this->maximumload)) {
      unsigned int newcapacity = (unsigned int) (((float) this->counter / (((this->minimumload > 0.0f) ? this->minimumload : 0.0f) +
                                                                           ((this->maximumload > 0.0f) ? this->maximumload : 0.0f))) * 2.0f);
      if(newcapacity > 0) this->setCapacity(newcapacity);
    }
  }

  return true;
}

bool gcgHASHTABLE::setCapacity(uintptr_t newcapacity) {
  // Check parameter
  if(newcapacity == 0 && this->capacity > 0 && this->counter > 0) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "setCapacity(): new capacity is zero but the table is not empty. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(newcapacity == this->capacity) return true; // Nothing to do

  // Is this a request to release resources?
  if(newcapacity == 0) {
    // Release all trees
    gcgAVLHASH **t = reinterpret_cast<gcgAVLHASH**>(this->table);
    for(unsigned int i = 0; i < this->capacity; i++, t++)
      if(*t != NULL) SAFE_DELETE(*t);  // Delete the AVL tree
    SAFE_FREE(this->table);
    this->capacity = 0;
    return true;
  }

  // Allocate the new table
  gcgCLASS **newtable = (gcgCLASS**) ALLOC(sizeof(gcgAVLHASH*) * newcapacity);
  if(newtable == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "setCapacity(): table allocation error. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Report adaptation
  gcgReport(GCG_REPORT_CODE(GCG_INFORMATION, GCG_DOMAIN_DATASTRUCTURE, GCG_ADAPTING), "setCapacity(): %d entries. Changing capacity from %d to %d. (%s:%d)", this->counter, this->capacity, newcapacity, basename((char*)__FILE__), __LINE__);

  // Save old table and commit new allocations
  gcgAVLHASH **oldtable = reinterpret_cast<gcgAVLHASH**>(this->table);
  uintptr_t oldcapacity = this->capacity;
  this->table = newtable;
  this->capacity = newcapacity;
  this->counter = 0;
  memset(this->table, 0, sizeof(gcgAVLHASH*) * this->capacity);

  // Scan current table to move nodes
  if(oldtable != NULL) {
    gcgAVLHASH **t = oldtable;
    for(unsigned int i = 0; i < oldcapacity; i++, t++)
      if(*t != NULL) { // Is there a tree in the table entry?
        // Get iterator and reset AVL tree
        gcgITERATOR *it = (*t)->detach();
        if(it != NULL) {  // The tree is not empty?
          // Scan the old tree and reinsert nodes in the new table
          gcgORDEREDNODE *node = static_cast<gcgORDEREDNODE*>(it->next());
          while(node != NULL) {
            // Compute table position using hash function
            long pos = this->hash(node) % this->capacity;

            // Check if there is an AVL tree
            gcgAVLHASH  *p = reinterpret_cast<gcgAVLHASH*>(this->table[pos]);
            if(p == NULL) {
              // Allocate new AVL tree
              p = new gcgAVLHASH(this);
              if(p == NULL) gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "setCapacity(): error allocating AVL subtree. (%s:%d)", basename((char*)__FILE__), __LINE__);
              else this->table[pos] = reinterpret_cast<gcgCLASS*>(p);
            }

            // It is not complementary to the if above: p can be allocated inside previous if
            if(p != NULL) {
              // Try to insert node in the subtree
              gcgORDEREDNODE *res = p->insert(node);
              if(res == node) this->counter++;
              else {
                SAFE_DELETE(node);
                gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INSERTIONFAILED), "setCapacity(): error inserting node in the new table. Node deleted. (%s:%d)", basename((char*)__FILE__), __LINE__);
              }
            }

            node = static_cast<gcgORDEREDNODE*>(it->next());
          }
          // Delete iterator
          SAFE_DELETE(it);
        }
        // Delete the AVL tree
        SAFE_DELETE(*t);
      }
    // Delete the old table
    SAFE_FREE(oldtable);
  }

  // Capacity adjusted
  return true;
}

gcgORDEREDNODE *gcgHASHTABLE::insert(gcgORDEREDNODE *newnode) {
   // Check parameter
  if(newnode == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "insert(): the new node is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  // Allocate table
  if(this->table == NULL) {
    // Compute initial capacity based on table loads
    if(this->minimumload > MINIMUM_TABLE_LOAD) this->setCapacity((unsigned int) (1.0 / this->minimumload));
    else this->setCapacity(MINIMUM_TABLE_SIZE);
  }

  // Compute table position using hash function
  long pos = this->hash(newnode) % this->capacity;

  // Check if there is a AVL tree
  gcgAVLHASH  *p = reinterpret_cast<gcgAVLHASH*>(this->table[pos]);
  if(p == NULL) {
    // Allocate new AVL tree
    p = new gcgAVLHASH(this);
    if(p == NULL) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "insert(): error allocating AVL subtree. (%s:%d)", basename((char*)__FILE__), __LINE__);
      return NULL;
    }
    this->table[pos] = reinterpret_cast<gcgCLASS*>(p);
  }

  // Try to insert node in the subtree
  gcgORDEREDNODE *res = p->insert(newnode);
  if(res == newnode) {
    this->counter++;

    // Successful insertion: check table MAXIMUM load
    if(this->maximumload > 0.0f && this->capacity > 0 && this->counter > 0)
      if(((float) this->counter / (float) this->capacity) > this->maximumload) {
        unsigned int newcapacity = (unsigned int) (((float) this->counter /
                                                  (this->maximumload + ((this->minimumload > 0.0f) ? this->minimumload : 0.0f))) * 2.0f);
        if(newcapacity > 0) this->setCapacity(newcapacity);
      }
  }

  return res;
}

gcgORDEREDNODE *gcgHASHTABLE::remove(gcgORDEREDNODE *key) {
  // Check parameter
  if(key == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "remove(): the key node is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  if(this->table != NULL) {
    // Compute table position using hash function
    long pos = this->hash(key) % this->capacity;

     // Check if there is a AVL tree
    gcgAVLHASH  *p = reinterpret_cast<gcgAVLHASH*>(this->table[pos]);
    if(p != NULL) {
      // Try to remove node from the subtree
      gcgORDEREDNODE *res = p->remove(key);
      if(res != NULL) {
        this->counter--;

        // Successful removal: check table MINIMUM load
        if(this->minimumload > 0.0f && this->capacity > 0 && this->counter > 0)
          if(((float) this->counter / (float) this->capacity) < this->minimumload) {
            unsigned int newcapacity = (unsigned int) (((float) this->counter /
                                                      (this->minimumload + ((this->maximumload > 0.0f) ? this->maximumload : this->minimumload + 1.0f))) * 2.0f);
            if(newcapacity > 0) this->setCapacity(newcapacity);
          }
      }
      return res;
    }
  }

  // Node not found
  gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "remove(): could not find the node to be removed. (%s:%d)", basename((char*)__FILE__), __LINE__);
  return NULL;
}


gcgORDEREDNODE *gcgHASHTABLE::search(gcgORDEREDNODE *key) {
  // Check parameter
  if(key == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "search(): the key node is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  if(this->table != NULL) {
    // Compute table position using hash function
    long pos = this->hash(key) % this->capacity;

     // Check if there is a AVL tree
    gcgAVLHASH  *p = reinterpret_cast<gcgAVLHASH*>(this->table[pos]);
    if(p != NULL) return p->search(key); // Search node in the subtree
  }

  // Node not found
  gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "search(): could not find the node to be removed. (%s:%d)", basename((char*)__FILE__), __LINE__);
  return NULL;
}


// Class for HASH TABLE iterations
class gcgHASHTABLEITERATOR : public gcgITERATOR {
  private:
    gcgITERATOR   **iterators;
    unsigned int  niterators;
    unsigned int  current;

  public:
    gcgHASHTABLEITERATOR(int mode, gcgHASHTABLE *t) {
      // Traverse table to discover how many iterators are needed
      this->current = 0;
      this->niterators = 0;
      gcgAVLHASH **p = reinterpret_cast<gcgAVLHASH**>(t->table);
      for(unsigned int i = 0; i < t->capacity; i++, p++)
        if(*p != NULL) this->niterators++;

      // Check iterator number
      if(this->niterators > 0) {
        // Allocate array of iterators
        this->iterators = (gcgITERATOR**) ALLOC(sizeof(gcgITERATOR*) * this->niterators);
        if(this->iterators == NULL) {
          gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgHASHTABLEITERATOR(): error allocating iterators. (%s:%d)", basename((char*)__FILE__), __LINE__);
          return;
        }

        // Scan table to get iterators
        memset(this->iterators, 0, sizeof(gcgITERATOR*) * this->niterators);
        gcgITERATOR **it = this->iterators;
        p = reinterpret_cast<gcgAVLHASH**>(t->table);
        for(unsigned int i = 0; i < t->capacity; i++, p++, it++)
          if(*p != NULL) *it = (gcgITERATOR*) (*p)->getIterator(mode);
      } else this->iterators = NULL;
    }

    ~gcgHASHTABLEITERATOR() {
      // Delete all iterators
      if(this->iterators != NULL && this->niterators > 0) {
        gcgITERATOR **it = this->iterators;
        for(unsigned int i = 0; i < this->niterators; i++, it++)
          if(*it != NULL) SAFE_DELETE(*it);
      }

      SAFE_FREE(this->iterators);
    }

    // Interface
    gcgDATA *next() {
      // Try to find a valid iterator
      while(this->current < this->niterators && this->iterators != NULL) {
        gcgITERATOR *it = this->iterators[this->current];
        if(it != NULL) {
          // Try to find next node
          gcgORDEREDNODE *node = (gcgORDEREDNODE*) it->next();
          if(node != NULL) return (gcgDATA*) node;
        }
        // Last iterator has ran out of nodes. Get next one.
        this->current++;
      }

      // No more nodes available
      return NULL;
    }
};

gcgITERATOR *gcgHASHTABLE::getIterator(int traversemode) {
  gcgHASHTABLEITERATOR *iterator = new gcgHASHTABLEITERATOR(traversemode, this);
  // Check allocation
  if(iterator == NULL) gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "getIterator(): could not allocate iterator. (%s:%d)", basename((char*)__FILE__), __LINE__);
  return (gcgITERATOR*) iterator;
}


bool gcgHASHTABLE::deleteAll() {
  if(this->table != NULL) {
    gcgAVLHASH**p = reinterpret_cast<gcgAVLHASH**>(this->table);
    // Scan table to delete all nodes
    for(unsigned int i = 0; i < this->capacity; i++, p++)
      if(*p != NULL) {
          // Explicitly delete all nodes of the subtree
          (*p)->deleteAll();
          // Delete AVL subtree
          SAFE_DELETE(*p);
      }

    // Delete table
    SAFE_FREE(this->table);
  }

  // Reset hash table
  this->capacity = 0;
  this->counter = 0;
  return true;
}



gcgITERATOR *gcgHASHTABLE::detach(int traversemode) {
  gcgITERATOR *it = this->getIterator(traversemode);

  if(this->table != NULL) {
    gcgAVLHASH**p = reinterpret_cast<gcgAVLHASH**>(this->table);
    // Scan table and reset all nodes
    for(unsigned int i = 0; i < this->capacity; i++, p++)
      if(*p != NULL) {
          gcgITERATOR *it = (*p)->detach(traversemode);
          // Discard subiterator
          SAFE_DELETE(it);
          // Delete AVL subtree
          SAFE_DELETE(*p);
      }

    // Delete table
    SAFE_FREE(this->table);
  }

  // Reset hash table
  this->capacity = 0;
  this->counter = 0;
  return it;
}

uintptr_t gcgHASHTABLE::getCounter() {
  return this->counter;
}
