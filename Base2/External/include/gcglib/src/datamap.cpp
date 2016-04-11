/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

    DATAMAP.CPP: classes for key/data pair mappings

    Marcelo Bernardes Vieira

**************************************************************************************/

#include "system.h"

// Minimum allocation blocks for the path
#define DEFAULT_ARRAY_BLOCK  1    // It must be > 1

// Bit extraction in MSB first order
#define GET_BIT(x, n) (((((char*) x)[(n) / CHAR_BIT]) >> ((CHAR_BIT - 1) - ((n) % CHAR_BIT))) & 0x01)

// PATRICIA macros: byte version (MSB first order)
#define CONSTRAINED_GET_BIT(x, b, l) ((b >= l) ? 1 : GET_BIT((x), b)) // MSB order with returning 1 after exausting key bits
#define CONSTRAINED_GET_CHILD(n, k, l) ((n)->child[CONSTRAINED_GET_BIT(k, (n)->bit, l)])

// PATRICIA macros: long version (MSB first order)
#define BITS_PER_LONG ((int) (sizeof(intptr_t) * CHAR_BIT))
#define GET_CHILD_LONG(n, k) ((n)->child[GET_BIT_LONG((k), (n)->bit)])
#define GET_BIT_LONG(x, b) (((int) (b) < (int) BITS_PER_LONG) ? (((intptr_t) (x)  >> ((BITS_PER_LONG - 1) - (b))) & ((intptr_t) 0x01)) : 1) // MSB first order with 1 after maximum size

// Uncompressed PATRICIA node but for compressed tree
// This organization is proposed in Chapter 15: Radix Search of the course of Algorithms and Data Structures, Princeton University,
// avaliable  at http://www.cs.princeton.edu/courses/archive/spring09/cos226/handouts/Algs3Ch15.pdf and developed by
// Robert Sedgewick since 1978.
typedef struct _GCG_PATRICIA_NODE {
  int           bit;
  GCGDIGITALKEY key;
  struct _GCG_PATRICIA_NODE *child[2];
  gcgDATA *data;
} GCG_PATRICIA_NODE;


gcgPATRICIAMAP::gcgPATRICIAMAP(unsigned int newkeysize) {
  root = NULL;
  counter = 0;
  keysize = newkeysize;
}

gcgPATRICIAMAP::~gcgPATRICIAMAP() {
  if(!this->deleteAll())
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_REMOVALERROR), "~gcgPATRICIAMAP(): could not remove all entries. (%s:%d)", basename((char*)__FILE__), __LINE__);
}

bool gcgPATRICIAMAP::setKeySize(unsigned int newkeysize) {
  if(root == NULL) {
    this->keysize = newkeysize;
    return true;
  }
  // Structure not empty: invalid operation
  gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_UNSUPPORTEDOPERATION), "setKeySize(): could not set new key size: trie not empty. (%s:%d)", basename((char*)__FILE__), __LINE__);
  return false;
}

uintptr_t gcgPATRICIAMAP::getCounter() {
  return this->counter;
}


// Class for PATRICIA iterators using the MSB first order = depth first traversal = O(log n) space cost
// The tree has arbitrary back links (nodes may act as internal, leaf, or both) and safely traversing nodes using
// the tree structure is costly high. In fact, the structure is a Directed Ciclic Graph emulating a tree.
class gcgPATRICIAMAPITERATOR : public gcgITERATOR {
  public:
    int               traversemode;
    GCG_PATRICIA_NODE *root;    // Points to the root: used only to delete nodes when detached
    bool              detach;   // True if the structure was detached from the gcgPATRICIAMAP: we need to free all GCG_PATRICIA_NODE nodes
    GCG_PATRICIA_NODE **stack;  // Stack to hold internal nodes
	  int               top;      // Top of the stack
	  int               maxnodes; // Maximum capacity of the stack
    GCG_PATRICIA_NODE *leaf;    // Hold up to one leaf to be returned in next() call

