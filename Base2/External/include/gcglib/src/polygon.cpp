/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS, IMAGE AND VISION
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

   POLYGON.CPP: functions for polygon processing.

   Patricia Cordeiro Pereira Pampanelli
   Marcelo Bernardes Vieira
**************************************************************************************/

#include "system.h"

// Vertex block allocation
#define GCG_VERTEX_BLOCK_SIZE             1024
#define GCG_VERTEX_HALF_EDGE_BLOCK_SIZE   16

// Star block allocation
#define GCG_POLYGON_STAR_SIZE             16
#define GCG_POLYGON_VERTEX_STAR_SIZE      16

/////////////////////////////////////////////////////////////
// Empty object construction
/////////////////////////////////////////////////////////////
gcgPOLYGONMESH::gcgPOLYGONMESH() {
  // Vertices and polygons
  nvertices   = 0;               // Number of vertices of this object
  vertices    = NULL;            // Linked list of vertices
  npolygons   = 0;               // Number of polygons of this object
  polygons    = NULL;            // Linked list of polygons
}

/////////////////////////////////////////////////////////////
// Object destruction. Releases all resources.
/////////////////////////////////////////////////////////////
gcgPOLYGONMESH::~gcgPOLYGONMESH() {
  destroyPolygon();
}

/////////////////////////////////////////////////////////////
// Release all resources
/////////////////////////////////////////////////////////////
void gcgPOLYGONMESH::destroyPolygon() {
  // Frees all resources
  for(gcgPOLYGONVERTEX *p = vertices, *a; p != NULL;) {
    a = p;
    p = p->nextVertex;
    for(unsigned int i = 0; i < p->nhalf_edges; i++) SAFE_FREE(p->half_edges[i]);
    SAFE_FREE(a->half_edges);
    SAFE_DELETE(a);
    if(p == vertices) break; // Double linked list
  }
  for(gcgPOLYGON *p = polygons, *a; p != NULL;) {
    a = p;
    p = p->nextPolygon;
    SAFE_DELETE(a);
    if(p == polygons) break; // Double linked list
  }

  // Vertices and polygons
  nvertices = 0;
  vertices  = NULL;
  npolygons = 0;
  polygons  = NULL;
}

/////////////////////////////////////////////////////////////
// Adds a new and empty polygon.
/////////////////////////////////////////////////////////////
bool gcgPOLYGONMESH::addPolygon(gcgPOLYGON *polygon) {
   // Adds a new empty polygon
   polygon->half_edge = NULL;
   polygon->nvertices = 0;

   // Update the list of polygons
   if(polygons) {
     polygons->prevPolygon->nextPolygon = polygon;
     polygon->prevPolygon = polygons->prevPolygon;
     polygons->prevPolygon = polygon;
   } else{
     polygons = polygon;
     polygon->nextPolygon = polygon;
     polygon->prevPolygon = polygon;
   }

   npolygons++;

   // Return the pointer of the polygon
   return true;
}

/////////////////////////////////////////////////////////////
// Adds a new and unused vertex.
// Returns the pointer to the vertex created.
/////////////////////////////////////////////////////////////
bool gcgPOLYGONMESH::addVertex(gcgPOLYGONVERTEX *new_vertex) {
   // Add a new vertex
   new_vertex->maxhalf_edges = GCG_VERTEX_HALF_EDGE_BLOCK_SIZE;
   new_vertex->half_edges = (GCGHALFEDGE**) ALLOC(sizeof(GCGHALFEDGE*) * GCG_VERTEX_HALF_EDGE_BLOCK_SIZE);
   if(!new_vertex->half_edges) return false;
   new_vertex->nhalf_edges = 0;

   // Init attributes
   memset(new_vertex->half_edges, 0, sizeof(GCGHALFEDGE*) * GCG_VERTEX_HALF_EDGE_BLOCK_SIZE);
   new_vertex->nextVertex = vertices;
   new_vertex->prevVertex = NULL;

   // Update the list of vertices
   if(vertices) vertices->prevVertex = new_vertex;
   vertices = new_vertex;

   // Vertex added
   nvertices++;
   return true;
}

/////////////////////////////////////////////////////////////
// Adds a new and unused vertex.
// Returns the pointer to the vertex added
/////////////////////////////////////////////////////////////
gcgPOLYGONVERTEX* gcgPOLYGONMESH::findSimilarVertex(gcgPOLYGONVERTEX *vertex){
   // Check parameters
   if(!vertex) return NULL;

   // Eliminate redundant vertices
   // Get the first vertex
   gcgPOLYGONVERTEX *current_vertex = vertices;
   for(register unsigned int i = 0; i < nvertices; i++) {
      if(vertex->isEqual(current_vertex)) return current_vertex;
      current_vertex = current_vertex->nextVertex;
   }

   return NULL;
}

