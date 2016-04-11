/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS, IMAGE AND VISION
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

   SEMIREGULAR48.CPP: restricted unbalanced binary tree

   Marcelo Bernardes Vieira
   Daniel Madeira
**************************************************************************************/

#include "system.h"

#define BLOCK_TRIANGLES       1024
#define BLOCK_NODES           2048
#define DEFAULT_ARRAY_BLOCK   1024

gcgSEMIREGULAR48::gcgSEMIREGULAR48() {
		// p3-----p2
		//  |   /	|
		//  |  / 	|
		//  | /   |
		// p0-----p1

    // Inicializa os blocos de memoria para controle
    maxnodos    = BLOCK_NODES;
    maxtriang   = BLOCK_TRIANGLES;
    nodos       = (NODO*) ALLOC(sizeof(NODO) * maxnodos);
    front       = (FRONTINFO*) ALLOC(sizeof(FRONTINFO) * maxtriang);
    vertices    = (int*)  ALLOC(3 * sizeof(int) * maxtriang);
    nnodos      = 0;
    ntriangulos = 0;

    // Stack initialization
    stack = NULL;
    maxstack = 0;
    top = -1;     // Not in depth

		// Inicializa as topology
		topology   = GCG_TOPOLOGY_PLANE;

    // Forma a malha básica
    initTesselation(NULL);
}

// Destrutor da árvore
gcgSEMIREGULAR48::~gcgSEMIREGULAR48(){
		SAFE_FREE(nodos);
		SAFE_FREE(front);
		SAFE_FREE(vertices);
		SAFE_FREE(stack);
}


void gcgSEMIREGULAR48::setTopology(int topologycode) {
  topology = (unsigned char) topologycode;
}

void gcgSEMIREGULAR48::divide_node(int inodo) {
    // Check the stack capacity
    CHECK_DYNAMIC_ARRAY_CAPACITY_VOID(nodos, maxnodos, nnodos + 38, NODO); // We´ll need at maximum new 38 nodes (d)

    if(ntriangulos >= maxtriang - 19) {
      maxtriang   += BLOCK_TRIANGLES;
      vertices = (int*) REALLOC(vertices, 3 * sizeof(int) * maxtriang);
      front = (FRONTINFO*) REALLOC(front, sizeof(FRONTINFO) * maxtriang);
    }

    register NODO *nodo      = &nodos[inodo];
    register int  *triang    = &vertices[3 * nodo->iInfo];
    unsigned char nivelatual = nodo->nivel;

    // Se o nodo pertence ao bordo, entao o divide
    if(nodo->vizbase < 0) {
      register int ifilhoesq = nnodos++, ifilhodir = nnodos++;
      // Calcula o novo vértice usando os três do triângulo
      triangle_bissection(inodo, geometria->sample(&vertices[nodo->iInfo * 3], -1), ifilhoesq);

		  // Atualiza ponteiros
		  nodos[ifilhodir].vizesq = (nodos[ifilhoesq].vizdir = -1);
		  return;
    }

    if(nodos[nodo->vizbase].vizbase != inodo) divide_node(nodo->vizbase);
    // NESSE MOMENTO TEMOS UM LOSANGO E PODEMOS DIVIDI-LO
    // Primeiro amostramos os pontos necessarios.

    register int ivizbase = nodo->vizbase;
    register int ivert, ibase;

    // Verifica se o losango forma uma descontinuidade.
    register int *tribase = &vertices[nodos[ivizbase].iInfo * 3];
    if(tribase[GCG_VESQ] == triang[GCG_VDIR]) {
      // Aqui o mesmo vertice pode ser usado para dividir o triangulo base
      // Calcula o novo vértice usando os quatro do losango
      ivert = ibase = geometria->sample(triang, tribase[GCG_VAPEX]);
    } else {
      // Aqui cada um deve ter o seu calculado em separado
      ivert = geometria->sample(triang,  -1);
      ibase = geometria->sample(tribase, -1);
    }

    // Agora podemos dividir os dois triangulos
    register int ifilhoesq  = nnodos++, ifilhodir  = nnodos++;
    register int bifilhoesq = nnodos++, bifilhodir = nnodos++;

    triangle_bissection(inodo,    ivert, ifilhoesq);
    triangle_bissection(ivizbase, ibase, bifilhoesq);

    // Conecta os ponteiros internos
    if(nivelatual & 0x1) {
      // Nivel impar
		  nodos[ifilhoesq].vizesq		= bifilhodir;
		  nodos[bifilhodir].vizdir	= ifilhoesq;
  		nodos[ifilhodir].vizdir	  = bifilhoesq;
      nodos[bifilhoesq].vizesq	= ifilhodir;
    } else {
      // Nível par
      nodos[ifilhoesq].vizdir		= bifilhodir;
      nodos[bifilhodir].vizesq	= ifilhoesq;
		  nodos[ifilhodir].vizesq		= bifilhoesq;
		  nodos[bifilhoesq].vizdir	= ifilhodir;
	 }
}


