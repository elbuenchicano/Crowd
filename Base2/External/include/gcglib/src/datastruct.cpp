/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

    DATASTRUCT.CPP: classes for general purpose data structures

    Marcelo Bernardes Vieira

**************************************************************************************/

#include "system.h"


//////////////////////////////////////////////////////////////////////
// gcgLINKEDLIST: implements a single linked list
//////////////////////////////////////////////////////////////////////

gcgITERATOR::~gcgITERATOR() {
}

gcgLINKEDLIST::gcgLINKEDLIST() {
  // Init list
  this->first   = NULL;
  this->last    = NULL;
  this->counter = 0;
  // No error
}

gcgLINKEDLIST::~gcgLINKEDLIST() {
  if(!this->deleteAll())
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_REMOVALERROR), "~gcgLINKEDLIST(): could not remove all entries. (%s:%d)", basename((char*)__FILE__), __LINE__);
}

bool gcgLINKEDLIST::insertFirst(gcgLINK *node) {
  // Check parameter
  if(node == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "insertFirst(): the node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Insert new node
  if(this->first == NULL) this->last = node;
  node->next = first;
  this->first = node;
  this->counter++;

  // No error
  return true;
}

bool gcgLINKEDLIST::insertLast(gcgLINK *node) {
  // Check parameter
  if(node == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "insertLast(): the node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Fill the entry
  node->next = NULL;
  if(this->last == NULL) this->first = node;
  else this->last->next = node;
  this->last = node;
  this->counter++;

  // No error
  return true;
}

gcgLINK* gcgLINKEDLIST::dequeueFirst() {
  // Remove entry
  gcgLINK *data = this->first;

  if(data) {
    this->first = this->first->next;
    if(this->first == NULL) this->last = NULL;
    this->counter--;
  } else {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "dequeueFirst(): the linked list is empty. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  return data;
}


gcgLINK* gcgLINKEDLIST::dequeueLast() {
  // Remove entry
  gcgLINK *data = this->last;

  if(data) {
    // Find previous node: O(n) operation
    gcgLINK *previous = NULL, *p;
    for(p = this->first; p != NULL; p = p->next)
      if(p->next == data) previous = p;

    if(previous == NULL) this->first = this->last = NULL;
    else {
        this->last = previous;
        previous->next = NULL;
      }
    this->counter--;
  } else {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "dequeueLast(): the linked list is empty. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  return data;
}

bool gcgLINKEDLIST::remove(gcgLINK *node) {
  if(this->first == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "remove(): the linked list is empty. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameter
  if(node == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "remove(): the node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Find previous node: O(n) operation
  gcgLINK *previous = NULL, *p;
  for(p = this->first; p != NULL; p = p->next)
    if(p->next == node) {
      previous = p;
      break;
    }

  if(node->next == NULL) this->last = previous;
  if(previous == NULL) this->first = node->next;
  else previous->next = node->next;

  this->counter--;
  node->next = NULL;

  return true;
}

uintptr_t gcgLINKEDLIST::getCounter() {
  return this->counter;
}

bool gcgLINKEDLIST::deleteAll() {
  // Free all elements
  while(this->first != NULL) {
    gcgLINK *p = this->first->next;
    SAFE_DELETE(this->first);
    this->first = p;
  }

  // Reset list
  this->first = this->last = NULL;
  return true;
}

gcgITERATOR *gcgLINKEDLIST::detach(int traversemode) {
  gcgITERATOR *it = getIterator(traversemode);
  // Reset list
  this->first = this->last = NULL;
  return it;
}

bool gcgLINKEDLIST::insertAfter(gcgLINK *newnode, gcgLINK *node) {
  // Check parameter
  if(newnode == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "insertAfter(): the new node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(node == NULL) {
    // Same as insertLast()
    newnode->next = NULL;
    if(this->last == NULL) this->first = newnode;
    else this->last->next = newnode;
    this->last = newnode;
  } else {
      newnode->next = node->next;
      node->next = newnode;
      if(node == this->last) this->last = newnode;
    }

  this->counter++;
  // No error
  return true;
}


bool gcgLINKEDLIST::insertBefore(gcgLINK *newnode, gcgLINK *node) {
  // Check parameter
  if(newnode == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "insertBefore(): the new node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(node == NULL) {
    // Same effect as insertFirst()
    if(this->first == NULL) this->last = newnode;
    newnode->next = first;
    this->first = newnode;
  } else {
      // Find previous node: O(n) operation
      gcgLINK *previous = NULL, *p;
      for(p = this->first; p != NULL; p = p->next)
        if(p->next == node) {
          previous = p;
          break;
        }

      newnode->next = node;
      if(previous != NULL) previous->next = newnode;
      else this->first = newnode;
    }

  this->counter++;
  // No error
  return true;
}


bool gcgLINKEDLIST::moveToLast(gcgLINK *node) {
  if(this->first == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "moveToLast(): the linked list is empty. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameter
  if(node == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "moveToLast(): the node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(this->last != node) {
    // Remove node
    // Find previous node: O(n) operation
    gcgLINK *previous = NULL, *p;
    for(p = this->first; p != NULL; p = p->next)
      if(p->next == node) {
        previous = p;
        break;
      }

    if(node->next == NULL) this->last = previous;
    if(previous == NULL) this->first = node->next;
    else previous->next = node->next;

    // Same effect as insertLast()
    node->next = NULL;
    if(this->last == NULL) this->first = node;
    else this->last->next = node;
    this->last = node;
  }

  return true;
}


bool gcgLINKEDLIST::moveToFirst(gcgLINK *node) {
  if(this->first == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "moveToFirst(): the linked list is empty. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameter
  if(node == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "moveToFirst(): the node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(this->first != node) {
    // Remove node
    // Find previous node: O(n) operation
    gcgLINK *previous = NULL, *p;
    for(p = this->first; p != NULL; p = p->next)
      if(p->next == node) {
        previous = p;
        break;
      }

    if(node->next == NULL) this->last = previous;
    if(previous == NULL) this->first = node->next;
    else previous->next = node->next;

    // Same effect as insertFirst()
    if(this->first == NULL) this->last = node;
    node->next = first;
    this->first = node;
  }

  return true;
}

bool gcgLINKEDLIST::switchNodes(gcgLINK *node1, gcgLINK *node2) {
  if(this->first == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "switchNodes(): the linked list is empty. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameter
  if(node1 == NULL || node2 == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "switchNodes(): a node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(node1 == node2) return true; // Nothing to do

  // Find previous node: O(n) operation
  gcgLINK *previous1 = NULL, *previous2 = NULL, *aux;
  for(aux = this->first; aux != NULL && (previous1 == NULL || previous2 == NULL); aux = aux->next) {
    if(aux->next == node1) previous1 = aux;
    if(aux->next == node2) previous2 = aux;
   }

  // Switch all pointers
  // Detect consecutive nodes
  aux = node2->next;
  if(node1->next == node2) node2->next = node1;
  else node2->next = node1->next;
  if(aux == node1) node1->next = node2;
  else node1->next = aux;

  if(previous1 != NULL && previous1 != node2) previous1->next = node2;
  if(previous2 != NULL && previous2 != node1) previous2->next = node1;

  if(previous1 == NULL) this->first = node2;
  if(previous2 == NULL) this->first = node1;
  if(node1->next == NULL) this->last = node1;
  if(node2->next == NULL) this->last = node2;

  return true;
}


// Class for linked list iterations
class gcgLINKEDLISTITERATOR : public gcgITERATOR {
  private:
    int     traversemode;
    gcgLINK *nextnode;  // Next node to be returned.

  public:
    gcgLINKEDLISTITERATOR(int mode, gcgLINK *first) : traversemode(mode), nextnode(first) {}
    ~gcgLINKEDLISTITERATOR() {}

    // Interface
    gcgDATA  *next() {
      gcgDATA *p = static_cast<gcgDATA*>(this->nextnode);
      if(p != NULL) this->nextnode = nextnode->next;
      return p;
    }
};

gcgITERATOR *gcgLINKEDLIST::getIterator(int traversemode) {
  gcgLINKEDLISTITERATOR *iterator = new gcgLINKEDLISTITERATOR(traversemode, this->first);
  // Check allocation
  if(iterator == NULL) gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "getIterator(): could not allocate iterator. (%s:%d)", basename((char*)__FILE__), __LINE__);
  return (gcgITERATOR*) iterator;
}





//////////////////////////////////////////////////////////////////////
// gcgDOUBLELINKEDLIST: implements a double linked list
//////////////////////////////////////////////////////////////////////

gcgDOUBLELINKEDLIST::gcgDOUBLELINKEDLIST() {
  // Init list
  this->first   = NULL;
  this->last    = NULL;
  this->counter = 0;
  // No error
}

gcgDOUBLELINKEDLIST::~gcgDOUBLELINKEDLIST() {
  if(!this->deleteAll())
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_REMOVALERROR), "~gcgDOUBLELINKEDLIST(): could not remove all entries. (%s:%d)", basename((char*)__FILE__), __LINE__);
}

bool gcgDOUBLELINKEDLIST::insertFirst(gcgLINK *node) {
  // Check parameter
  if(node == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "insertFirst(): the node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Insert new node
  ((gcgDOUBLELINK*) node)->previous = NULL;
  if(this->first != NULL) ((gcgDOUBLELINK*) this->first)->previous = node;
  else this->last = node;
  node->next = first;
  this->first = node;
  this->counter++;

  // No error
  return true;
}

bool gcgDOUBLELINKEDLIST::insertLast(gcgLINK *node) {
  // Check parameter
  if(node == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "insertLast(): the node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Fill the entry
  node->next = NULL;
  if(this->last != NULL) this->last->next = node;
  else this->first = node;
  ((gcgDOUBLELINK*) node)->previous = this->last;
  this->last = node;
  this->counter++;

  // No error
  return true;
}

gcgLINK* gcgDOUBLELINKEDLIST::dequeueFirst() {
  // Remove entry
  gcgLINK *data = this->first;

  if(data) {
    this->first = this->first->next;
    if(this->first == NULL) this->last = NULL;
    else ((gcgDOUBLELINK*) this->first)->previous = NULL;
    ((gcgDOUBLELINK*) data)->previous = data->next = NULL;
    this->counter--;
  } else {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "dequeueFirst(): the linked list is empty. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  return data;
}


gcgLINK* gcgDOUBLELINKEDLIST::dequeueLast() {
  // Remove entry
  gcgLINK *data = this->last;

  if(data) {
    this->last = ((gcgDOUBLELINK*) this->last)->previous;
    if(this->last == NULL) this->first = NULL;
    else ((gcgDOUBLELINK*) this->last)->next = NULL;
    ((gcgDOUBLELINK*) data)->previous = data->next = NULL;
    this->counter--;
  } else {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "dequeueLast(): the linked list is empty. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return NULL;
  }

  return data;
}

bool gcgDOUBLELINKEDLIST::remove(gcgLINK *node) {
  if(this->first == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "remove(): the linked list is empty. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameter
  if(node == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "remove(): the node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(((gcgDOUBLELINK*) node)->previous != NULL) ((gcgDOUBLELINK*) node)->previous->next = node->next;
  else this->first = node->next;

  if(node->next != NULL) ((gcgDOUBLELINK*) node->next)->previous = ((gcgDOUBLELINK*) node)->previous;
  else this->last = ((gcgDOUBLELINK*) node)->previous;

  this->counter--;
  ((gcgDOUBLELINK*) node)->previous = node->next = NULL;

  return true;
}

uintptr_t gcgDOUBLELINKEDLIST::getCounter() {
  return this->counter;
}

bool gcgDOUBLELINKEDLIST::deleteAll() {
  // Free all elements
  while(this->first != NULL) {
    gcgLINK *p = this->first->next;
    SAFE_DELETE(this->first);
    this->first = p;
  }

  // Reset list
  this->first = this->last = NULL;
  return true;
}

gcgITERATOR *gcgDOUBLELINKEDLIST::detach(int traversemode) {
  gcgITERATOR *it = getIterator(traversemode);
  // Reset linst
  this->first = this->last = NULL;
  return it;
}

bool gcgDOUBLELINKEDLIST::insertAfter(gcgLINK *newnode, gcgLINK *node) {
  // Check parameter
  if(newnode == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "insertAfter(): the new node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(node == NULL) {
    // Same effect as insertLast()
    newnode->next = NULL;
    if(this->last != NULL) this->last->next = newnode;
    else this->first = newnode;
    ((gcgDOUBLELINK*) newnode)->previous = this->last;
    this->last = newnode;
  } else {
      if(((gcgDOUBLELINK*) node)->next) ((gcgDOUBLELINK*) node->next)->previous = newnode;
      else this->last = newnode;
      ((gcgDOUBLELINK*) newnode)->next = ((gcgDOUBLELINK*) node)->next;
      ((gcgDOUBLELINK*) node)->next = newnode;
      ((gcgDOUBLELINK*) newnode)->previous = node;
    }

  this->counter++;
  // No error
  return true;
}


bool gcgDOUBLELINKEDLIST::insertBefore(gcgLINK *newnode, gcgLINK *node) {
  // Check parameter
  if(newnode == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "insertBefore(): the new node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(node == NULL) {
    // Same effect as insertFirst()
    ((gcgDOUBLELINK*) newnode)->previous = NULL;
    if(this->first != NULL) ((gcgDOUBLELINK*) this->first)->previous = newnode;
    else this->last = newnode;
    newnode->next = first;
    this->first = newnode;
  } else {
      if(((gcgDOUBLELINK*) node)->previous) ((gcgDOUBLELINK*) node)->previous->next = newnode;
      else this->first = newnode;
      ((gcgDOUBLELINK*) newnode)->previous = ((gcgDOUBLELINK*) node)->previous;
      ((gcgDOUBLELINK*) node)->previous = newnode;
      newnode->next = node;
    }

  this->counter++;
  // No error
  return true;
}


bool gcgDOUBLELINKEDLIST::moveToLast(gcgLINK *node) {
  if(this->first == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "moveToLast(): the linked list is empty. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameter
  if(node == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "moveToLast(): the node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(this->last != node) {
    // Remove node
    if(node->next == NULL) this->last = ((gcgDOUBLELINK*) node)->previous;
    else ((gcgDOUBLELINK*) node->next)->previous = ((gcgDOUBLELINK*) node)->previous;
    if(((gcgDOUBLELINK*) node)->previous == NULL) this->first = node->next;
    else ((gcgDOUBLELINK*) node)->previous->next = node->next;


    // Same effect as insertLast()
    node->next = NULL;
    if(this->last != NULL) this->last->next = node;
    else this->first = node;
    ((gcgDOUBLELINK*) node)->previous = this->last;
    this->last = node;
  }

  return true;
}


bool gcgDOUBLELINKEDLIST::moveToFirst(gcgLINK *node) {
  if(this->first == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "moveToFirst(): the linked list is empty. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameter
  if(node == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "moveToFirst(): the node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(this->first != node) {
    // Remove node
    if(((gcgDOUBLELINK*) node)->previous != NULL) ((gcgDOUBLELINK*) node)->previous->next = node->next;
    else {
      this->first = node->next;
      if(node->next != NULL) ((gcgDOUBLELINK*) node->next)->previous = NULL;
    }

    if(node->next != NULL) ((gcgDOUBLELINK*) node->next)->previous = ((gcgDOUBLELINK*) node)->previous;
    else {
      this->last = ((gcgDOUBLELINK*) node)->previous;
      if(((gcgDOUBLELINK*) node)->previous != NULL) ((gcgDOUBLELINK*) node)->previous->next = NULL;
    }

    // Same effect as insertFirst()
    ((gcgDOUBLELINK*) node)->previous = NULL;
    if(this->first != NULL) ((gcgDOUBLELINK*) this->first)->previous = node;
    else this->last = node;
    node->next = first;
    this->first = node;
  }

  return true;
}

bool gcgDOUBLELINKEDLIST::switchNodes(gcgLINK *node1, gcgLINK *node2) {
  if(this->first == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_DATASTRUCTURE, GCG_INFORMATIONPROBLEM), "switchNodes(): the linked list is empty. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  // Check parameter
  if(node1 == NULL || node2 == NULL) {
    gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_DATASTRUCTURE, GCG_BADPARAMETERS), "switchNodes(): a node parameter is NULL. (%s:%d)", basename((char*)__FILE__), __LINE__);
    return false;
  }

  if(node1 == node2) return true; // Nothing to do

  // Switch all pointers
  gcgLINK *aux = ((gcgDOUBLELINK*) node1)->previous;
  if(((gcgDOUBLELINK*) node2)->previous == node1) ((gcgDOUBLELINK*) node1)->previous = node2;
  else ((gcgDOUBLELINK*) node1)->previous = ((gcgDOUBLELINK*) node2)->previous;
  if(aux == node2) ((gcgDOUBLELINK*) node2)->previous = node1;
  else ((gcgDOUBLELINK*) node2)->previous = aux;

  aux = node1->next;
  if(node2->next == node1) node1->next = node2;
  else node1->next = node2->next;
  if(aux == node2) node2->next = node1;
  else node2->next = aux;

  // Check neighborhood for node1
  if(((gcgDOUBLELINK*) node1)->previous != NULL) ((gcgDOUBLELINK*) node1)->previous->next = node1;
  else this->first = node1;
  if(node1->next != NULL) ((gcgDOUBLELINK*) node1->next)->previous = node1;
  else this->last = node1;

  // Check neighborhood for node2
  if(((gcgDOUBLELINK*) node2)->previous != NULL) ((gcgDOUBLELINK*) node2)->previous->next = node2;
  else this->first = node2;
  if(node2->next != NULL) ((gcgDOUBLELINK*) node2->next)->previous = node2;
  else this->last = node2;

  return true;
}


// Class for linked list iterations
class gcgDOUBLELINKEDLISTITERATOR : public gcgITERATOR {
  private:
    int           traversemode;
    gcgDOUBLELINK *nextnode;  // Next node to be returned.

  public:
    gcgDOUBLELINKEDLISTITERATOR(int mode, gcgDOUBLELINK *firstnode) : traversemode(mode), nextnode(firstnode) {}
    ~gcgDOUBLELINKEDLISTITERATOR() {}

    // Interface
    gcgDATA  *next() {
      gcgDOUBLELINK *p = static_cast<gcgDOUBLELINK*>(this->nextnode);
      if(p != NULL) {
        if(traversemode == 1) this->nextnode = (gcgDOUBLELINK*) nextnode->previous; // Tail to Head order
        else this->nextnode = (gcgDOUBLELINK*) nextnode->next; // Head to Tail order (default)
      }

      return static_cast<gcgDATA*>(p);
    }
};

gcgITERATOR *gcgDOUBLELINKEDLIST::getIterator(int traversemode) {
  gcgDOUBLELINKEDLISTITERATOR *iterator;

  if(traversemode == 1) iterator = new gcgDOUBLELINKEDLISTITERATOR(1, (gcgDOUBLELINK*) this->last);
  else iterator = new gcgDOUBLELINKEDLISTITERATOR(0, (gcgDOUBLELINK*) this->first);

  // Check allocation
  if(iterator == NULL) gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "getIterator(): could not allocate iterator. (%s:%d)", basename((char*)__FILE__), __LINE__);
  return (gcgITERATOR*) iterator;
}