/////////////////////////////////////////////////////////////
// Appends a new vertex to an existing polygon
/////////////////////////////////////////////////////////////
bool gcgPOLYGONMESH::addPolygonVertex(gcgPOLYGON *ipolygon, gcgPOLYGONVERTEX *vertex, unsigned int newvertexIndex) {
   // Check parameters
   if(!ipolygon || !vertex) return false;

   // It represents the half-edge to what will be added later
   GCGHALFEDGE* current_half_edge = NULL;

   // Get the position where the new vertex will be added
   if(ipolygon->half_edge) {
      current_half_edge = ipolygon->half_edge;
      for(unsigned int i = 0; i < newvertexIndex && i < ipolygon->nvertices; i++)
         current_half_edge = current_half_edge->nextHalfEdge;
   }

   // Creates the new half-edge to add the new vertex
   GCGHALFEDGE* new_half_edge = new GCGHALFEDGE();
   if(!new_half_edge) return false;

   new_half_edge->face = ipolygon;
   new_half_edge->nextBrotherHalfEdge = new_half_edge;
   new_half_edge->prevBrotherHalfEdge = new_half_edge;
   new_half_edge->srcVertex = vertex;

   // Links the new half-edge
   if(current_half_edge) {
     // Pointer for the first half-edge of the list
     new_half_edge->nextHalfEdge = current_half_edge;
     new_half_edge->prevHalfEdge = current_half_edge->prevHalfEdge;
     current_half_edge->prevHalfEdge->nextHalfEdge = new_half_edge;
     current_half_edge->prevHalfEdge = new_half_edge;
   } else {
     // It is the first half_edge of ipolygon
     new_half_edge->nextHalfEdge = new_half_edge;
     new_half_edge->prevHalfEdge = new_half_edge;
     ipolygon->half_edge = new_half_edge;
   }

   //Pointer the first half-edge to the new half-edge in the end of the list
   if(newvertexIndex == 0) ipolygon->half_edge = new_half_edge;

   // Adds the new half-edge to the list of half-edge of the vertex
   // Check array capacity
   if(vertex->nhalf_edges + 1 > vertex->maxhalf_edges){
      // We need realloc this array
      vertex->maxhalf_edges += GCG_VERTEX_HALF_EDGE_BLOCK_SIZE;
      vertex->half_edges = (GCGHALFEDGE**) REALLOC(vertex->half_edges, sizeof(GCGHALFEDGE*) * vertex->maxhalf_edges);
      if(!vertex->half_edges) return false;
      memset(&vertex->half_edges[vertex->nhalf_edges], 0, vertex->maxhalf_edges - vertex->nhalf_edges);
   }

   // Here we're sure that it fits
   vertex->half_edges[vertex->nhalf_edges] = new_half_edge;
   vertex->nhalf_edges++;

   // Increase the number of vertices
   ipolygon->nvertices++;

   // Browse all half_edges of the polygon
   // Get first half-edge of ipolygon
   GCGHALFEDGE *aux_half_edge = ipolygon->half_edge;
   for(unsigned int i = 0; i < ipolygon->nvertices && ipolygon->nvertices > 1; i++){
      // Code to remove the half-edge from the list of half-edges brother
      aux_half_edge->prevBrotherHalfEdge->nextBrotherHalfEdge = aux_half_edge->nextBrotherHalfEdge;
      aux_half_edge->nextBrotherHalfEdge->prevBrotherHalfEdge = aux_half_edge->prevBrotherHalfEdge;

      // Insert the half-edge on the list of half-edges brother
      // Get opposite vertex
      gcgPOLYGONVERTEX *opposite_vertex = aux_half_edge->nextHalfEdge->srcVertex;
      // Get the opposite half-edge
      for(unsigned int j = 0; j < opposite_vertex->nhalf_edges && opposite_vertex->half_edges[j]->face != ipolygon; j++){
         if(opposite_vertex->half_edges[j]->nextHalfEdge->srcVertex == aux_half_edge->srcVertex){
            aux_half_edge->nextBrotherHalfEdge = opposite_vertex->half_edges[j]->nextBrotherHalfEdge;
            opposite_vertex->half_edges[j]->nextBrotherHalfEdge = aux_half_edge;
            aux_half_edge->prevBrotherHalfEdge = opposite_vertex->half_edges[j];
            aux_half_edge->nextBrotherHalfEdge->prevBrotherHalfEdge = aux_half_edge;
         }
      }
      // Get similar half-edges
      for(unsigned int j = 0; j < aux_half_edge->srcVertex->nhalf_edges && aux_half_edge->srcVertex->half_edges[j]->face != ipolygon; j++){
         if(aux_half_edge->srcVertex->half_edges[j]->nextHalfEdge->srcVertex == aux_half_edge->nextHalfEdge->srcVertex){
            aux_half_edge->nextBrotherHalfEdge = aux_half_edge->srcVertex->half_edges[j]->nextBrotherHalfEdge;
            aux_half_edge->srcVertex->half_edges[j]->nextBrotherHalfEdge = aux_half_edge;
            aux_half_edge->prevBrotherHalfEdge = aux_half_edge->srcVertex->half_edges[j];
            aux_half_edge->nextBrotherHalfEdge->prevBrotherHalfEdge = aux_half_edge;
         }
      }
      // All half-edges of the polygon
      aux_half_edge = aux_half_edge->nextHalfEdge;
   }
   return true;
}

/////////////////////////////////////////////////////////////
// Returns the gcgPOLYGONVERTEX in the position "vertexIndex"
/////////////////////////////////////////////////////////////
gcgPOLYGONVERTEX* gcgPOLYGONMESH::getPolygonVertex(gcgPOLYGON *ipolygon, unsigned int vertexIndex){
   // Check parameters
   if(!ipolygon) return FALSE;

   GCGHALFEDGE* ihalf_edge = ipolygon->half_edge;

   // Get the position of the vertex
   for(unsigned int i = 0; i < vertexIndex && i < ipolygon->nvertices; i++) ihalf_edge = ihalf_edge->nextHalfEdge;

   // Set the acess information
   last_half_edge = ihalf_edge;

   return ihalf_edge->srcVertex;
}

/////////////////////////////////////////////////////////////
// Returns the gcgPOLYGONVERTEX in the position "vertexIndex"
/////////////////////////////////////////////////////////////
gcgPOLYGONVERTEX* gcgPOLYGONMESH::getPolygonNextVertex(){
   // Check parameters
   if(!polygons || !last_half_edge) return FALSE;

   return last_half_edge->nextHalfEdge->srcVertex;
}

/////////////////////////////////////////////////////////////
// Returns the GCGPOLYGON of edge vertexID1->vertexID2 or vertexI2->vertexID1
/////////////////////////////////////////////////////////////
gcgPOLYGON* gcgPOLYGONMESH::getPolygonFromEdge(gcgPOLYGONVERTEX *vertexID1, gcgPOLYGONVERTEX *vertexID2){
   // Check parameters
   if(!vertexID1 || !vertexID2) return FALSE;

   // List of half-edges
   for(unsigned int i = 0; i < vertexID1->nhalf_edges; i++){
      // Find the correct half-edge
      GCGHALFEDGE *current_half_edge = vertexID1->half_edges[i]->nextBrotherHalfEdge;
      if(current_half_edge->nextHalfEdge->srcVertex == vertexID2){
         first_half_edge_brother = current_half_edge;
         last_half_edge_brother = current_half_edge;
         return current_half_edge->face;
      }
   }

   // List of half-edges
   for(unsigned int i = 0; i < vertexID2->nhalf_edges; i++){
      // Find the correct half-edge
      GCGHALFEDGE *current_half_edge = vertexID2->half_edges[i]->nextBrotherHalfEdge;
      if(current_half_edge->nextHalfEdge->srcVertex == vertexID1){
         first_half_edge_brother = current_half_edge;
         last_half_edge_brother = current_half_edge;
         return current_half_edge->face;
      }
   }

   return NULL;
}

/////////////////////////////////////////////////////////////
// Returns the next GCGPOLYGON of the last edge
/////////////////////////////////////////////////////////////
gcgPOLYGON* gcgPOLYGONMESH::getNextPolygonFromEdge(){
   // Check parameters
   if(!polygons || !last_half_edge_brother) return FALSE;

   last_half_edge_brother = last_half_edge_brother->nextBrotherHalfEdge;
   if(last_half_edge_brother && last_half_edge_brother != first_half_edge_brother)
   return last_half_edge_brother->face;
   else return NULL;
}