void gcgSEMIREGULAR48::triangle_bissection(int inodo, int bis_point, int ifilhoesq){
    /*        vapex
    //         /|\
    //       /  |  \
    //     /    |    \
    //   /	    |      \
    // vesq-----*-----vdir
    //      novo_apex
    //
    // O novo apex (bis_point) é um ponto interno do triângulo.
    */
    register NODO  *nodo      = &nodos[inodo];
    register NODO  *filhoesq  = &nodos[ifilhoesq];
    register int   ifilhodir  = ifilhoesq + 1;
    register NODO  *filhodir  = &nodos[ifilhodir];

    nodo->filhoesq = ifilhoesq;

    // The search is in depth?
    if(top >= 0) {
      // Check the stack capacity
      CHECK_DYNAMIC_ARRAY_CAPACITY_VOID(stack, maxstack, top + 2, int);

      stack[++top] = ifilhoesq + 1;
      stack[++top] = ifilhoesq;
    }

    // Conecta os ponteiros dos VIZINHOS
    if(nodo->nivel & 0x01) { // Is odd?
      // O nível é IMPAR

      // Testa o triangulo da esquerda, no nível do PAI.
      if(nodo->vizesq >= 0) {
        register NODO *vizesq   = &nodos[nodo->vizesq];

        if(vizesq->vizdir == inodo) vizesq->vizdir = ifilhodir;
        else vizesq->vizbase = ifilhodir;
      }

      // Testa o triangulo da direira, no nível do PAI.
      if(nodo->vizdir >= 0) {
        register NODO *vizdir   = &nodos[nodo->vizdir];
        if(vizdir->vizesq == inodo) vizdir->vizesq = ifilhoesq;
        else vizdir->vizbase = ifilhoesq;
      }

      // Vizinhos da esquerda e da direita do PAI
      filhoesq->vizdir  = ifilhodir;
      filhoesq->vizbase = nodo->vizdir;

      filhodir->vizbase = nodo->vizesq;
      filhodir->vizesq  = ifilhoesq;

      // O TESTE DA BASE FICA PARA DEPOIS
    } else {
      // O nível é PAR

      // Testa o triangulo da esquerda, no nível do PAI.
      if(nodo->vizesq >= 0) {
        register NODO *vizesq   = &nodos[nodo->vizesq];
        if(vizesq->vizdir == inodo) vizesq->vizdir = ifilhoesq;
        else vizesq->vizbase = ifilhoesq;
      }

      // Testa o triangulo da direira, no nível do PAI.
      if(nodo->vizdir >= 0) {
        register NODO *vizdir   = &nodos[nodo->vizdir];
        if(vizdir->vizesq == inodo) vizdir->vizesq = ifilhodir;
        else vizdir->vizbase = ifilhodir;
      }

      // Vizinhos da esquerda e da direita do PAI
      filhoesq->vizesq  = ifilhodir;
      filhoesq->vizbase = nodo->vizesq;

      filhodir->vizbase = nodo->vizdir;
      filhodir->vizdir  = ifilhoesq;

      // O TESTE DA BASE FICA PARA DEPOIS
    }

    // Inicia os ponteiros dos FILHOS
    filhoesq->filhoesq = filhodir->filhoesq = -1;

    filhoesq->nivel  = filhodir->nivel = nodo->nivel + 1;
    //filhoesq->indice = 2 * triang->indice + 1;
    //filhodir->indice = filhoesq->indice + 1;//2 * triang->indice + 2;

    // Começa a inicializacao dos triangulos
    int itriesq = ntriangulos++;    // ALOCA O TRIANGULO DO FILHO ESQUERDO
    register int *triesq = &vertices[3 * (filhoesq->iInfo = itriesq)];
    register int *triang = &vertices[3 * nodo->iInfo];

    //===> reutiliza o triangulo do pai <===
    front[itriesq].lastmask = front[filhodir->iInfo = nodo->iInfo].lastmask;

    if(nodo->nivel & 0x01) { // is odd?
      // O nível é IMPAR

      // Filho esquerdo
      triesq[GCG_VAPEX] = bis_point;
      triesq[GCG_VESQ]  = triang[GCG_VAPEX];
      triesq[GCG_VDIR]  = triang[GCG_VESQ];

      // Filho direito
      triang[GCG_VESQ]  = triang[GCG_VDIR];
      triang[GCG_VDIR]  = triang[GCG_VAPEX];
      triang[GCG_VAPEX] = bis_point;
    } else {
      // O nível é PAR

      // Filho esquerdo
      triesq[GCG_VAPEX] = bis_point;
      triesq[GCG_VESQ]  = triang[GCG_VDIR];
      triesq[GCG_VDIR]  = triang[GCG_VAPEX];

			// Filho direito
			triang[GCG_VDIR]  = triang[GCG_VESQ];
			triang[GCG_VESQ]  = triang[GCG_VAPEX];
      triang[GCG_VAPEX] = bis_point;
    }

    // ====> Reutiliza o triangulo do pai <====
    nodo->iInfo = -1;       // Disconnects the father, info available only for
                            //  front nodes.

    // Perdemos o pai, ganhamos dois filhos
}



