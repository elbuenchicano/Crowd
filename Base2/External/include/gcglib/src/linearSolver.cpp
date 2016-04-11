#include "gcg.h"

#define SIGN(a, b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

static double PYTHAG(double a, double b) {
  double at = fabs(a), bt = fabs(b), ct, result;

  if(at > bt) {
    ct = bt / at;
    return at * sqrt(1.0 + ct * ct);
  } else
      if(bt > 0.0) {
        ct = at / bt;
        return bt * sqrt(1.0 + ct * ct);
      }

  return 0.0;
}

bool dsvd(gcgDISCRETE2D<double> *a, gcgDISCRETE1D<double> *w, gcgDISCRETE2D<double> *v, int m, int n){
    int flag, i, its, j, jj, k, l, nm;
    double c, f, h, s, x, y, z;
    double anorm = 0.0, g = 0.0, scale = 0.0;
    gcgDISCRETE1D<double> *rv1;

    if(m < n) {
      fprintf(stderr, "#rows must be > #cols \n");
      return(0);
    }

    rv1 = new gcgDISCRETE1D<double>(n, 0);

    /* Householder reduction to bidiagonal form */
    for(i = 0; i < n; i++) {
        /* left-hand reduction */
        l = i + 1;
        rv1->data[i] = scale * g;
        g = s = scale = 0.0;
        if(i < m) {
            for(k = i; k < m; k++)
                scale += fabs((double)a->data[k*n + i]);
            if(scale) {
                for(k = i; k < m; k++) {
                    a->data[k * n + i] = (double) ((double) a->data[k * n + i] / scale);
                    s += ((double) a->data[k * n + i] * (double) a->data[k * n + i]);
                }
                f = (double) a->data[i*n + i];
                g = -SIGN(sqrt(s), f);
                h = f * g - s;
                a->data[i * n + i] = (double) (f - g);

                if(i != n - 1) {
                    for (j = l; j < n; j++) {
                        for (s = 0.0, k = i; k < m; k++)
                            s += ((double) a->data[k * n + i] * (double) a->data[k * n + j]);
                        f = s / h;
                        for (k = i; k < m; k++)
                            a->data[k * n + j] += (double) (f * (double) a->data[k * n + i]);
                    }
                }
                for(k = i; k < m; k++)
                    a->data[k * n + i] = (double) ((double)a->data[k * n + i] * scale);
            }
        }
        w->data[i] = (double) (scale * g);

        /* right-hand reduction */
        g = s = scale = 0.0;
        if(i < m && i != n - 1) {
            for(k = l; k < n; k++)
                scale += fabs((double) a->data[i * n + k]);

            if(scale) {
                for(k = l; k < n; k++) {
                    a->data[i * n + k] = (double) ((double) a->data[i * n + k] / scale);
                    s += ((double) a->data[i * n + k] * (double) a->data[i * n + k]);
                }
                f = (double) a->data[i * n + l];
                g = -SIGN(sqrt(s), f);
                h = f * g - s;
                a->data[i * n + l] = (double)(f - g);

                for(k = l; k < n; k++)
                    rv1->data[k] = (double)a->data[i * n + k] / h;

                if(i != m - 1) {
                    for(j = l; j < m; j++) {
                        for(s = 0.0, k = l; k < n; k++)
                            s += ((double) a->data[j * n + k] * (double) a->data[i * n + k]);

                        for(k = l; k < n; k++)
                            a->data[j * n + k] += (double) (s * rv1->data[k]);
                    }
                }

                for(k = l; k < n; k++)
                    a->data[i * n + k] = (double) ((double) a->data[i * n + k] * scale);
            }
        }
        anorm = MAX(anorm, (fabs((double) w->data[i]) + fabs(rv1->data[i])));
    }

    /* accumulate the right-hand transformation */
    for(i = n - 1; i >= 0; i--) {
        if(i < n - 1) {
            if(g) {
                for(j = l; j < n; j++)
                    v->data[j * n + i] = (double) (((double) a->data[i * n + j] / (double) a->data[i * n + l]) / g);
                    /* double division to avoid underflow */

                for(j = l; j < n; j++) {
                    for(s = 0.0, k = l; k < n; k++)
                        s += ((double) a->data[i * n + k] * (double) v->data[k * n + j]);

                    for(k = l; k < n; k++)
                        v->data[k * n + j] += (double) (s * (double) v->data[k * n + i]);
                }
            }
            for(j = l; j < n; j++)
                v->data[i * n + j] = v->data[j * n + i] = 0.0;
        }
        v->data[i * n + i] = 1.0;
        g = rv1->data[i];
        l = i;
    }

    /* accumulate the left-hand transformation */
    for(i = n - 1; i >= 0; i--) {
        l = i + 1;
        g = (double) w->data[i];
        if(i < n - 1)
            for (j = l; j < n; j++) a->data[i * n + j] = 0.0;
        if(g) {
            g = 1.0 / g;
            if(i != n - 1) {
                for(j = l; j < n; j++) {
                    for(s = 0.0, k = l; k < m; k++)
                        s += ((double) a->data[k * n + i] * (double) a->data[k * n + j]);
                    f = (s / (double) a->data[i * n + i]) * g;
                    for(k = i; k < m; k++)
                        a->data[k * n + j] += (double) (f * (double) a->data[k * n + i]);
                }
            }
            for(j = i; j < m; j++)
                a->data[j * n + i] = (double) ((double) a->data[j * n + i]*g);
        } else
            for(j = i; j < m; j++) a->data[j * n + i] = 0.0;

        ++a->data[i * n + i];
    }

    /* diagonalize the bidiagonal form */
    for(k = n - 1; k >= 0; k--) {  /* loop over singular values */
        for(its = 0; its < 30; its++) {  /* loop over allowed iterations */
            flag = 1;
            for(l = k; l >= 0; l--) {  /* test for splitting */
                nm = l - 1;
                if(fabs(rv1->data[l]) + anorm == anorm) {
                    flag = 0;
                    break;
                }
                if(fabs((double) w->data[nm]) + anorm == anorm) break;
            }

            if(flag) {
                c = 0.0;
                s = 1.0;
                for(i = l; i <= k; i++) {
                    f = s * rv1->data[i];
                    if(fabs(f) + anorm != anorm) {
                        g = (double) w->data[i];
                        h = PYTHAG(f, g);
                        w->data[i] = (double) h;
                        h = 1.0 / h;
                        c = g * h;
                        s = (- f * h);
                        for(j = 0; j < m; j++) {
                            y = (double) a->data[j * n + nm];
                            z = (double) a->data[j * n + i];
                            a->data[j * n + nm] = (double)(y * c + z * s);
                            a->data[j * n + i] = (double)(z * c - y * s);
                        }
                    }
                }
            }
            z = (double) w->data[k];
            if(l == k) { /* convergence */
                if(z < 0.0) { /* make singular value nonnegative */
                    w->data[k] = (double) (-z);
                    for(j = 0; j < n; j++)
                        v->data[j * n + k] = (-v->data[j * n + k]);
                }
                break;
            }
            if(its >= 30) {
              delete rv1;
              fprintf(stderr, "No convergence after 30,000! iterations \n");
              return(0);
            }

            /* shift from bottom 2 x 2 minor */
            x = (double) w->data[l];
            nm = k - 1;
            y = (double) w->data[nm];
            g = rv1->data[nm];
            h = rv1->data[k];
            f = ((y - z) * (y + z) + (g - h) * (g + h)) / (2.0 * h * y);
            g = PYTHAG(f, 1.0);
            f = ((x - z) * (x + z) + h * ((y / (f + SIGN(g, f))) - h)) / x;

            /* next QR transformation */
            c = s = 1.0;
            for (j = l; j <= nm; j++) {
                i = j + 1;
                g = rv1->data[i];
                y = (double) w->data[i];
                h = s * g;
                g = c * g;
                z = PYTHAG(f, h);
                rv1->data[j] = z;
                c = f / z;
                s = h / z;
                f = x * c + g * s;
                g = g * c - x * s;
                h = y * s;
                y = y * c;
                for (jj = 0; jj < n; jj++) {
                    x = (double) v->data[jj * n + j];
                    z = (double) v->data[jj * n + i];
                    v->data[jj * n + j] = (double) (x * c + z * s);
                    v->data[jj * n + i] = (double) (z * c - x * s);
                }
                z = PYTHAG(f, h);
                w->data[j] = (double)z;
                if(z) {
                    z = 1.0 / z;
                    c = f * z;
                    s = h * z;
                }
                f = (c * g) + (s * y);
                x = (c * y) - (s * g);
                for(jj = 0; jj < m; jj++) {
                    y = (double) a->data[jj * n + j];
                    z = (double) a->data[jj * n + i];
                    a->data[jj * n + j] = (double) (y * c + z * s);
                    a->data[jj * n + i] = (double) (z * c - y * s);
                }
            }
            rv1->data[l] = 0.0;
            rv1->data[k] = f;
            w->data[k] = (double) x;
        }
    }

    delete rv1;
    return true;
}