/////////////////////////////////////////////////////////////
// Remove vertex, without removing the polygons. It does not
// delete the vertex.
/////////////////////////////////////////////////////////////
bool gcgPOLYGONMESH::removeVertex(gcgPOLYGONVERTEX *vertexID){
   // Check parameters
   if(!vertexID) return false;

   unsigned int numHalfEdges = vertexID->nhalf_edges;
   // Remove all half-edges of the list
   for(unsigned int i = 0; i < numHalfEdges; i++){
      GCGHALFEDGE* current_half_edge = vertexID->half_edges[0];
      removeHalfEdge(current_half_edge);
   }

   // Remove vertex
   if(vertexID == vertices) vertices = vertexID->nextVertex;
   if(vertexID->prevVertex) vertexID->prevVertex->nextVertex = vertexID->nextVertex;
   if(vertexID->nextVertex) vertexID->nextVertex->prevVertex = vertexID->prevVertex;
   if(nvertices == 1) vertices = NULL;

   nvertices--;
   return true;
}

/////////////////////////////////////////////////////////////
// Remove half-edge, if it exists
/////////////////////////////////////////////////////////////
bool gcgPOLYGONMESH::removeHalfEdge(GCGHALFEDGE *ihalf_edge) {
   // Check parameters
   if(!ihalf_edge) return false;

   // Remove half-edge from the list of half-edges of this vertex
   // Get vertex of this half-edge
   gcgPOLYGONVERTEX* vertexID = ihalf_edge->srcVertex;
   for(unsigned int i = 0; i < vertexID->nhalf_edges; i++){
      if(vertexID->half_edges[i] == ihalf_edge){
         vertexID->nhalf_edges--;
         vertexID->half_edges[i] = vertexID->half_edges[vertexID->nhalf_edges];
         vertexID->half_edges[vertexID->nhalf_edges] = NULL;
         break;
      }
   }

   // Remove half-edge from the polygon
   if(ihalf_edge->nextHalfEdge == ihalf_edge || ihalf_edge->prevHalfEdge == ihalf_edge)
      ihalf_edge->face->half_edge = NULL; // If it's the last half-edge of this polygon
   else{
      if(ihalf_edge->face->half_edge == ihalf_edge) ihalf_edge->face->half_edge = ihalf_edge->nextHalfEdge;
      if(ihalf_edge->nextHalfEdge) ihalf_edge->nextHalfEdge->prevHalfEdge = ihalf_edge->prevHalfEdge;
      if(ihalf_edge->prevHalfEdge) ihalf_edge->prevHalfEdge->nextHalfEdge = ihalf_edge->nextHalfEdge;
   }
   // Update the number of vertices of polygon "ihalf_edge->face"
   ihalf_edge->face->nvertices--;

   // Fixed the list of half-edges brother
   // Get first half-edge of ipolygon
   GCGHALFEDGE *aux_half_edge = ihalf_edge->face->half_edge;
   for(unsigned int i = 0; i < ihalf_edge->face->nvertices; i++){
      // Code to remove the half-edge from the list of half-edges brother
      aux_half_edge->prevBrotherHalfEdge->nextBrotherHalfEdge = aux_half_edge->nextBrotherHalfEdge;
      aux_half_edge->nextBrotherHalfEdge->prevBrotherHalfEdge = aux_half_edge->prevBrotherHalfEdge;

      // Insert the half-edge on the list of half-edges brother
      // Get opposite vertex
      gcgPOLYGONVERTEX *opposite_vertex = aux_half_edge->nextHalfEdge->srcVertex;
      // Get the opposite half-edge
      for(unsigned int j = 0; j < opposite_vertex->nhalf_edges && opposite_vertex->half_edges[j]->face != ihalf_edge->face; j++){
         if(opposite_vertex->half_edges[j]->nextHalfEdge->srcVertex == aux_half_edge->srcVertex){
            aux_half_edge->nextBrotherHalfEdge = opposite_vertex->half_edges[j]->nextBrotherHalfEdge;
            opposite_vertex->half_edges[j]->nextBrotherHalfEdge = aux_half_edge;
            aux_half_edge->prevBrotherHalfEdge = opposite_vertex->half_edges[j];
            aux_half_edge->nextBrotherHalfEdge->prevBrotherHalfEdge = aux_half_edge;
         }
      }
      // Get similar half-edges
      for(unsigned int j = 0; j < aux_half_edge->srcVertex->nhalf_edges && aux_half_edge->srcVertex->half_edges[j]->face != ihalf_edge->face; j++){
         if(aux_half_edge->srcVertex->half_edges[j]->nextHalfEdge->srcVertex == aux_half_edge->nextHalfEdge->srcVertex){
            aux_half_edge->nextBrotherHalfEdge = aux_half_edge->srcVertex->half_edges[j]->nextBrotherHalfEdge;
            aux_half_edge->srcVertex->half_edges[j]->nextBrotherHalfEdge = aux_half_edge;
            aux_half_edge->prevBrotherHalfEdge = aux_half_edge->srcVertex->half_edges[j];
            aux_half_edge->nextBrotherHalfEdge->prevBrotherHalfEdge = aux_half_edge;
         }
      }
      // All half-edges of polygon
      aux_half_edge = aux_half_edge->nextHalfEdge;
   }
   // Remove half-edge
   SAFE_FREE(ihalf_edge);

   return true;
}

/////////////////////////////////////////////////////////////
// Remove edge, if it exists
// Remove all half-edges that are in the list of half-edges brothers
/////////////////////////////////////////////////////////////
bool gcgPOLYGONMESH::removeEdge(gcgPOLYGONVERTEX *vertex1, gcgPOLYGONVERTEX *vertex2){
   // Check parameters
   if(!vertex1 || !vertex2) return FALSE;

   // List of half-edges
   for(unsigned int i = 0; i < vertex1->nhalf_edges; i++) {
      // Find the correct half-edge
      if(vertex1->half_edges[i]->nextHalfEdge->srcVertex == vertex2){
         // Remove all half-edges that are in the list of half-edges brothers
         removeHalfEdge(vertex1->half_edges[i]);
         i--;
      }
   }
   // List of half-edges
   for(unsigned int i = 0; i < vertex2->nhalf_edges; i++) {
      // Find the correct half-edge
      if(vertex2->half_edges[i]->nextHalfEdge->srcVertex == vertex1){
         // Remove all half-edges that are in the list of half-edges brothers
         removeHalfEdge(vertex2->half_edges[i]);
         i--;
      }
   }
   return true;
}

/////////////////////////////////////////////////////////////
// Remove a polygon from a mesh. The polygon is not deleted.
/////////////////////////////////////////////////////////////
bool gcgPOLYGONMESH::removePolygon(gcgPOLYGON *ipolygon) {
   // Check parameters
   if(!ipolygon) return true;

   unsigned int aux_num_polygons = ipolygon->nvertices;
   // Remove each half-edge belonging to the polygon
   for(unsigned int i = 0; i < aux_num_polygons; i++)
      removeHalfEdge(ipolygon->half_edge);

   // Remove polygon
   if(polygons == ipolygon) polygons = ipolygon->nextPolygon;
   if(ipolygon->prevPolygon) ipolygon->prevPolygon->nextPolygon = ipolygon->nextPolygon;
   if(ipolygon->nextPolygon) ipolygon->nextPolygon->prevPolygon = ipolygon->prevPolygon;
   if(npolygons == 1) polygons = NULL;

   return true;
}

