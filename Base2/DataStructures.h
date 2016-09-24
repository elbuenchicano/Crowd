#ifndef _DATA_STRS_H_
#define _DATA_STRS_H_

#include <iostream>
#include <string.h>
#include <vector>
#include <list>
#include <math.h>
#include <assert.h>

///---------------Btree Nodo Base----------
//-----------------------------------------

template <class t>
struct RBnodo
{
  t			    dato;
  RBnodo<t>	*hijos[2];
  bool      color = true;
  RBnodo(t dat){
    dato = dat;
    hijos[0] = nullptr;
    hijos[1] = nullptr;
  }
};

///---------------Btree function ob--------
//-----------------------------------------
template <class t>
struct comp{ bool operator ()(t a, t b) { return a < b; } };

//---------------------------------------
//---------------------------------------
struct arbol_tr
{
  typedef		int				    tipo;
  typedef		RBnodo<tipo>	nodo;
  typedef		comp<tipo>		compa;

};

///---------------Btree base----------
//------------------------------------

template <class tr>
struct RBTree
{
  typedef	typename tr::nodo   nodo;
  typedef	typename tr::tipo   tipo;
  typedef	typename tr::compa  compa;
  typedef std::pair<nodo**, bool>  par;
  typedef std::vector<par>         vecp;
  typedef std::pair<nodo*, int>    parni;
  typedef std::vector<parni>       vecn;

  int      nelem_ = 0;
  nodo		 *raiz_ = nullptr;
  RBTree() {}

  bool    insertar(tipo);
  bool    find(tipo, nodo**&, vecp &);
  void    mostrar();
  void    mostrar(nodo *);
  void    mostrar(vecn &);
  void    recolectar(vecn &);
  void    balance(vecp &);
  void    rotacion(nodo **, bool);
  void    caso1(nodo **);
  void    caso2(nodo **, bool);
  void    caso3(nodo **, bool);
};
typedef RBTree<arbol_tr>  rbt;

//------------------------------------

template <class tr>
bool    RBTree<tr>::insertar(tipo d_)
{
  if (!raiz_){
    raiz_ = new nodo(d_);
    raiz_->color = false;
  }
  else{
    nodo ** n_ = nullptr;
    vecp    camino_;
    if (find(d_, n_, camino_))return false;
    *n_ = new nodo(d_);
    camino_.push_back(par(n_, false));
    balance(camino_);
  }
  ++nelem_;
  return true;
}

//------------------------------------
template <class tr>
bool    RBTree<tr>::find(tipo d_, nodo ** &n_, vecp & camino_)
{
  bool ch_;
  for (n_ = &raiz_; *n_; ){
    if ((*n_)->dato == d_)return true;
    ch_ = (*n_)->dato < d_;
    camino_.push_back(par(n_, ch_));
    n_ = &(*n_)->hijos[ch_];
  }
  return false;
}

//------------------------------------

template <class tr>
void    RBTree<tr>::mostrar()
{
  vecn no_;
  recolectar(no_);
  mostrar(no_);
}

//------------------------------------
template <class tr>
void    RBTree<tr>::mostrar(nodo * n_)
{
  if (n_){
    mostrar(n_->hijos[0]);
    cout << n_->dato;
    mostrar(n_->hijos[1]);
  }
}
//------------------------------------


template <class t>
static inline int mostrar_es(t dato, int p)
{
  for (int i = 0; i<p; ++i)
  {
    std::cout << ".";
  }
  cout << dato;
  for (int i = 0; i<p; ++i)
  {
    std::cout << ".";
  }
  return p * 2 + 1;
}
static inline int mostrar_es(int p)
{
  int r = 2 * p + 1;
  for (int i = 0; i<r; ++i)
  {
    std::cout << ".";
  }
  return r;
}