///////////////////////////////////////////////
// Inicializa a árvore formando a malha basica
///////////////////////////////////////////////
int gcgSEMIREGULAR48::initTesselation(gcgADAPTIVE *geo) {
    // Libera arvore e triangulos
    nnodos = 0;
    ntriangulos = 0;

    geometria = geo;
    if(geo) geo->beginTesselation();

    ///////////////////////////
		// FORMA A MALHA BÁSICA
		///////////////////////////
		int       itriesq = ntriangulos++;
		register int *triesq = &vertices[3 * itriesq];
		int       itridir = ntriangulos++;
		register int *tridir = &vertices[3 * itridir];

    triesq[GCG_VAPEX]     = 3;    // APEX Triangulo esquerdo
		triesq[GCG_VESQ]      = 0;    // VESQ Triangulo esquerdo
		triesq[GCG_VDIR]      = 2;    // VDIR Triangulo esquerdo

		tridir[GCG_VAPEX]     = 1;    // APEX Triangulo direito
		tridir[GCG_VESQ]      = 2;    // VESQ Triangulo direito
		tridir[GCG_VDIR]      = 0;    // VDIR Triangulo direito

		// Info for initial front
		front[itriesq].lastmask = front[itridir].lastmask = 0;

		// Ponteiros dos filhos da raiz
		int   ifilhoesq = nnodos++;
		NODO  *filhoesq = &nodos[ifilhoesq];
		int   ifilhodir = nnodos++;
		NODO  *filhodir = &nodos[ifilhodir];

		// Ponteiros do filho (triangulo) esquerdo
		filhoesq->nivel     = 1;
		//triesq->indice    = 1;
		filhoesq->iInfo     = itriesq;
		filhoesq->filhoesq  = -1;
		filhoesq->vizesq    = -1;
		filhoesq->vizdir    = -1;
		filhoesq->vizbase   = ifilhodir;

		// Ponteiros do filho (triangulo) direito
		filhodir->nivel     = 1;
		//tridir->indice    = 2;
		filhodir->iInfo     = itridir;
		filhodir->filhoesq  = -1;
		filhodir->vizesq    = -1;
		filhodir->vizdir    = -1;
		filhodir->vizbase   = ifilhoesq;

 		// Testa se há necessidade de conectar bordas.
    if(topology) {
      // Forma as quatro bases dividindo os dois filhos da raiz
      divide_node(ifilhoesq);

      switch(topology) {
        case GCG_TOPOLOGY_TORUS:  // WEST to EAST and NORTH to SOUTH
                                  nodos[filhoesq->filhoesq + 1].vizbase = filhodir->filhoesq + 1;
                                  nodos[filhodir->filhoesq + 1].vizbase = filhoesq->filhoesq + 1;

        case GCG_TOPOLOGY_CYLINDER:// WEST to EAST
                                  nodos[filhoesq->filhoesq].vizbase = filhodir->filhoesq;
                                  nodos[filhodir->filhoesq].vizbase = filhoesq->filhoesq;
                                  break;
      }
      return 2; // First node to be processed
    }

    return 0; // First node to be processed
}