/////////////////////////////////////////////////////////////
// Removes all isolated points. Returns the number of removed
// vertices.
/////////////////////////////////////////////////////////////
unsigned int gcgPOLYGONMESH::removeIsolatedVertices(){
   // Check parameters
   if(nvertices == 0) return 0;

   unsigned int removedVertices = 0;
   gcgPOLYGONVERTEX *current_vertex = vertices;

   for(unsigned int i = 0; i < nvertices; i++){
      if(current_vertex->nhalf_edges == 0){
         // Remove vertex
         if(current_vertex == vertices) vertices = current_vertex->nextVertex;
         if(current_vertex->prevVertex) current_vertex->prevVertex->nextVertex = current_vertex->nextVertex;
         if(current_vertex->nextVertex) current_vertex->nextVertex->prevVertex = current_vertex->prevVertex;
         if(nvertices == removedVertices) vertices = NULL;
         SAFE_FREE(current_vertex);
         removedVertices++;
      }
      current_vertex = current_vertex->nextVertex;
   }
   nvertices -= removedVertices;
   return removedVertices;
}

/////////////////////////////////////////////////////////////
// Store in starVertices and starPolygon the simplexes that compose the star of vertexIndex
// Update numVertices and numPolygons with the number of vertices and polygons of the star
/////////////////////////////////////////////////////////////
bool gcgPOLYGONMESH::getVertexStar(gcgPOLYGONVERTEX *vertexID, gcgPOLYGONVERTEX **starVertexVertices, unsigned int* numVertices, gcgPOLYGON **starVertexPolygons, unsigned int* numPolygons){
   // Check parameters
   if(!vertexID || !starVertexVertices || numVertices == 0 || !starVertexPolygons || numPolygons == 0) return false;

   // Count number of vertices and polygons to control the size of starVertexVertices and starVertexPolygons
   // If they're not enough then return FALSE
   unsigned int aux_numVertices = 0, error = false;
   // To avoid counting an edge twice
   bool existVertex = false;

   // If size of starVertexPolygons is not enough
   if(*numPolygons < vertexID->nhalf_edges) return false;//return vertexID->nhalf_edges - *numPolygons;

   for(unsigned int i = 0; i < vertexID->nhalf_edges; i++){
      // Get polygons of the star
      starVertexPolygons[i] = vertexID->half_edges[i]->face;
      // Get vertices of the star
      GCGHALFEDGE* current_half_edge = starVertexPolygons[i]->half_edge;
      for(unsigned int j = 1; j < starVertexPolygons[i]->nvertices; j++){
         current_half_edge = current_half_edge->nextHalfEdge;
         existVertex = false;
         for(unsigned int k = 0; k < aux_numVertices; k++)
            if(starVertexVertices[k] == current_half_edge->srcVertex) existVertex = TRUE;
         if(!existVertex){
            if(aux_numVertices >= *numVertices){
               aux_numVertices++;
               error = true;
            }else{
               starVertexVertices[aux_numVertices] = current_half_edge->srcVertex;
               aux_numVertices++;
            }
         }
      }
   }
   *numPolygons = vertexID->nhalf_edges;
   *numVertices = aux_numVertices;
   if(error) return false; //return aux_numVertices;
   // Success
   return true;
}

/////////////////////////////////////////////////////////////
// Store in starVertices and starPolygon the simplexes that compose the star of vertexIndex
/////////////////////////////////////////////////////////////
unsigned int gcgPOLYGONMESH::getVerticesVertexStar(gcgPOLYGONVERTEX *vertexID, gcgPOLYGONVERTEX **starVertexVertices, unsigned int* numVertices){
   // Check parameters
   if(!vertexID || !starVertexVertices || numVertices == 0) return FALSE;

   // Count number of vertices and polygons to control the size of starVertexVertices and starVertexPolygons
   // If they're not enough then return false
   unsigned int aux_numVertices = 0;
   bool error = false;
   bool existVertex = false; // To avoid counting an edge twice

   for(unsigned int i = 0; i < vertexID->nhalf_edges; i++){
      // Get vertices of the star
      GCGHALFEDGE* current_half_edge = vertexID->half_edges[i]->face->half_edge;
      for(unsigned int j = 1; j < vertexID->half_edges[i]->face->nvertices; j++){
         current_half_edge = current_half_edge->nextHalfEdge;
         existVertex = false;
         for(unsigned int k = 0; k < aux_numVertices; k++)
            if(starVertexVertices[k] == current_half_edge->srcVertex) existVertex = TRUE;
         if(!existVertex){
            if(aux_numVertices >= *numVertices){
               aux_numVertices++;
               error = true;
            }else{
               starVertexVertices[aux_numVertices] = current_half_edge->srcVertex;
               aux_numVertices++;
            }
         }
      }
   }

   *numVertices = aux_numVertices;
   if(error) return aux_numVertices;

   // Success
   return true;
}

/////////////////////////////////////////////////////////////
// Store in starVertices and starPolygon the simplexes that
// compose the star of IEDGE
/////////////////////////////////////////////////////////////
unsigned int gcgPOLYGONMESH::getPolygonsVertexStar(gcgPOLYGONVERTEX *vertexID, gcgPOLYGON **starVertexPolygons, unsigned int* numPolygons){
   // Check parameters
   if(!vertexID || !starVertexPolygons || numPolygons == 0) return false;

   // If size of starVertexPolygons is not enough
   if(*numPolygons < vertexID->nhalf_edges) return vertexID->nhalf_edges - *numPolygons;

   for(unsigned int i = 0; i < vertexID->nhalf_edges; i++)
      starVertexPolygons[i] = vertexID->half_edges[i]->face; // Get polygons of the star

   *numPolygons = vertexID->nhalf_edges;

   // Success
   return true;
}