template <class tr>
void    RBTree<tr>::mostrar(vecn & no_)
{
  if (no_.size() == 0)return;
  int final = no_[nelem_ - 1].second + 1,
    pro = log2(no_[nelem_ - 1].second + 1) + 1,
    pos = 1;
  int salto = pow(2, pro - 1) - 1;
  char col[]{ 'B','R' };

  mostrar_es(raiz_->dato, salto * 2);
  cout << "." << col[no_[0].first->color ? 1 : 0] << ".";

  for (int i = 2; i <= final; ++i)
  {
    if (log2(i) - (int)log2(i) == 0)
    {
      pro--;
      salto = pow(2, pro - 1) - 1;
      cout << endl;
    }
    if (no_[pos].second + 1 == i)
    {
      mostrar_es(no_[pos++].first->dato, salto * 2);
      cout << "." << col[no_[pos - 1].first->color ? 1 : 0] << ".";
    }
    else
    {
      mostrar_es(salto * 2);
      cout << "...";
    }
  }
  cout << endl << endl;
}

//------------------------------------

template <class tr>
void    RBTree<tr>::recolectar(vecn & no_)
{
  if (raiz_)
  {
    no_.push_back(parni(raiz_, 0));
    for (int i = 0; i<nelem_; i++)
    {
      if (no_[i].first->hijos[0])
        no_.push_back(parni
          (no_[i].first->hijos[0],
            no_[i].second * 2 + 1));
      if (no_[i].first->hijos[1])
        no_.push_back(parni
          (no_[i].first->hijos[1],
            no_[i].second * 2 + 2));
    }
  }
}
//-------------------------------------

//rotacion a la derecha 1 izquierda 0
template <class tr>
void    RBTree<tr>::rotacion(nodo ** n, bool d)
{
  nodo * temp = (*n)->hijos[!d];
  assert(temp);
  (*n)->hijos[!d] = temp->hijos[d];
  temp->hijos[d] = *n;
  *n = temp;
}

//-----------------------------------

template <class tr>
void    RBTree<tr>::caso1(nodo ** p)
{
  (*p)->hijos[0]->color = false;
  (*p)->hijos[1]->color = false;
  (*p)->color = true;
}

template <class tr>
void    RBTree<tr>::caso3(nodo ** p, bool c)
{
  //intercambiar color entre el padre y el abuelo
  (*p)->color = true;
  (*p)->hijos[c]->color = false;
  //rotacion contraria al camino
  rotacion(p, !c);

}

template <class tr>
void    RBTree<tr>::caso2(nodo ** a, bool c)
{
  //rotacion contraria al camino
  rotacion(&(*a)->hijos[c], c);
  caso3(a, c);
}



template <class tr>
void    RBTree<tr>::balance(vecp & no_)
{

  int i = no_.size() - 1;
  while (i>0 && (*no_[i].first)->color && (*no_[i - 1].first)->color)
  {
    i -= 2;
    if ((*no_[i].first)->hijos[!no_[i].second] && (*no_[i].first)->hijos[!no_[i].second]->color)
      caso1(no_[i].first);
    else
    {
      if (no_[i].second == no_[i + 1].second)
        caso3(no_[i].first, no_[i].second);
      else
        caso2(no_[i].first, no_[i].first);
    }
  }
  raiz_->color = false;
}

//----------------------------------------

////////////////////////////////////////////////////////////////////////////////

template <class t>
struct Segment {
  t _ini = 0, 
    _fin = 0;
  //...........................................................................
  bool operator < (Segment<t> seg) { return ini < seg.ini; }
  Segment(t ini, t fin):
    _ini(ini),
    _fin(fin){}
  Segment() {}
};



//binary search for segments into a segment vector
template <class t>
bool datastr_findSegmentVec(std::vector<Segment<t> > & vec, t data) {
  int mpos;
  for (int i = 0, f = vec.size() - 1; i <= f; ) {
    mpos = (i + f >> 1);
    if (data >= vec[mpos]._ini && data <= vec[mpos]._fin) return true;
    if (data <= vec[mpos]._ini)
      f = mpos - 1;
    else
      i = mpos + 1;
  }
  return false;
}
////////////////////////////////////////////////////////////////////////////////

#endif//_DATA_STRS_H_