bool gcgLinearSolver(gcgDISCRETE2D<double> *A, gcgDISCRETE1D<double> *b, gcgDISCRETE1D<double> *x, int m, int n){
    gcgDISCRETE1D<double> *w, *aux;
    gcgDISCRETE2D<double> *v, *A1;
    int i, j;

    //verifica se m > n
    if(m < n){
        fprintf(stderr, "Erro: numero de equacoes deve ser maior que o numero de incognitas");
        return 0;
    }

    //alocação das matrizes e vetores utilizados
    A1 = new gcgDISCRETE2D<double>(n, m, 0, 0);
    w = new gcgDISCRETE1D<double>(n, 0);
    v = new gcgDISCRETE2D<double>(n, n, 0, 0);
    aux = new gcgDISCRETE1D<double>(n, 0);

    //copia A para A1
    A1->duplicateSignal(A);

    //cálculo da decomposição SVD
    dsvd(A1, w, v);

    //resolução do sistema utilizando a fórmula x = V(1/W)U'
    //onde 1/W é a matriz diagonal formada pelos elementos 1/wi
    //e U' é a transposta de U
    for(i = 0; i < n; i++){
        aux->data[i] = 0.0;
        for(j = 0; j < m; j++) aux->data[i] += A1->data[j*n+i]*b->data[j];
        aux->data[i] = aux->data[i]/w->data[i];
    }

    for(i = 0; i < n; i++){
        x->data[i] = 0;
        for(j = 0; j < n; j++) x->data[i] += v->data[i*n+j]*aux->data[j];
    }

    //liberação das matrizes e vetores
    delete A1;
    delete w;
    delete v;
    delete aux;

    return true;
}