/////////////////////////////////////////////////////////////
// Store in starVertices and starPolygon the simplexes that compose the star of IEDGE.
/////////////////////////////////////////////////////////////
unsigned int gcgPOLYGONMESH::getEdgeStar(gcgPOLYGONVERTEX *vertexID1, gcgPOLYGONVERTEX *vertexID2, gcgPOLYGONVERTEX **starEdgeVertices, unsigned int* numVertices, gcgPOLYGON **starEdgePolygons, unsigned int* numPolygons){
   // Check parameters
   if(!vertexID1 || !vertexID2 || !starEdgeVertices || numVertices == 0 || !starEdgePolygons || numPolygons == 0) return false;

   unsigned int aux_numPolygons = 0, aux_numVertices = 0, error = false;

   // Find rigth half-edge
   GCGHALFEDGE *opposite_half_edge = NULL;
   for(unsigned int i = 0; i < vertexID1->nhalf_edges; i++)
      if(vertexID1->half_edges[i]->nextHalfEdge->srcVertex == vertexID2) opposite_half_edge = vertexID1->half_edges[i];

   // Test if the edge vertexIndex1->vertexIndex2 exits
   if(!opposite_half_edge){
      *numVertices = aux_numVertices;
      *numPolygons = aux_numPolygons;
      return true;
   }

   // Save this polygon
   if(*numPolygons < aux_numPolygons) aux_numPolygons++;
   else{
      starEdgePolygons[aux_numPolygons] = opposite_half_edge->face;
      aux_numPolygons++;
   }

   unsigned int existVertex = false;

   // Save vertices of this polygon
   // Get vertices of the star
   GCGHALFEDGE* current_half_edge = opposite_half_edge->face->half_edge;
   for(unsigned int j = 0; j < opposite_half_edge->face->nvertices; j++){
      current_half_edge = current_half_edge->nextHalfEdge;
      existVertex = FALSE;
      for(unsigned int k = 0; k < aux_numVertices; k++)
         if(starEdgeVertices[k] == current_half_edge->srcVertex) existVertex = TRUE;
      if(!existVertex){
         if(aux_numVertices >= *numVertices){
            aux_numVertices++;
            error = TRUE;
         } else{
            starEdgeVertices[aux_numVertices] = current_half_edge->srcVertex;
            aux_numVertices++;
         }
      }
   }

   // If border
   if(!opposite_half_edge->nextBrotherHalfEdge){
      *numPolygons = aux_numPolygons;
      *numVertices = aux_numVertices;
      return true;
   }

   current_half_edge = opposite_half_edge->nextBrotherHalfEdge;
   while(current_half_edge != opposite_half_edge){
      // Save this polygon
      if(*numPolygons < aux_numPolygons) aux_numPolygons++;
      else {
         starEdgePolygons[aux_numPolygons] = current_half_edge->face;
         aux_numPolygons++;
      }

      // Save vertices of this polygon
      GCGHALFEDGE* aux_current_half_edge = current_half_edge->face->half_edge;
      for(unsigned int j = 0; j < current_half_edge->face->nvertices; j++){
         aux_current_half_edge = aux_current_half_edge->nextHalfEdge;
         existVertex = FALSE;
         for(unsigned int k = 0; k < aux_numVertices; k++)
            if(starEdgeVertices[k] == aux_current_half_edge->srcVertex) existVertex = TRUE;
         if(!existVertex){
            if(aux_numVertices >= *numVertices){
               aux_numVertices++;
               error = true;
            } else{
               starEdgeVertices[aux_numVertices] = aux_current_half_edge->srcVertex;
               aux_numVertices++;
            }
         }
      }
      // Get next half-edge
      current_half_edge = current_half_edge->nextBrotherHalfEdge;
   }
   // Success
   *numPolygons = aux_numPolygons;
   *numVertices = aux_numVertices;
   if(error) return MAX(*numPolygons, *numVertices);
   return TRUE;
}

/////////////////////////////////////////////////////////////
// Return the valence of ivertex
/////////////////////////////////////////////////////////////
unsigned int gcgPOLYGONMESH::getValence(gcgPOLYGONVERTEX *vertexID){
   // Check parameters
   if(!vertexID) return FALSE;

   return vertexID->nhalf_edges;
}

/////////////////////////////////////////////////////////////
// Test link condition for stellar operations.
// Return TRUE if the operations is permitted. Return FALSE if not.
/////////////////////////////////////////////////////////////
bool gcgPOLYGONMESH::testLinkCondition(gcgPOLYGONVERTEX *vertex1, gcgPOLYGONVERTEX *vertex2){
   // Check parameters
   if(!vertex1 || !vertex2) return false;

   // Vertex Star Vectors -> VertexIndex1
   unsigned int numVertices1 = GCG_POLYGON_VERTEX_STAR_SIZE, numPolygons1 = GCG_POLYGON_STAR_SIZE;
   gcgPOLYGONVERTEX** starVertexVertices1 = (gcgPOLYGONVERTEX**) ALLOC(sizeof(gcgPOLYGONVERTEX*) * GCG_POLYGON_VERTEX_STAR_SIZE);
   if(!starVertexVertices1) return false;
   gcgPOLYGON** starVertexPolygons1 = (gcgPOLYGON**) ALLOC(sizeof(gcgPOLYGON*) * GCG_POLYGON_STAR_SIZE);
   if(!starVertexPolygons1) return false;

   // Vertex Star Vectors -> VertexIndex2
   unsigned int numVertices2 = GCG_POLYGON_VERTEX_STAR_SIZE, numPolygons2 = GCG_POLYGON_STAR_SIZE;
   gcgPOLYGONVERTEX** starVertexVertices2 = (gcgPOLYGONVERTEX**) ALLOC(sizeof(gcgPOLYGONVERTEX*) * GCG_POLYGON_VERTEX_STAR_SIZE);
   if(!starVertexVertices2) return false;
   gcgPOLYGON** starVertexPolygons2 = (gcgPOLYGON**) ALLOC(sizeof(gcgPOLYGON*) * GCG_POLYGON_STAR_SIZE);
   if(!starVertexPolygons2) return false;

   // Edge Star Vectors
   unsigned int numVerticesEdge = GCG_POLYGON_VERTEX_STAR_SIZE, numPolygonsEdge = GCG_POLYGON_STAR_SIZE;
   gcgPOLYGONVERTEX** starEdgeVertices = (gcgPOLYGONVERTEX**) ALLOC(sizeof(gcgPOLYGONVERTEX*) * GCG_POLYGON_VERTEX_STAR_SIZE);
   if(!starEdgeVertices) return false;
   gcgPOLYGON** starEdgePolygons = (gcgPOLYGON**) ALLOC(sizeof(gcgPOLYGON*) * GCG_POLYGON_STAR_SIZE);
   if(!starEdgePolygons) return false;

   // Intersection Star Vectors
   unsigned int numVerticesIntersection = GCG_POLYGON_VERTEX_STAR_SIZE, numPolygonsIntersection = GCG_POLYGON_STAR_SIZE;
   gcgPOLYGONVERTEX** starIntersectionVertices = (gcgPOLYGONVERTEX**) ALLOC(sizeof(gcgPOLYGONVERTEX*) * GCG_POLYGON_VERTEX_STAR_SIZE);
   if(!starIntersectionVertices) return false;
   gcgPOLYGON** starIntersectionPolygons = (gcgPOLYGON**) ALLOC(sizeof(gcgPOLYGON*) * GCG_POLYGON_STAR_SIZE);
   if(!starIntersectionPolygons) return false;

   // Get the star of vertexIndex1
   unsigned int result = getVertexStar(vertex1, starVertexVertices1, &numVertices1, starVertexPolygons1, &numPolygons1);
   if(result != 1){
      numVertices1 += GCG_POLYGON_VERTEX_STAR_SIZE;
      starVertexVertices1 = (gcgPOLYGONVERTEX**) REALLOC(starVertexVertices1, sizeof(gcgPOLYGONVERTEX*) * numVertices1);
      numPolygons1 += GCG_POLYGON_STAR_SIZE;
      starVertexPolygons1 = (gcgPOLYGON**) REALLOC(starVertexPolygons1, sizeof(gcgPOLYGON*) * numPolygons1);
      getVertexStar(vertex1, starVertexVertices1, &numVertices1, starVertexPolygons1, &numPolygons1);
   }

   // Get the star of vertexIndex2
   result = getVertexStar(vertex2, starVertexVertices2, &numVertices2, starVertexPolygons2, &numPolygons2);
   if(result != 1){
      numVertices2 += GCG_POLYGON_VERTEX_STAR_SIZE;
      starVertexVertices2 = (gcgPOLYGONVERTEX**) REALLOC(starVertexVertices2, sizeof(gcgPOLYGONVERTEX*) * numVertices2);
      numPolygons2 += GCG_POLYGON_STAR_SIZE;
      starVertexPolygons2 = (gcgPOLYGON**) REALLOC(starVertexPolygons2, sizeof(gcgPOLYGON*) * numPolygons2);
      getVertexStar(vertex2, starVertexVertices2, &numVertices2, starVertexPolygons2, &numPolygons2);
   }

   // Get the star of edge: vertexIndex1 -> vertexIndex2
   result = getEdgeStar(vertex1, vertex2, starEdgeVertices, &numVerticesEdge, starEdgePolygons, &numPolygonsEdge);
   if(result != 1){
      numVerticesEdge += GCG_POLYGON_VERTEX_STAR_SIZE;
      starEdgeVertices = (gcgPOLYGONVERTEX**) REALLOC(starEdgeVertices, sizeof(gcgPOLYGONVERTEX*) * numVerticesEdge);
      numPolygonsEdge += GCG_POLYGON_STAR_SIZE;
      starEdgePolygons = (gcgPOLYGON**) REALLOC(starEdgePolygons, sizeof(gcgPOLYGON*) * numPolygonsEdge);
      getEdgeStar(vertex1, vertex2, starEdgeVertices, &numVerticesEdge, starEdgePolygons, &numPolygonsEdge);
   }

   // Get the star intersection
   getStarIntersection(starVertexVertices1, starVertexPolygons1, starVertexVertices2, starVertexPolygons2, starIntersectionVertices, starIntersectionPolygons, numVertices1, numPolygons1, numVertices2, numPolygons2, &numVerticesIntersection, &numPolygonsIntersection);


   SAFE_FREE(starVertexVertices1);
   SAFE_FREE(starVertexVertices2);
   SAFE_FREE(starVertexPolygons1);
   SAFE_FREE(starVertexPolygons2);

   bool exits = starComparison(starEdgeVertices, starEdgePolygons, starIntersectionVertices, starIntersectionPolygons, numVerticesEdge, numPolygonsEdge, numVerticesIntersection, numPolygonsIntersection);

   SAFE_FREE(starEdgeVertices);
   SAFE_FREE(starEdgePolygons);
   SAFE_FREE(starIntersectionVertices);
   SAFE_FREE(starIntersectionPolygons);

   return exits;
}