  public:
    gcgPATRICIAMAPITERATOR(int mode, GCG_PATRICIA_NODE *rootp, bool detached) : traversemode(mode), root(rootp), detach(detached) {
      // Detaching parameters
      this->stack = NULL;
      this->top = 0;
      this->maxnodes = 0;
      this->leaf = NULL;

      // Check tree
      if(root == NULL) {
        gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_MEMORY, GCG_BADPARAMETERS), "gcgPATRICIAMAPITERATOR(): tree is empty. (%s:%d)", basename((char*)__FILE__), __LINE__);
        return;
      }

      // Stack for iteration
      this->stack = (GCG_PATRICIA_NODE**) ALLOC(sizeof(GCG_PATRICIA_NODE*) * DEFAULT_ARRAY_BLOCK);
      if(this->stack != NULL) {
	      GCG_PATRICIA_NODE *node;

        // Init attributes
        this->maxnodes = DEFAULT_ARRAY_BLOCK;

        // Fill in stack of nodes: its average size is O(log n), where n is the maximum key size in bits. Worst case is O(n)
        node = root;
	      while(node != NULL) {
          // Check current array size
          CHECK_DYNAMIC_ARRAY_CAPACITY_VOID(this->stack, this->maxnodes, this->top + 1, GCG_PATRICIA_NODE*);

          // Put current node in the stack
          this->stack[(this->top)++] = node;

          // Go until find the parent of a leaf
          if(mode == 0) { // Ascending order (default)
            // Check bit 0 child: it MIGHT be null
            if(node->child[0] == NULL) node = NULL;
            else if(node->child[0]->bit > node->bit) node = node->child[0];
                 else node = NULL;
          } else { // Descending order
                // Check bit 1 child: it might NOT be null
                if(node->child[1]->bit > node->bit) node = node->child[1];
                else node = NULL;
              }
        }
      } else gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "gcgPATRICIAMAPITERATOR(): stack allocation error. (%s:%d)", basename((char*)__FILE__), __LINE__);

    }

    virtual ~gcgPATRICIAMAPITERATOR() {
      deleteDetached();
      SAFE_FREE(this->stack);
      this->top = 0;
      this->maxnodes = 0;
      this->traversemode = 0;
      this->detach = false;
      this->leaf = NULL;
    }

    // Delete the PATRICIA structure if detached
    void deleteDetached() {
      if(this->detach && this->root != NULL) {
        // Check stack
        if(this->stack == NULL) {
          this->stack = (GCG_PATRICIA_NODE**) ALLOC(sizeof(GCG_PATRICIA_NODE*) * DEFAULT_ARRAY_BLOCK);
          if(this->stack == NULL) {
            gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "deleteDetached(): stack allocation error. (%s:%d)", basename((char*)__FILE__), __LINE__);
            return;
          }
          this->maxnodes = DEFAULT_ARRAY_BLOCK;
        }

        // Init stack for a breath first deallocation
        this->top = 0;
        this->stack[this->top++] = this->root;
        int current = 0;

        // Fill stack
        // CAUTION: the iterator can have less nodes than the original tree if an internal node is used as root
        while(current < this->top) {
          GCG_PATRICIA_NODE *node = this->stack[current++];
          // Check current array size: we need at least two new positions
          CHECK_DYNAMIC_ARRAY_CAPACITY_VOID(this->stack, this->maxnodes, this->top + 1, GCG_PATRICIA_NODE*);

          // Check bit 0 child: it MIGHT be null
          if(node->child[0] != NULL)
            if(node->child[0]->bit > node->bit) this->stack[top++] = node->child[0];
          // Check bit 1 child: it might NOT be null
          if(node->child[1]->bit > node->bit) this->stack[top++] = node->child[1];
        }

        // Scan stack to delete objects
        for(current = 0; current < this->top; current++)
          SAFE_FREE(this->stack[current]); // With the children saved in the stack, we can delete the nodes

        // Done
        this->root = NULL;
        this->detach = false;
      }
    }

    // Interface for MSB iterations
    gcgDATA *next() {
      if(this->stack != NULL) {
        // Check if there is a remaining leaf
        if(this->leaf != NULL) {
          gcgDATA *data = this->leaf->data;
          this->leaf = NULL;
          return data;
        }

        // Now check if stack is empty and find next leaf
        while(this->top > 0) {
          // Retrieve top of the stack
          GCG_PATRICIA_NODE *node = this->stack[--this->top], *child;

          if(traversemode == 1) {
            //// DESCENDING ORDER
            // Fill the stack with rightmost children of the left child
            if(node->child[0] == NULL) child = NULL; // Check bit 0 child: it MIGHT be null
            else if(node->child[0]->bit > node->bit) child = node->child[0];
                  else child = NULL;

	          while(child != NULL) {
              // Check current array size
              CHECK_DYNAMIC_ARRAY_CAPACITY(this->stack, this->maxnodes, this->top + 1, GCG_PATRICIA_NODE*, NULL);

              // Put current node in the stack.
              this->stack[(this->top)++] = child;

              // Check bit 1 child: it might NOT be null
              if(child->child[1]->bit > child->bit) child = child->child[1];
              else child = NULL;
            }

            // If this is a leaf, we return its descendants
            if(node->bit >= node->child[1]->bit) {
              if(node->child[0] != NULL)
                if(node->bit >= node->child[0]->bit) this->leaf = node->child[0]; // Found also a left leaf
              // Found a right leaf
              return node->child[1]->data;
            }

            // Maybe found ONLY a left leaf
            if(node->child[0] != NULL)
              if(node->bit >= node->child[0]->bit) return node->child[0]->data;
          } else {
            //// ASCENDING ORDER (DEFAULT)
            // Fill the stack with leftmost children of the right child
            if(node->child[1]->bit > node->bit) child = node->child[1];
            else child = NULL;

	          while(child != NULL) {
              // Check current array size
              CHECK_DYNAMIC_ARRAY_CAPACITY(this->stack, this->maxnodes, this->top + 1, GCG_PATRICIA_NODE*, NULL);

              // Put current node in the stack
              this->stack[(this->top)++] = child;

              // Check bit 0 child: it MIGHT be null
              if(child->child[0] == NULL) child = NULL;
              else if(child->child[0]->bit > child->bit) child = child->child[0];
                    else child = NULL;
            }

            // If this is a leaf, we return its descendants
            if(node->child[0] != NULL)
              if(node->bit >= node->child[0]->bit) {
                if(node->bit >= node->child[1]->bit) this->leaf = node->child[1]; // Found also a right leaf
                // Found a left leaf
                return node->child[0]->data;
              }

            // Maybe found ONLY a right leaf
            if(node->bit >= node->child[1]->bit) return node->child[1]->data;
          }
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



gcgITERATOR *gcgPATRICIAMAP::getIterator(int traversemode){
  return (gcgITERATOR*) new gcgPATRICIAMAPITERATOR(traversemode, this->root, false);
}

gcgITERATOR *gcgPATRICIAMAP::suffixIterator(GCGDIGITALKEY key, unsigned int prefixsize, int traversemode){
  GCG_PATRICIA_NODE *node = this->root;
  int bitparent = -1;

  // Special case: key size is fixed but smaller than sizeof(intptr_t)
  if(this->keysize > 0 && this->keysize <= sizeof(intptr_t)) {
    int bitlength = this->keysize * CHAR_BIT - 1; // Adjusted bit length
    prefixsize = (prefixsize == 0) ? bitlength : prefixsize - 1; // Adjust prefixsize
    if(prefixsize > (unsigned int) bitlength) return NULL; // Prefix size is greater than maximum possible key size in bits
    uintptr_t mask = 0;

    // Make mask
    for(int i = 0; (unsigned int) i <= prefixsize; i++) mask |= ((uintptr_t) 0x01 << (bitlength - i));
    key.intkey &= mask;

    // Search by value
    while(node)
      if(node->bit > bitparent && (bitlength - ((BITS_PER_LONG - 1) - node->bit)) <= (int) prefixsize) { // Using MSB
        bitparent = node->bit;
        node = node->child[GET_BIT_LONG(key.intkey, bitparent)];
      } else
          // Node is "external": check if prefixkey is a prefix of it
          if((node->key.intkey & mask) == (uintptr_t) key.intkey) return (gcgITERATOR*) new gcgPATRICIAMAPITERATOR(traversemode, node, false); // Ok use this node as root
          else return NULL;

    return NULL;
  }

  // Check parameters
  if(key.pkey == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "suffixIterator(): null key. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  // Compute lengths
  int bitlength, length = (this->keysize > 0) ? this->keysize : (int) strlen((const char*) key.pkey);
  if(prefixsize == 0) bitlength = ((this->keysize == 0) ? length + 1 : length) * CHAR_BIT;
  else {
        if(prefixsize > (unsigned int) length && this->keysize > 0) return NULL; // Prefix size is greater than maximum possible key size
        else length = MIN((unsigned int) length, prefixsize);
        bitlength = length * CHAR_BIT;
      }

  // Traverse tree
  while(node)
    if(node->bit > bitparent && node->bit < bitlength) {  // Using MSB
      bitparent = node->bit;
      node = node->child[GET_BIT(key.pkey, bitparent)];
    } else
      // Node is "external": check if prefixkey is a prefix of it
      if(this->keysize == 0)
        if(strncmp((const char*) node->key.pkey, (const char*) key.pkey, length) == 0) return (gcgITERATOR*) new gcgPATRICIAMAPITERATOR(traversemode, node, false); // Ok use this node as root
        else return NULL;
      else if(memcmp(node->key.pkey, key.pkey, length) == 0) return (gcgITERATOR*) new gcgPATRICIAMAPITERATOR(traversemode, node, false); // Ok use this node as root
           else return NULL;

  // Failed
  return NULL;
}


gcgITERATOR *gcgPATRICIAMAP::detach(int traversemode) {
  gcgITERATOR *it = (gcgITERATOR*) new gcgPATRICIAMAPITERATOR(traversemode, this->root, true);

  // Reset the PATRICIA: keep key size
  this->root   = NULL;
  this->counter = 0;
  return it;
}

bool gcgPATRICIAMAP::deleteAll() {
  if(this->root != NULL) {
    // Sanity check
    if(this->counter == 0) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_INVALIDOBJECT), "deleteAll(): counter does not match tree size. (%s:%d)", basename((char*)__FILE__), __LINE__);
      return false;
    }

    // Array of nodes to be deleted: non-recursive breath first destruction
    uintptr_t top = 0, current = 0;
	  GCG_PATRICIA_NODE **path = (GCG_PATRICIA_NODE**) ALLOC(sizeof(GCG_PATRICIA_NODE*) * (this->counter + 1));

	  if(path == NULL) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "deleteAll(): stack allocation error. (%s:%d)", basename((char*)__FILE__), __LINE__);
      return false; // Memory allocation error
    }

    // Init stack for a breath first deallocation
    path[top++] = this->root;

    // Fill stack with all nodes: the tree has arbitrary back links (nodes may act as internal, leaf, or both) and safely deleting nodes using
    // the tree structure is costly high. In fact, the structure is a Directed Ciclic Graph emulating a tree.
    while(current < top) {
      GCG_PATRICIA_NODE *node = path[current++];

      // Check bit 0 child: it MIGHT be null
      if(node->child[0] != NULL)
        if(node->child[0]->bit > node->bit) path[top++] = node->child[0];
      // Check bit 1 child: it might NOT be null
      if(node->child[1]->bit > node->bit) path[top++] = node->child[1];
    }

    // Scan stack to delete objects
    for(current = 0; current < top; current++) {
      // Delete data
      SAFE_DELETE(path[current]->data);

      // With its children saved in the stack, we can delete the node
      SAFE_FREE(path[current]);
    }

    // Free path
    SAFE_FREE(path);
  }

  // Reset the PATRICIA: keep key size
  this->root = NULL;
  this->counter = 0;
  return true;
}