///////////////////////////////////////////////////////////////////////////////
// Tessela a malha de forma top-down e em largura. Para cada nó-triângulo,
// chama a função refina para testar se ele deve ser refinado. Se sim. chama a
// funcao amostra_param para determinar o novo ponto.
///////////////////////////////////////////////////////////////////////////////
int gcgSEMIREGULAR48::tesselate(int maxtri, gcgADAPTIVE *geo){
    // Forma a malha básica
    int inodo = initTesselation(geo);

    top = -1; // Not in depth

    // Gera a árvore em largura: FIFO (first-in first-out)
    for(; inodo < nnodos && maxtri > ntriangulos; inodo++) {
      register NODO *nodo = &nodos[inodo];
      register int  index = nodo->iInfo;
      register unsigned char *mask = &front[index].lastmask;

      if(index > -1) { // Is a front node?
        if((*mask & GCG_MAXIMUMLOD) == 0) {
          *mask = geometria->action(nodo->nivel, &vertices[3 * index], *mask);
          if((*mask & (GCG_REFINE | GCG_SIMPLIFY)) == GCG_REFINE) divide_node(inodo);
        }
      } else {} // Não dá para prever se esse nodo será dividido forçosamente
              //  mais tarde. Nao pode contar aqui.
    }

    // No triangles removed from front
    nhidden  = 0;
    hidemask = 0x00;  // All triangles are kept
    showmask = 0x00;  // Not important since hidemask = 0x00

    if(geo) geo->endTesselation();

    // Retorna o numero de triangulos obtidos
    return ntriangulos;
}


///////////////////////////////////////////////////////////////////////////////
// Tessela a malha de forma top-down e em largura. Para cada nó-triângulo,
// chama a função refina para testar se ele deve ser refinado. Se sim. chama a
// funcao amostra_param para determinar o novo ponto.
///////////////////////////////////////////////////////////////////////////////
int gcgSEMIREGULAR48::tesselate(gcgADAPTIVE *geo){
    // Forma a malha básica
    int inodo = initTesselation(geo);

    // Check the stack
    if(maxstack == 0) {
      maxstack = DEFAULT_ARRAY_BLOCK;
      stack = (int*) ALLOC(maxstack * sizeof(int));
    }

    // Stack initialization
    stack[0] = 1;
    stack[1] = 0;
    if(topology) {
      stack[2] = 2;
      stack[3] = 3;
      top = 3;
    } else top = 1;

    while(top >= 0) {
      inodo = stack[top--];
      register NODO *nodo = &nodos[inodo];
      register int index = nodo->iInfo;
      register unsigned char *mask = &front[index].lastmask;

      if(index > -1) { // Is a front node?
        if((*mask & GCG_MAXIMUMLOD) == 0) {
          *mask = geometria->action(nodo->nivel, &vertices[3 * index], *mask);
          if((*mask & (GCG_REFINE | GCG_SIMPLIFY)) == GCG_REFINE) divide_node(inodo);
        }
      }; // Não dá para prever se esse nodo será dividido forçosamente
         //  mais tarde. Nao pode contar aqui.
    }

    // No triangles removed from front
    nhidden  = 0;
    hidemask = 0x00;  // All triangles are kept
    showmask = 0x00;  // Not important since hidemask = 0x00

    if(geo) geo->endTesselation();

    // Retorna o numero de triangulos obtidos
    return ntriangulos;
}

///////////////////////////////////////////////////////////////////////////////
// Returns the last tesselation vertex index array. If 'ntriang' is non-null
// stores the number of triangles (vertex triplets) in it.
///////////////////////////////////////////////////////////////////////////////
int* gcgSEMIREGULAR48::getIndexArray(int *ntriang) {
  if(ntriang) *ntriang = ntriangulos - nhidden;
  return vertices;
}