/////////////////////////////////////////////////////////////
// Return the intersection between star1 and star2
/////////////////////////////////////////////////////////////
bool gcgPOLYGONMESH::getStarIntersection(gcgPOLYGONVERTEX** starVertexVertices1, gcgPOLYGON** starVertexPolygons1, gcgPOLYGONVERTEX** starVertexVertices2, gcgPOLYGON** starVertexPolygons2, gcgPOLYGONVERTEX** starIntersectionVertices, gcgPOLYGON** starIntersectionPolygons, unsigned int numVertices1, unsigned int numPolygons1, unsigned int numVertices2, unsigned int numPolygons2, unsigned int* numVerticesIntersection, unsigned int* numPolygonsIntersection){
   // Check parameters
   if(!starVertexVertices1 || !starVertexPolygons1 || !starVertexVertices2 || !starVertexPolygons2 ||
      !starIntersectionVertices || !starIntersectionPolygons || numVertices1 == 0 || numPolygons1 == 0 ||
      numVertices2 == 0 || numPolygons2 == 0 || numVerticesIntersection == 0 || numPolygonsIntersection == 0) return FALSE;

   unsigned int aux_numVertices = 0, aux_numPolygons = 0;

   // Get vertices
   for(unsigned int i = 0; i < numVertices1; i++)
      for(unsigned int j = 0; j < numVertices2; j++)
         if(starVertexVertices1[i] == starVertexVertices2[j]){
            starIntersectionVertices[aux_numVertices] = starVertexVertices1[i];
            aux_numVertices++;
         }
   for(unsigned int i = 0; i < numPolygons1; i++)
      for(unsigned int j = 0; j < numPolygons2; j++)
         if(starVertexPolygons1[i] == starVertexPolygons2[j]){
            starIntersectionPolygons[aux_numPolygons] = starVertexPolygons1[i];
            aux_numPolygons++;
         }

   *numVerticesIntersection = aux_numVertices;
   *numPolygonsIntersection = aux_numPolygons;

   return true;
}

/////////////////////////////////////////////////////////////
// Compare two stars: return TRUE if they're equal, FALSE if not
/////////////////////////////////////////////////////////////
bool gcgPOLYGONMESH::starComparison(gcgPOLYGONVERTEX** starVertices1, gcgPOLYGON** starPolygons1, gcgPOLYGONVERTEX** starVertices2, gcgPOLYGON** starPolygons2, unsigned int numVertices1, unsigned int numPolygons1, unsigned int numVertices2, unsigned int numPolygons2){
   // Check parameters
   if(!starVertices1 || !starPolygons1 || !starVertices2 || !starPolygons2 ||
      numVertices1 == 0 || numPolygons1 == 0 || numVertices2 == 0 || numPolygons2 == 0 ||
      numVertices1 != numVertices2 || numPolygons1 != numPolygons2) return false;

   // Returned false if the number of elements is different
   unsigned int equal = false;
   // Compare Vertices
   for(unsigned int i = 0; i < numVertices1; i++){
      for(unsigned int j = 0; j < numVertices2; j++)
         if(starVertices1[i] == starVertices2[j]) equal = true;
      if(!equal) return false;
   }

   // Compare Polygons
   for(unsigned int i = 0; i < numPolygons1; i++){
      for(unsigned int j = 0; j < numPolygons2; j++)
         if(starPolygons1[i] == starPolygons2[j]) equal = true;
      if(!equal) return false;
   }
   return true;
}