/*
// Utility to inspect the tree
static void tree_trav_str(GCG_PATRICIA_NODE  *t, int n, int bit) {
  char s[256];
  if(t == NULL) return;

  if(t->bit > bit) {
    tree_trav_str((GCG_PATRICIA_NODE*) t->child[1], n+1, t->bit);
    sprintf(s, "%%%ds%%s(%%d)----------->", n * 2);
    printf(s, "", t->key, t->bit);
    printf("\n\n");
    tree_trav_str((GCG_PATRICIA_NODE*) t->child[0], n+1, t->bit);
  }
}

static void tree_trav_int(GCG_PATRICIA_NODE  *t, int n, int bit) {
  char s[256];
  if(t == NULL) return;

  if(t->bit > bit) {
    tree_trav_int((GCG_PATRICIA_NODE*) t->child[1], n+1, t->bit);
    sprintf(s, "%%%ds%%d(%%d)----------->", n * 2);
    printf(s, "", t->key.ikey, t->bit);
    printf("\n\n");
    tree_trav_int((GCG_PATRICIA_NODE*) t->child[0], n+1, t->bit);
  }
}
*/


gcgDATA *gcgPATRICIAMAP::removeByValue(GCGDIGITALKEY key){
  // Prepare to search for closest string in the tree
  GCG_PATRICIA_NODE *node = this->root, *parent = NULL, *grand = NULL, *grandgrand = NULL;
  gcgDATA *data = NULL;
  int bitparent = -1, valueparent = -1, valuegrand = -1, valuegrandgrand = -1;

  // Search for the key in the tree
  while(node)
    if(node->bit > bitparent) {
      if(node->bit > BITS_PER_LONG) return NULL; // Not found: equality is useful to correctly find the NULL key
      valuegrandgrand = valuegrand;
      grandgrand = grand;
      valuegrand = valueparent;
      grand = parent;
      bitparent = node->bit;
      parent = node;
      valueparent = GET_BIT_LONG(key.intkey, bitparent); // Must be constrained to find the NULL key
      node = node->child[valueparent];
    } else
      // Node is "external": compare keys
      if(node->key.intkey == key.intkey) break; // Short fixed length key found
      else return NULL; // Not found

  // Case 1: tree is empty
  if(node == NULL) return NULL; // Nothing to do

  if(parent == node) {
    // Case 2: delete the only element of the tree
    if(grand == NULL) this->root = node->child[(valueparent) ? 0 : 1]; // New root is the other pointer
    else  // Case 3: it is a leaf (optimization: special case of next type)
          grand->child[valuegrand] = node->child[(valueparent) ? 0 : 1]; // Save the other pointer
  } else
        // Case 4: the grandparent is the node to be deleted
        if(node == grand) {
          // Parent takes the place of grandparent
          parent->child[valuegrand]    = parent->child[(valueparent) ? 0 : 1];
          int notvaluegrand = (valuegrand) ? 0 : 1;
          parent->child[notvaluegrand] = grand->child[notvaluegrand];
          parent->bit = grand->bit;
          // The grandgrandparent must refer to it
          if(grandgrand != NULL) grandgrand->child[valuegrandgrand] = parent;
          else this->root = parent; // It is new root
        } else {
            // We need to find the parent of the internal node
            GCG_PATRICIA_NODE *aux = this->root;
            grandgrand = NULL;
            while(aux != node) {
              grandgrand = aux;
              valuegrandgrand = GET_BIT_LONG(key.intkey, aux->bit); // Must be constrained to find the NULL key
              aux = aux->child[valuegrandgrand];
            }

            // Case 5: node is internal and the parent takes its place
            // Grand bypass parent
            grand->child[valuegrand] = parent->child[(valueparent) ? 0 : 1];

            // Parent takes the place of node
            parent->bit = node->bit;
            parent->child[0] = node->child[0];
            parent->child[1] = node->child[1];

            // The previous node must refer to it
            if(grandgrand != NULL) grandgrand->child[valuegrandgrand] = parent;
            else this->root = parent; // It is new root
          }

  // Everything ok, delete node and return data
  data = node->data;
  SAFE_FREE(node);
  this->counter--;
  return data;
}