///////////////////////////////////////////////////////////////////////////////
// Selects the triangles using a flag. If the logical AND of the flag and
// the triangle flag is non-null, this triangle is moved to the end of array.
// If the flag is 0x00, then all triangles of the tesselation are delivered
// in the next call of getIndexArray. Returns the number of selected triangles.
///////////////////////////////////////////////////////////////////////////////

int gcgSEMIREGULAR48::hideTriangles(unsigned char hmask, unsigned char smask) {
  if(!hmask) {
    this->nhidden  = 0;
    this->hidemask = 0x00;
    this->showmask = 0x00;
  } else
    if(hmask != this->hidemask || smask != this->showmask) {
      // Simply reorganize the front triangles
      register int ini = 0, end = ntriangulos - 1;
      while(ini < end) {
        unsigned char a = front[ini].lastmask & smask || !(front[ini].lastmask & hmask);
        unsigned char b = front[end].lastmask & smask || !(front[end].lastmask & hmask);

        if(!a && b) {
          // Swaps info
          FRONTINFO t = front[ini];
          front[ini] = front[end];
          front[end] = t;

          register int tmp, *vert1 = &vertices[ini * 3], *vert2 = &vertices[end * 3];
          //Swaps triangles
          tmp = vert1[GCG_VAPEX]; vert1[GCG_VAPEX] = vert2[GCG_VAPEX]; vert2[GCG_VAPEX] = tmp;
          tmp = vert1[GCG_VESQ];  vert1[GCG_VESQ]  = vert2[GCG_VESQ];  vert2[GCG_VESQ]  = tmp;
          tmp = vert1[GCG_VDIR];  vert1[GCG_VDIR]  = vert2[GCG_VDIR];  vert2[GCG_VDIR]  = tmp;
          ini++; end--;
        } else {
          if(a) ini++;
          if(!b) end--;
        }
      }

      // Accounts for the last one
      if(ini == end && (front[ini].lastmask & smask || !(front[ini].lastmask & hmask))) ini++;

      this->hidemask = hmask;
      this->showmask = smask;
      this->nhidden  = ntriangulos - ini;
    }

  return ntriangulos - nhidden;
}