/////////////////////////////////////////////////////////////
// Edge collapse: the FIRST vertex of parameters is removed and keep the last one
/////////////////////////////////////////////////////////////
bool gcgPOLYGONMESH::edgeCollapse(gcgPOLYGONVERTEX *vertex1, gcgPOLYGONVERTEX *vertex2){
   // Check parameters
   if(!vertex1 || !vertex2) return false;
   // Test the linkCondition
   if(!testLinkCondition(vertex1, vertex2)) return false;

   // Get star of edge
   unsigned int numVerticesEdge = GCG_POLYGON_VERTEX_STAR_SIZE, numPolygonsEdge = GCG_POLYGON_STAR_SIZE;
   gcgPOLYGONVERTEX** starEdgeVertices = (gcgPOLYGONVERTEX**) ALLOC(sizeof(gcgPOLYGONVERTEX*) * GCG_POLYGON_VERTEX_STAR_SIZE);
   if(!starEdgeVertices) return FALSE;
   gcgPOLYGON** starEdgePolygons = (gcgPOLYGON**) ALLOC(sizeof(gcgPOLYGON*) * GCG_POLYGON_STAR_SIZE);
   if(!starEdgePolygons) return FALSE;

   unsigned int result = getEdgeStar(vertex1, vertex2, starEdgeVertices, &numVerticesEdge, starEdgePolygons, &numPolygonsEdge);
   if(result != 1) {
      numVerticesEdge += GCG_POLYGON_VERTEX_STAR_SIZE;
      starEdgeVertices = (gcgPOLYGONVERTEX**) REALLOC(starEdgeVertices, sizeof(gcgPOLYGONVERTEX*) * numVerticesEdge);
      numPolygonsEdge += GCG_POLYGON_STAR_SIZE;
      starEdgePolygons = (gcgPOLYGON**) REALLOC(starEdgePolygons, sizeof(gcgPOLYGON*) * numPolygonsEdge);
      getEdgeStar(vertex1, vertex2, starEdgeVertices, &numVerticesEdge, starEdgePolygons, &numPolygonsEdge);
   }

   // Delete vertex1 from edgeStar
   for(unsigned int i = 0; i < numVerticesEdge; i++)
      if(vertex1 == starEdgeVertices[i])
         starEdgeVertices[i] = NULL;

   // Remove the polygons
   for(unsigned int i = 0; i < numPolygonsEdge; i++)
      removePolygon(starEdgePolygons[i]);

   // Update all half-edge of vertexID1, now their origin will be vertexID2
   for(unsigned int i = 0; i < vertex1->nhalf_edges; i++)
      vertex1->half_edges[i]->srcVertex = vertex2;

   // Add all half-edges of vertexID1 on the list of vertexID2's half-edges
   while(vertex2->maxhalf_edges < vertex2->nhalf_edges + vertex1->nhalf_edges){
      // We need realloc this array
      vertex2->maxhalf_edges += GCG_VERTEX_HALF_EDGE_BLOCK_SIZE;
      vertex2->half_edges = (GCGHALFEDGE**) REALLOC(vertex2->half_edges, sizeof(GCGHALFEDGE*) * vertex2->maxhalf_edges);
      for(unsigned int k = vertex2->maxhalf_edges - GCG_VERTEX_HALF_EDGE_BLOCK_SIZE; k < GCG_VERTEX_HALF_EDGE_BLOCK_SIZE; k++)
         vertex2->half_edges[k] = NULL;
      if(!vertex2->half_edges) return false;
   }
   memcpy(&vertex2->half_edges[vertex2->nhalf_edges], vertex1->half_edges, sizeof(GCGHALFEDGE*) * vertex1->nhalf_edges);
   vertex2->nhalf_edges = vertex2->nhalf_edges + vertex1->nhalf_edges;

   // Remove vertex1
   if(vertex1 == vertices){
      vertices = vertex1->nextVertex;
      vertices->prevVertex = NULL;
   }
   if(vertex1->prevVertex) vertex1->prevVertex->nextVertex = vertex1->nextVertex;
   if(vertex1->nextVertex) vertex1->nextVertex->prevVertex = vertex1->prevVertex;
   if(nvertices == 1) vertices = NULL;
   SAFE_DELETE(vertex1);

   nvertices--;

   // Update the half-edges brothers
   // Look for the half-edge that has the origin on vertexIndex2 and finish in a vertex of edge star
   for(unsigned int i = 0; i < vertex2->nhalf_edges; i++)
      for(unsigned int j = 0; j < numVerticesEdge; j++)
         if(starEdgeVertices && starEdgeVertices[j] == vertex2->half_edges[i]->nextHalfEdge->srcVertex)
            // Found Half-Edge: vertex2->half_edges[i]
            // Found Vertex: starEdgeVertices[j]
            for(unsigned int k = 0; k < starEdgeVertices[j]->nhalf_edges; k++)
               if(starEdgeVertices[j]->half_edges[k]->nextHalfEdge->srcVertex == vertex2){
                  // Update half-edges brother
                  GCGHALFEDGE* aux_half_edge_brother = vertex2->half_edges[i]->nextBrotherHalfEdge;
                  vertex2->half_edges[i]->nextBrotherHalfEdge = starEdgeVertices[j]->half_edges[k];
                  starEdgeVertices[j]->half_edges[k]->nextBrotherHalfEdge = aux_half_edge_brother;
               }

   return true;
}

/////////////////////////////////////////////////////////////
// Half-Edge collapse. New vertex is copied and is not
// deleted.
/////////////////////////////////////////////////////////////
bool gcgPOLYGONMESH::halfEdgeCollapse(gcgPOLYGONVERTEX *vertex1, gcgPOLYGONVERTEX *vertex2, gcgPOLYGONVERTEX *newvertex){
   // Check parameters
   if(!vertex1 || !vertex2) return false;

   // Test the link condition
   if(!testLinkCondition(vertex1, vertex2)) return false;
   vertex1->copy(newvertex);
   return true;
}