gcgDATA *gcgPATRICIAMAP::remove(GCGDIGITALKEY key){
  // Special case: key size is fixed but smaller than sizeof(intptr_t)
  if(this->keysize > 0 && this->keysize <= sizeof(intptr_t)) return this->removeByValue(key);

  // Check parameters
  if(key.pkey == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "remove(): null key. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  // Prepare to search for closest string in the tree
  GCG_PATRICIA_NODE *node = this->root, *parent = NULL, *grand = NULL, *grandgrand = NULL;
  gcgDATA *data = NULL;
  int bitparent = -1, valueparent = -1, valuegrand = -1, valuegrandgrand = -1,
      bitlength = ((this->keysize > 0) ? this->keysize : (int) strlen((const char*) key.pkey) + 1) * CHAR_BIT;

  // Search for the key in the tree
  while(node)
    if(node->bit > bitparent) {
      if(node->bit > bitlength) return NULL; // Not found: equality is useful to correctly find the NULL key
      valuegrandgrand = valuegrand;
      grandgrand = grand;
      valuegrand = valueparent;
      grand = parent;
      bitparent = node->bit;
      parent = node;
      valueparent = CONSTRAINED_GET_BIT(key.pkey, bitparent, bitlength); // Must be constrained to find the NULL key
      node = node->child[valueparent];
    } else
      // Node is "external": compare keys
      if(this->keysize == 0)
        if(strcmp((const char*) node->key.pkey, (const char*) key.pkey) == 0) break; // String found
        else return NULL;  // Not found
      else if(memcmp(node->key.pkey, key.pkey, this->keysize) == 0) break; // Long fixed length key found
           else return NULL;  // Not found

  // Case 1: tree is empty
  if(node == NULL) return NULL; // Nothing to do

  if(parent == node) {
    // Case 2: delete the only element of the tree
    if(grand == NULL) this->root = node->child[(valueparent) ? 0 : 1]; // New root is the other pointer
    else  // Case 3: it is a leaf (optimization: special case of next type)
          grand->child[valuegrand] = node->child[(valueparent) ? 0 : 1]; // Save the other pointer
  } else
        // Case 4: the grandparent is the node to be deleted
        if(node == grand) {
          // Parent takes the place of grandparent
          parent->child[valuegrand]    = parent->child[(valueparent) ? 0 : 1];
          int notvaluegrand = (valuegrand) ? 0 : 1;
          parent->child[notvaluegrand] = grand->child[notvaluegrand];
          parent->bit = grand->bit;
          // The grandgrandparent must refer to it
          if(grandgrand != NULL) grandgrand->child[valuegrandgrand] = parent;
          else this->root = parent; // It is new root
        } else {
            // We need to find the parent of the internal node
            GCG_PATRICIA_NODE *aux = this->root;
            grandgrand = NULL;
            while(aux != node) {
              grandgrand = aux;
              valuegrandgrand = GET_BIT(key.pkey, aux->bit);
              aux = aux->child[valuegrandgrand];
            }

            // Case 5: node is internal and the parent takes its place
            // Grand bypass parent
            grand->child[valuegrand] = parent->child[(valueparent) ? 0 : 1];

            // Parent takes the place of node
            parent->bit = node->bit;
            parent->child[0] = node->child[0];
            parent->child[1] = node->child[1];

            // The previous node must refer to it
            if(grandgrand != NULL) grandgrand->child[valuegrandgrand] = parent;
            else this->root = parent; // It is new root
          }

  // Show the tree
//  if(this->keysize == 0) tree_trav_str(this->root, 0, -1);
//  else tree_trav_int(this->root, 0, -1);
//  getchar();

  // Everything ok, delete node and return data
  data = node->data;
  SAFE_FREE(node);
  this->counter--;
  return data;
}


static inline GCG_PATRICIA_NODE *createNodePatricia(GCGDIGITALKEY key, gcgDATA *value, int bit, int bitvalue, GCG_PATRICIA_NODE *nextnode) {
  GCG_PATRICIA_NODE *node = (GCG_PATRICIA_NODE*) ALLOC(sizeof(GCG_PATRICIA_NODE));
  if(node == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "createNodePatricia(): error allocating PATRICIA node. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  // Basic information
  node->key = key;
  node->bit = bit;
  node->data = value;
  node->child[bitvalue] = node;
  node->child[(bitvalue)?0:1] = nextnode;

  // Success
  return node;
}


// Faster insertion for keys with size fixed up to sizeof(long)
gcgDATA *gcgPATRICIAMAP::insertByValue(GCGDIGITALKEY key, gcgDATA *value) {
  // Prepare to search for closest string in the tree
  GCG_PATRICIA_NODE *node = this->root, *parent = NULL;
  int bitparent = -1;

  // Search for closest string in the tree
  while(node)
    if(node->bit > bitparent) {
      parent = node;
      bitparent = node->bit;
      node = GET_CHILD_LONG(node, key.intkey);
    } else
      // Node is "external": compare keys
      if(node->key.intkey == key.intkey) return node->data; // Short fixed length key found
      else break;

  // We have to find a suitable bit position for the new node
  int bitpos = 0;
  if(node != NULL) {
    // Here, "node" is "external": find differing bit
    while(GET_BIT_LONG(key.intkey, bitpos) == GET_BIT_LONG(node->key.intkey, bitpos) && bitpos < BITS_PER_LONG) bitpos++; // They must differ anywhere
  } else
      // Leaf is NULL: this is the leftmost entry
      while(GET_BIT_LONG(key.intkey, bitpos) == 0 && bitpos < BITS_PER_LONG) bitpos++;

  if(parent == NULL) {
    // Case 1: tree is empty
    if(node == NULL) this->root = createNodePatricia(key, value, bitpos, 1, NULL); // Create first node of the tree
    else  // Case 2: there is only one element in the tree
          this->root = createNodePatricia(key, value, bitpos, GET_BIT_LONG(key.intkey, bitpos), this->root); // Create new root
  } else
      // Case 3: it fits after external node's parent (optimization: special case of next type)
      if(bitparent < bitpos) {
        int bitvalue = GET_BIT_LONG(key.intkey, bitparent);
        parent->child[bitvalue] = createNodePatricia(key, value, bitpos, GET_BIT_LONG(key.intkey, bitpos), parent->child[bitvalue]); // Create new child
      } else {
            // Case 4: it fits between last valid node and external node's parent. Precise location must be found.
            bitparent = -1;
            parent = NULL;
            node = this->root;
            while(node)
              if(node->bit > bitparent) {
                if(node->bit < bitpos) {
                  bitparent = node->bit;
                  parent = node;  // Points to last valid internal node
                  node = GET_CHILD_LONG(node, key.pkey);
                } else {
                    // Found insertion point
                    if(parent == NULL) this->root = createNodePatricia(key, value, bitpos, GET_BIT_LONG(key.intkey, bitpos), this->root); // The root is the insertion point!
                    else {
                        // Found an intermediary node
                        int bitvalue = GET_BIT_LONG(key.intkey, bitparent);
                        parent->child[bitvalue] = createNodePatricia(key, value, bitpos, GET_BIT_LONG(key.intkey, bitpos), parent->child[bitvalue]); // Create new child
                      }

                    // Done
                    break;
                  }
              } else {
                    // Sanity check: we cannot reach an external node
                    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_INVALIDOBJECT), "insert(): corrupted data structure. (%s:%d)", basename((char*)__FILE__), __LINE__);
                    return NULL;
                  }
        }

  // Show the tree
//tree_trav_int(this->root, 0, -1);
//getchar();

  // Success
  this->counter++;
  return value;
}