/*
int           posfaixas;    // Quantidade de vertices usados
int           *faixas;      // Buffer para manter a sequencia de faixas (STRIPS)
int           maxfaixas;    // Capacidade do buffer de faixas

int gcgSEMIREGULAR48::enfaixar(int inodo, int n, int v[]) {
    // Verifica se há espaço no buffer de faixas
    if(maxfaixas <= posfaixas + n + 1) return n;

    // Testa a base
    if(inodo >= 0) {
      NODO  *nodo = &nodos[inodo];

      if(nodo->itriang >= 0 && nodo->processo != processo)
        if(triflags[nodo->itriang] != TRIANGULO_INVISIVEL) {
          int *triang = &vertices[nodo->itriang * 3];
          register int v1  = v[n-2];
          register int v2  = v[n-1];
          int a = n;

          if(n & 0x01) { // Is odd?
              if(v1 == triang[VAPEX] && v2 == triang[VDIR] || v1 == triang[VDIR]  && v2 == triang[VAPEX])  v[n++] = triang[VESQ];
              else if(v1 == triang[VAPEX] && v2 == triang[VESQ] || v1 == triang[VESQ]  && v2 == triang[VAPEX])  v[n++] = triang[VDIR];
              else if(v1 == triang[VESQ]  && v2 == triang[VDIR] || v1 == triang[VDIR]  && v2 == triang[VESQ])  v[n++] = triang[VAPEX];
          } else {
              if(v1 == triang[VAPEX] && v2 == triang[VESQ] || v1 == triang[VESQ]  && v2 == triang[VAPEX])  v[n++] = triang[VDIR];
              else if(v1 == triang[VAPEX] && v2 == triang[VDIR] || v1 == triang[VDIR]  && v2 == triang[VAPEX])  v[n++] = triang[VESQ];
              else if(v1 == triang[VESQ]  && v2 == triang[VDIR] || v1 == triang[VDIR]  && v2 == triang[VESQ])  v[n++] = triang[VAPEX];
            }

          if(a != n) {
            nodo->processo = processo;

            n = enfaixar(nodo->vizdir,  n, v);
            n = enfaixar(nodo->vizesq,  n, v);
            n = enfaixar(nodo->vizbase, n, v);
          }
        }
    }
    return n;
}

///////////////////////////////////////////////////////////////////////////////
// Entrega uma sequencia de triangulos.
///////////////////////////////////////////////////////////////////////////////
unsigned int gcgSEMIREGULAR48::sequenciarTriangulos(PROCESSA_MALHA *proc) {
//    int ntri = 0;
//    int *triang = vertices;
//    for(int i = 0; i < ntriangulos; i++, triang += 3)
//      if(triflags[i] != TRIANGULO_INVISIVEL) {
//  		  proc->triangulo(triang); // Entrega os triangulos gerados
//  		  ntri++;
//      }
//    return ntri;

    proc->triangulo(ntriangulos * 3, vertices); // Entrega todos os triangulos gerados
    return ntriangulos;
}

///////////////////////////////////////////////////////////////////////////////
// Constroi varias sequencias de faixas de triangulos (STRIPS).
///////////////////////////////////////////////////////////////////////////////
int gcgSEMIREGULAR48::construirfaixas() {
    int *vertex;
    int *nvertices;
    int nfaixas = 0;
    int inodo = 0;

    // Vamos comecar um novo processamento dos nodos
    processo++;
    posfaixas = 0;

    for(NODO *nodo = nodos; inodo < nnodos; inodo++, nodo++)
      if(nodo->processo != processo && nodo->itriang >= 0) {
        int *triang = &vertices[nodo->itriang * 3];

        if(triflags[nodo->itriang] != TRIANGULO_INVISIVEL) {
          // Marca esse nodo como processado
          nodo->processo = processo;

          // Verifica se há espaço no buffer de faixas
          if(maxfaixas <= posfaixas + 256) {// Espectativa de tamanho de uma faixa = 256
            maxfaixas += 2048;
            faixas = (int*) REALLOC(faixas, sizeof(int) * maxfaixas);
            if(!faixas)  throw;
          }

          // Ponteiros para esse STRIP
          nvertices = (int*) &faixas[posfaixas];
          vertex  = &faixas[posfaixas+1];

          *nvertices = 3;
          // Cria faixa pelo vizinho direito
          vertex[0] = triang[VDIR];
          vertex[1] = triang[VAPEX];
          vertex[2] = triang[VESQ];
          *nvertices = enfaixar(nodo->vizdir, 3, vertex);

          // Se nao deu certo, cria faixa pelo vizinho esquerdo
          if(*nvertices == 3) {
            vertex[0] = triang[VESQ];
            vertex[1] = triang[VAPEX];
            vertex[2] = triang[VDIR];
            *nvertices = enfaixar(nodo->vizesq, 3, vertex);
          }

          // Se nao deu certo, cria faixa pelo vizinho base
          if(*nvertices == 3) {
            vertex[0] = triang[VAPEX];
            vertex[1] = triang[VESQ];
            vertex[2] = triang[VDIR];
            *nvertices = enfaixar(nodo->vizbase, 3, vertex);
          }

          // Mesmo os triangulos sao colocados como strips
          posfaixas += *nvertices + 1;

          // Mais uma faixa
          nfaixas++;
        }
      }

    return nfaixas;
}

///////////////////////////////////////////////////////////////////////////////
// Entrega uma sequencia de faixas de triangulos (STRIPS).
///////////////////////////////////////////////////////////////////////////////
unsigned int gcgSEMIREGULAR48::sequenciarFaixas(PROCESSA_MALHA *proc) {
    int nstrips = 0, ntri = 0;

    // Verifica se as faixas foram geradas
    if(posfaixas == 0) construirfaixas();

    for(int i = 0, pos = 0; pos < posfaixas; i++, pos += (int) faixas[pos]+1) {//if((int)faixas[pos] > 128) {
      proc->faixa(faixas[pos], &faixas[pos+1]);
      ntri += faixas[pos]-2;
      nstrips++;
    }

    return ntri;
}
*/