/////////////////////////////////////////////////////////////
// Edge Flip
/////////////////////////////////////////////////////////////
bool gcgPOLYGONMESH::edgeFlip(gcgPOLYGONVERTEX *vertex1, gcgPOLYGONVERTEX *vertex2){
   // Check parameters
   if(!vertex1 || !vertex2) return false;

   // Test the linkCondition
   if(!testLinkCondition(vertex1, vertex2)) return false;

   // Get star of edge
   unsigned int numVerticesEdge = GCG_POLYGON_VERTEX_STAR_SIZE, numPolygonsEdge = GCG_POLYGON_STAR_SIZE;
   gcgPOLYGONVERTEX** starEdgeVertices = (gcgPOLYGONVERTEX**) ALLOC(sizeof(gcgPOLYGONVERTEX*) * GCG_POLYGON_VERTEX_STAR_SIZE);
   if(!starEdgeVertices) return FALSE;
   gcgPOLYGON** starEdgePolygons = (gcgPOLYGON**) ALLOC(sizeof(gcgPOLYGON*) * GCG_POLYGON_STAR_SIZE);
   if(!starEdgePolygons) return FALSE;

   unsigned int result = getEdgeStar(vertex1, vertex2, starEdgeVertices, &numVerticesEdge, starEdgePolygons, &numPolygonsEdge);
   if(result != 1) {
      numVerticesEdge += GCG_POLYGON_VERTEX_STAR_SIZE;
      starEdgeVertices = (gcgPOLYGONVERTEX**) REALLOC(starEdgeVertices, sizeof(gcgPOLYGONVERTEX*) * numVerticesEdge);
      numPolygonsEdge += GCG_POLYGON_STAR_SIZE;
      starEdgePolygons = (gcgPOLYGON**) REALLOC(starEdgePolygons, sizeof(gcgPOLYGON*) * numPolygonsEdge);
      getEdgeStar(vertex1, vertex2, starEdgeVertices, &numVerticesEdge, starEdgePolygons, &numPolygonsEdge);
   }

   for(unsigned int i = 0; i < vertex1->nhalf_edges; i++) {
      for(unsigned int j = 0; j < vertex2->nhalf_edges; j++) {
         // Half-edge vertexIndex1 -> vertexIndex2
         // Half-edge vertexIndex2 -> vertexIndex1
         if(vertex1->half_edges[i]->nextHalfEdge->srcVertex == vertex2 &&
            vertex2->half_edges[j]->nextHalfEdge->srcVertex == vertex1) {

            gcgPOLYGONVERTEX* new_vertex1 = vertex1->half_edges[i]->prevHalfEdge->srcVertex;
            gcgPOLYGON* new_polygon1 = vertex1->half_edges[i]->face;
            gcgPOLYGONVERTEX* new_vertex2 = vertex2->half_edges[j]->prevHalfEdge->srcVertex;
            gcgPOLYGON* new_polygon2 = vertex2->half_edges[j]->face;

            // Get the position where the new half-edge will be add
            unsigned int edgeIndex1 = 0, edgeIndex2 = 0;
            GCGHALFEDGE* current_half_edge = new_polygon1->half_edge;
            for(unsigned int k = 0; k < new_polygon1-> nvertices; k++){
               if(current_half_edge == vertex1->half_edges[i]) edgeIndex1 = k;
               current_half_edge = current_half_edge->nextHalfEdge;
            }
            // Get the position where the new half-edge will be add
            current_half_edge = new_polygon2->half_edge;
            for(unsigned int k = 0; k < new_polygon2-> nvertices; k++){
               if(current_half_edge == vertex2->half_edges[j]) edgeIndex2 = k;
               current_half_edge = current_half_edge->nextHalfEdge;
            }
            removeHalfEdge(vertex1->half_edges[i]);
            removeHalfEdge(vertex2->half_edges[j]);
            removeEdge(vertex1, vertex2);
            addPolygonVertex(new_polygon1, new_vertex2->half_edges[0]->srcVertex, edgeIndex1);
            addPolygonVertex(new_polygon2, new_vertex1->half_edges[0]->srcVertex, edgeIndex2);
            break;
         }
      }
   }

   return true;
}

/////////////////////////////////////////////////////////////
// Edge Weld: choose the vertex that creates minor error
/////////////////////////////////////////////////////////////
bool gcgPOLYGONMESH::edgeWeld(gcgPOLYGONVERTEX *vertexID){
   // Check parameters
   if(!vertexID) return false;
   return edgeCollapse(vertexID, vertexID->half_edges[0]->nextHalfEdge->srcVertex);
}

/////////////////////////////////////////////////////////////
// Edge Split: divide all polygons that use this edge
/////////////////////////////////////////////////////////////
bool gcgPOLYGONMESH::edgeSplit(gcgPOLYGONVERTEX *vertex1, gcgPOLYGONVERTEX *vertex2, gcgPOLYGONVERTEX *newvertex){
   // Check parameters
   if(!vertex1 || !vertex2) return false;

   for(unsigned int i = 0; i < vertex1->nhalf_edges; i++){
      for(unsigned int j = 0; j < vertex2->nhalf_edges; j++){
         // Half-edge vertexIndex1 -> vertexIndex2
         // Half-edge vertexIndex2 -> vertexIndex1
         if(vertex1->half_edges[i]->nextHalfEdge->srcVertex == vertex2 &&
            vertex2->half_edges[j]->nextHalfEdge->srcVertex == vertex1 &&
            vertex1->half_edges[i]->face->nvertices > 2 &&
            vertex2->half_edges[j]->face->nvertices > 2){

            gcgPOLYGON *old_polygon1 = vertex1->half_edges[i]->face;
            gcgPOLYGON *old_polygon2 = vertex2->half_edges[j]->face;

            polygonSplit(old_polygon1, 1, newvertex);
            polygonSplit(old_polygon2, 1, newvertex);

            return true;
         }
      }
   }

   return false;
}

/////////////////////////////////////////////////////////////
// Edge Split: divide all polygons that use this edge
/////////////////////////////////////////////////////////////
bool gcgPOLYGONMESH::polygonSplit(gcgPOLYGON *ipolygon, unsigned int vertexIndex, gcgPOLYGONVERTEX *newvertex){
  // Check parameters
  if(!ipolygon) return false;
  if(ipolygon->nvertices < 3) return false;

  // Get vertexIndex
  GCGHALFEDGE *half_edgeID = ipolygon->half_edge;
  for(unsigned int i = 1; i < vertexIndex; i++)
    half_edgeID = half_edgeID->nextHalfEdge;

  gcgPOLYGONVERTEX *vertexID = half_edgeID->srcVertex;
  gcgPOLYGONVERTEX *vertex1  = half_edgeID->nextHalfEdge->srcVertex;

  // Add new polygon 1
  gcgPOLYGON *new_polygon1 = new gcgPOLYGON();
  if(!addPolygon(new_polygon1)) return false;
  addPolygonVertex(new_polygon1, vertexID,  0);
  addPolygonVertex(new_polygon1, vertex1,   1);
  addPolygonVertex(new_polygon1, newvertex, 2);

  // Add new polygon 3
  gcgPOLYGON *new_polygon2 = new gcgPOLYGON();
  if(!addPolygon(new_polygon2)) return false;
  addPolygonVertex(new_polygon2, newvertex, new_polygon2->nvertices);
  GCGHALFEDGE *current_half_edge = half_edgeID->nextHalfEdge->nextHalfEdge;
  for(unsigned int k = 0; k < ipolygon->nvertices - 1; k++){
     addPolygonVertex(new_polygon2, current_half_edge->srcVertex, new_polygon2->nvertices);
     current_half_edge = current_half_edge->nextHalfEdge;
  }

  // Remove the polygon
  removePolygon(ipolygon);

  return true;
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////// End of gcgPOLYGONMESH class implementation //////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