gcgDATA *gcgPATRICIAMAP::insert(GCGDIGITALKEY key, gcgDATA *value) {
  // Check parameters
  if(value == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "insert(): cannot insert null data. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  // Special case: key size is fixed but smaller than sizeof(intptr_t)
  if(this->keysize > 0 && this->keysize <= sizeof(intptr_t)) return this->insertByValue(key, value);

  // Check null keys for null terminated strings (keysize = 0) or long fixed size keys (keysize > sizeof(long))
  if(key.pkey == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "insert(): cannot insert null key pointer when key size is %d. (%s:%d)", this->keysize, basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  // Prepare to search for closest string in the tree
  GCG_PATRICIA_NODE *node = this->root, *parent = NULL;
  int bitparent = -1, bitlength = ((this->keysize > 0) ? this->keysize : (int) strlen((const char*) key.pkey) + 1) * CHAR_BIT;

  // Search for closest string in the tree
  while(node)
    if(node->bit > bitparent) {
      parent = node;
      bitparent = node->bit;
      node = CONSTRAINED_GET_CHILD(node, key.pkey, bitlength); // We cannot access beyond new key's length
    } else
      // Node is "external": compare keys
      if(this->keysize == 0)
        if(strcmp((const char*) node->key.pkey, (const char*) key.pkey) == 0) return node->data; // String found
        else break;
      else if(memcmp(node->key.pkey, key.pkey, this->keysize) == 0) return node->data; // Long fixed length key found
           else break;

  // We have to find a suitable bit position for the new node
  int bitpos = 0;
  if(node != NULL) {
    // Here, "node" is "external": find differing bit
    void *leafkey = node->key.pkey;
    int leaflength = ((this->keysize > 0) ? this->keysize : (int) strlen((const char*) leafkey) + 1) * CHAR_BIT;
    while(CONSTRAINED_GET_BIT(key.pkey, bitpos, bitlength) == CONSTRAINED_GET_BIT(leafkey, bitpos, leaflength) && bitpos < bitlength) bitpos++; // They must differ anywhere
  } else
      // Leaf is NULL: this is the leftmost entry
      while(CONSTRAINED_GET_BIT(key.pkey, bitpos, bitlength) == 0) bitpos++; // CONSTRAINED_GET_BIT will check bitlength and return 1

  if(parent == NULL) {
    // Case 1: tree is empty
    if(node == NULL) this->root = createNodePatricia(key, value, bitpos, 1, NULL); // Create first node of the tree
    else  // Case 2: there is only one element in the tree
          this->root = createNodePatricia(key, value, bitpos, CONSTRAINED_GET_BIT(key.pkey, bitpos, bitlength), this->root); // Create new root
  } else
      // Case 3: it fits after external node's parent (optimization: special case of next type)
      if(bitparent < bitpos) {
        int bitvalue = CONSTRAINED_GET_BIT(key.pkey, bitparent, bitlength);
        parent->child[bitvalue] = createNodePatricia(key, value, bitpos, CONSTRAINED_GET_BIT(key.pkey, bitpos, bitlength), parent->child[bitvalue]); // Create new child
      } else {
            // Case 4: it fits between last valid node and external node's parent. Precise location must be found.
            bitparent = -1;
            parent = NULL;
            node = this->root;
            while(node)
              if(node->bit > bitparent) {
                if(node->bit < bitpos) {
                  bitparent = node->bit;
                  parent = node;  // Points to last valid internal node
                  node = CONSTRAINED_GET_CHILD(node, key.pkey, bitlength);
                } else {
                    // Found insertion point
                    if(parent == NULL) this->root = createNodePatricia(key, value, bitpos, CONSTRAINED_GET_BIT(key.pkey, bitpos, bitlength), this->root); // The root is the insertion point!
                    else {
                        // Found an intermediary node
                        int bitvalue = CONSTRAINED_GET_BIT(key.pkey, bitparent, bitlength);
                        parent->child[bitvalue] = createNodePatricia(key, value, bitpos, CONSTRAINED_GET_BIT(key.pkey, bitpos, bitlength), parent->child[bitvalue]); // Create new child
                      }

                    // Done
                    break;
                  }
              } else {
                    // Sanity check: we cannot reach an external node
                    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_INVALIDOBJECT), "insert(): corrupted data structure. (%s:%d)", basename((char*)__FILE__), __LINE__);
                    return NULL;
                  }
        }


  // Show the tree
//  if(this->keysize == 0) tree_trav_str(this->root, 0, -1);
//  getchar();

  // Success
  this->counter++;
  return value;
}

gcgDATA *gcgPATRICIAMAP::search(GCGDIGITALKEY key) {
  // Special case: key size is fixed but smaller than sizeof(intptr_t)
  if(this->keysize > 0 && this->keysize <= sizeof(intptr_t)) {
    // Faster searchings keys with size fixed up to sizeof(intptr_t)
    GCG_PATRICIA_NODE *node = this->root;
    int bitparent = -1;
    while(node)
      if(node->bit > bitparent) {
        bitparent = node->bit;
        node = node->child[GET_BIT_LONG(key.intkey, bitparent)];
      } else
        // Node is "external": compare keys
        if(node->key.intkey == key.intkey) return node->data; // Short fixed length key found
        else return NULL;

    // Not found
    return NULL;
  }

  // Check parameters
  if(key.pkey == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "search(): null key. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  // Traverse tree
  GCG_PATRICIA_NODE *node = this->root;
  int bitlength = ((this->keysize > 0) ? this->keysize : (int) strlen((const char*) key.pkey) + 1) * CHAR_BIT, bitparent = -1;
  while(node)
    if(node->bit > bitparent && node->bit < bitlength) {
      bitparent = node->bit;
      node = node->child[GET_BIT(key.pkey, bitparent)];
    } else
      // Node is "external": compare keys
      if(this->keysize == 0)
        if(strcmp((const char*) node->key.pkey, (const char*) key.pkey) == 0) return node->data; // String found
        else return NULL;
      else if(memcmp(node->key.pkey, key.pkey, this->keysize) == 0) return node->data; // Long fixed length key found
           else return NULL;

  // Not found
  return NULL;
}

