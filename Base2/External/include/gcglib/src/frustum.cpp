/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

    FRUSTUM.CPP: class for handling viewing frustums.

    Marcelo Bernardes Vieira
**************************************************************************************/

#include "system.h"

//////////////////////////////////////////////////////////////////////////////
// CLASS gcgFRUSTUM
//////////////////////////////////////////////////////////////////////////////
//http://web.archive.org/web/20041029003853/
//http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q2

gcgFRUSTUM::gcgFRUSTUM() {
  // Inicializacao do quaternion de orientation
  gcgSETVECTOR4(orientation, 0.0f, 0.0f, 0.0f, 1.0f);

  // Inicializacao da perspectiva (Calcula os parametros)
  setPerspective(30.0f, 1.0f, 0.0001f, 5000.0f);
}

gcgFRUSTUM::~gcgFRUSTUM() {
}

// Posicao do observador: codifica a nova posicao em um quaternion com distancia
void gcgFRUSTUM::setPosition(float x, float y, float z) {
  // Atualiza posicao e distancia a origem
  this->x = x;
  this->y = y;
  this->z = z;

  computeParameters();
}

// Posicao do observador: codifica a nova posicao em um quaternion com distancia
void gcgFRUSTUM::setPosition(double x, double y, double z) {
  // Atualiza posicao e distancia a origem
  this->x = (float) x;
  this->y = (float) y;
  this->z = (float) z;

  computeParameters();
}

// Posicao do observador: codifica a nova posicao em um quaternion com distancia
void gcgFRUSTUM::setPosition(VECTOR3 pos) {
  // Atualiza posicao e distancia a origem
  this->x = pos[0];
  this->y = pos[1];
  this->z = pos[2];

  computeParameters();
}

// Posicao do observador: codifica a nova posicao em um quaternion com distancia
void gcgFRUSTUM::setPosition(VECTOR3d pos) {
  // Atualiza posicao e distancia a origem
  this->x = (float) pos[0];
  this->y = (float) pos[1];
  this->z = (float) pos[2];

  computeParameters();
}


// Move o observador, dado um vetor
void gcgFRUSTUM::movePosition(float dx, float dy, float dz) {
  setPosition(x + dx, y + dy, z + dz);
}

// Move o observador, dado um vetor
void gcgFRUSTUM::movePosition(double dx, double dy, double dz) {
  setPosition(x + dx, y + dy, z + dz);
}

// Move o observador, dado um vetor
void gcgFRUSTUM::movePosition(VECTOR3 d) {
  setPosition(x + d[0], y + d[1], z + d[2]);
}

// Move o observador, dado um vetor
void gcgFRUSTUM::movePosition(VECTOR3d d) {
  setPosition(x + d[0], y + d[1], z + d[2]);
}

// Avanca a posicao do observador
void gcgFRUSTUM::advancePosition(float frente, float acima, float direita) {
  setPosition(x + -view[0]*frente + right[0]*direita + up[0]*acima,
              y + -view[1]*frente + right[1]*direita + up[1]*acima,
              z + -view[2]*frente + right[2]*direita + up[2]*acima);
}

// Avanca a posicao do observador
void gcgFRUSTUM::advancePosition(double frente, double acima, double direita) {
  setPosition(x + -view[0]*frente + right[0]*direita + up[0]*acima,
              y + -view[1]*frente + right[1]*direita + up[1]*acima,
              z + -view[2]*frente + right[2]*direita + up[2]*acima);
}


void gcgFRUSTUM::resetOrientation() {
  gcgSETVECTOR4(orientation, 0.0, 0.0, 0.0, 1.0);
  computeParameters();
}

// Configurador da direcao de visualizacao por vetor
void gcgFRUSTUM::setViewVector(float dirx, float diry, float dirz) {
  VECTOR3d v = {dirx, diry, dirz};
  setViewVector(v);
}

// Configurador da direcao de visualizacao por vetor
void gcgFRUSTUM::setViewVector(double dirx, double diry, double dirz) {
  VECTOR3d v = {dirx, diry, dirz};
  setViewVector(v);
}

// Configurador da direcao de visualizacao por vetor
void gcgFRUSTUM::setViewVector(VECTOR3 dir) {
  VECTOR3d v = {dir[0], dir[1], dir[2]};
  setViewVector(v);
}

// Configurador da direcao de visualizacao por vetor
void gcgFRUSTUM::setViewVector(VECTOR3d v) {
  VECTOR3d dif;

  gcgNORMALIZEVECTOR3(v, v);
  gcgSUBVECTOR3(dif, v, view);

  if(gcgLENGTHVECTOR3(dif) > EPSILON) {
    double   theta;
    VECTOR4d quat;
    VECTOR3d n;
    VECTOR3d viewcopy;
    gcgCOPYVECTOR3(viewcopy, view);

    // Find normal and angle cosine
    gcgCROSSVECTOR3(n, view, v);
    theta = gcgDOTVECTOR3(v, viewcopy);

    // Check normal
    if(gcgLENGTHVECTOR3(n) < 4.0*EPSILON) {
      gcgCOPYVECTOR3(n, right);
    } else gcgNORMALIZEVECTOR3(n, n);

    // Check angle
    if(theta > 1.0) theta = 1.0;
    else if(theta < -1.0) theta = -1.0;

    theta = acos(theta);

    // Gira camera
    gcgAXISTOQUATERNION(quat, theta, n);
    gcgNORMALIZEQUATERNION(quat, quat);               // to avoid numeric errors
    gcgMULTQUATERNION(orientation, orientation, quat);
    gcgNORMALIZEQUATERNION(orientation, orientation);  // to avoid numeric errors

    computeParameters();
    // Force to avoid errors
    view[0] = (float) v[0]; view[1] = (float) v[1]; view[2] = (float) v[2];
  }
}

// Configurador da direcao de visualizacao com coordenadas geograficas (polares)
void gcgFRUSTUM::setViewVectorSpherical(float latit, float longit) {
  setViewVectorSpherical((double) latit, (double) longit);
}

// Configurador da direcao de visualizacao com coordenadas geograficas (polares)
void gcgFRUSTUM::setViewVectorSpherical(double latit, double longit) {
  VECTOR3d eixo;
  VECTOR4d quat;

  latit  = DEG2RAD(latit);
  longit = DEG2RAD(longit);

  latit  = fmod(latit,  M_PI);
  longit = fmod(longit, M_PI);

    // Reinicializa quaternion de orientacao
  gcgSETVECTOR4(orientation, 0.0, 0.0, 0.0, 1.0);

  // Gira em Y
  gcgSETVECTOR3(eixo, 0, 1, 0);
  gcgAXISTOQUATERNION(quat, longit, eixo);
  gcgMULTQUATERNION(orientation, quat, orientation);

  // Gira em X
  gcgSETVECTOR3(eixo, 1, 0, 0);
  gcgAXISTOQUATERNION(quat, -latit, eixo); // Positivo sobe
  gcgMULTQUATERNION(orientation, quat, orientation);

  computeParameters();
}

// Configurador do vetor cima por vetor
void gcgFRUSTUM::setUpVector(float cimax, float cimay, float cimaz) {
  VECTOR3d v = {cimax, cimay, cimaz};
  setUpVector(v);
}

// Configurador do vetor cima por vetor
void gcgFRUSTUM::setUpVector(double cimax, double cimay, double cimaz) {
  VECTOR3d v = {cimax, cimay, cimaz};
  setUpVector(v);
}

// Configurador do vetor cima por vetor
void gcgFRUSTUM::setUpVector(VECTOR3 dir) {
  VECTOR3d v = {dir[0], dir[1], dir[2]};
  setUpVector(v);
}

// Configurador do vetor cima por vetor
void gcgFRUSTUM::setUpVector(VECTOR3d v) {
  VECTOR3d dif;

  gcgNORMALIZEVECTOR3(v, v);
  gcgSUBVECTOR3(dif, v, up);

  if(gcgLENGTHVECTOR3(dif) > EPSILON) {
    VECTOR3d n;
    VECTOR4d quat;
    double theta;
    VECTOR3d upcopy;
    gcgCOPYVECTOR3(upcopy, up);


    // Find normal and angle cosine
    gcgCROSSVECTOR3(n, up, v);
    theta = gcgDOTVECTOR3(v, upcopy);

    // Check normal
    if(gcgLENGTHVECTOR3(n) < 4.0*EPSILON) {
      gcgCOPYVECTOR3(n, view);
    } else gcgNORMALIZEVECTOR3(n, n);

    // Check angle
    if(theta > 1.0) theta = 1.0;
    else if(theta < -1.0) theta = -1.0;

    theta = acos(theta);

    // Gira camera
    gcgAXISTOQUATERNION(quat, theta, n);
    gcgNORMALIZEQUATERNION(quat, quat);         // to avoid numeric errors
    gcgMULTQUATERNION(orientation, orientation, quat);
    gcgNORMALIZEQUATERNION(orientation, orientation);  // to avoid numeric errors

    computeParameters();
    // Force to avoid errors
    up[0] = (float) v[0]; up[1] = (float) v[1]; up[2] = (float) v[2];
  }
}



// Ajusta as direcoes para visualizar o alvo.
void gcgFRUSTUM::setTarget(float alvoX, float alvoY, float alvoZ) {
  setViewVector(x - alvoX, y - alvoY, z - alvoZ);
}

// Ajusta as direcoes para visualizar o alvo.
void gcgFRUSTUM::setTarget(double alvoX, double alvoY, double alvoZ) {
  setViewVector(x - alvoX, y - alvoY, z - alvoZ);
}

// Ajusta as direcoes para visualizar o alvo.
void gcgFRUSTUM::setTarget(VECTOR3 alvo) {
  setViewVector(x - alvo[0], y - alvo[1], z - alvo[2]);
}

// Ajusta as direcoes para visualizar o alvo.
void gcgFRUSTUM::setTarget(VECTOR3d alvo) {
  setViewVector(x - alvo[0], y - alvo[1], z - alvo[2]);
}


  // Funcao para girar a esfera que envolve o visualizador
// Trackball: p1,p2 em [-1,1]x[-1,1] plano no qual a bola é projetada. rotacao livre.
void gcgFRUSTUM::rotateOrientationTrackball(double x1, double y1, double x2, double y2) {
  VECTOR4d   quat;  // Vetor velocidade angular da bola em forma de quartenion

  gcgTrackball(quat, x1, y1, x2, y2);
  gcgMULTQUATERNION(orientation, quat, orientation);

  // Atualiza vetores de direcao
  computeParameters();
}

// Gira o planetário usando deslocamentos em angulos geograficos.
// Rotacao livre em graus do planetario.
void gcgFRUSTUM::rotateOrientationSpherical(double dlat, double dlong) {
  VECTOR3d eixo;
  VECTOR4d quat;

  dlat  = DEG2RAD(dlat);
  dlong = DEG2RAD(dlong);

  // Gira em X
  gcgSETVECTOR3(eixo, 1, 0, 0);
  gcgAXISTOQUATERNION(quat, -dlat, eixo);    // Positivo sobe: mao direita
  gcgMULTQUATERNION(orientation, quat, orientation);

  // Gira em Y
  gcgSETVECTOR3(eixo, 0, 1, 0);
  gcgAXISTOQUATERNION(quat, dlong, eixo);
  gcgMULTQUATERNION(orientation, quat, orientation);

  computeParameters();
}


// Rotation with Euler angles using the LOCAL COORDINATE SYSTEM.
void gcgFRUSTUM::rotateOrientationEuler(double pitch, double yaw, double roll) {
  VECTOR4d qx, qy, qz, quat;

  pitch = DEG2RAD(pitch);
  yaw   = DEG2RAD(yaw);
  roll  = DEG2RAD(roll);

  VECTOR3d viewcopy, upcopy, rightcopy;
  gcgCOPYVECTOR3(viewcopy, view);
  gcgCOPYVECTOR3(upcopy, up);
  gcgCOPYVECTOR3(rightcopy, right);

  gcgAXISTOQUATERNION(qx, pitch, rightcopy);
  gcgAXISTOQUATERNION(qy, yaw,   upcopy);
  gcgAXISTOQUATERNION(qz, roll,  viewcopy);

  gcgMULTQUATERNION(quat, qy,   qx);
  gcgMULTQUATERNION(quat, qz, quat);
  gcgMULTQUATERNION(orientation, orientation, quat);

  computeParameters();
}


// Rotation of the planetarium around a given axis.
void gcgFRUSTUM::rotateOrientationAxis(double theta, double axisX, double axisY, double axisZ) {
  VECTOR4d quat;
  VECTOR3d n;

  theta = DEG2RAD(theta);
  // Gira camera
  gcgSETVECTOR3(n, axisX, axisY, axisZ);
  gcgNORMALIZEVECTOR3(n, n);
  gcgAXISTOQUATERNION(quat, theta, n);
  gcgNORMALIZEQUATERNION(quat, quat);               // to avoid numeric errors
  gcgMULTQUATERNION(orientation, orientation, quat);
  gcgNORMALIZEQUATERNION(orientation, orientation);  // to avoid numeric errors

  computeParameters();
}


// Alinha o cima com Y, e lado com X
void gcgFRUSTUM::alignAxis() {
  VECTOR3d eixo;
  VECTOR4d quat;
  double lat, lon;

  // Calcula latitude
  if(view[1] >  1.0) view[1] = 1.0;
  if(view[1] < -1.0) view[1] = -1.0;
  lat = ((M_PI * 0.5) - acos(view[1]));      // Latitude atual

  if(lat > (M_PI*0.5 - 0.0076)) lat = (M_PI*0.5 - 0.0076); // Simply a small number
  else if(lat < -(M_PI*0.5 - 0.0076)) lat = -(M_PI*0.5 - 0.0076);

  // Temos que testar os quadrantes para a longitude em [0, 2PI)
  if(fabs(view[0]) < EPSILON) view[0] = (float) ((view[0] < 0.0)? -EPSILON : EPSILON);
  lon = (atan((double) view[2] / (double) view[0])); // Longitude atual
  if(view[0] < 0.0) lon += M_PI;
  lon = ((M_PI * 0.5) - lon);             // Longitude atual

  // Gira em Y
  gcgSETVECTOR3(eixo, 0, 1, 0);
  gcgAXISTOQUATERNION(orientation, lon, eixo);

  // Gira em X
  gcgSETVECTOR3(eixo, 1, 0, 0);
  gcgAXISTOQUATERNION(quat, -lat, eixo); // Positivo sobe
  gcgMULTQUATERNION(orientation, quat, orientation);

  computeParameters();
}

// Gira o planetario mas sempre alinha o vetor up com os meridianos
// e o vetor lateral com os paralelos.
// Rotacao em graus do planetario. Mantem os eixos alinhados com os paralelos
// e meridianos.
/*void gcgFRUSTUM::rotateOrientationAligned(float dlat, float dlong) {
  VECTOR4 eixo, quat;
  float   lat,  lon;

  dlat  = DEG2RAD(dlat);
  dlong = DEG2RAD(dlong);

  // Cacula latitude
  if(view[1] >  1.0) view[1] = 1.0;
  if(view[1] < -1.0) view[1] = -1.0;

  lat = (M_PI*0.5) - acos(view[1]);      // Latitude atual

  lat += dlat;                      // Nova latitude
  if(lat > (M_PI*0.5 - 0.0006)) lat = (M_PI*0.5 - 0.0006);
  else if(lat < -(M_PI*0.5 - 0.0006)) lat = -(M_PI*0.5 - 0.0006);

  // Temos que testar os quadrantes para a longitude em [0, 2PI)
  if(fabs(view[0]) < EPSILON) view[0] = (view[0] < 0.0)? -EPSILON : EPSILON;
  lon = atan(view[2]/view[0]); // Longitude atual
  if(view[0] < 0.0) lon += M_PI;

  lon = (M_PI*0.5)-lon;             // Longitude atual
  lon += dlong;                   // Nova longitude

  // Gira em Y
  gcgSETVECTOR3(eixo, 0, 1, 0);
  gcgAXISTOQUATERNION(orientation, lon, eixo);

  // Gira em X
  gcgSETVECTOR3(eixo, 1, 0, 0);
  gcgAXISTOQUATERNION(quat, -lat, eixo); // Positivo sobe
  gcgMULTQUATERNION(orientation, quat, orientation);

  computeParameters();
}*/



// Gira todo o sistema em torno de uma esfera com centro determinado.
// Trackball: p1,p2 em [-1,1]x[-1,1] plano no qual a esfera é projetada
void gcgFRUSTUM::rotateOrbitTrackball(double centroX, double centroY, double centroZ, double x1, double y1, double x2, double y2) {
  VECTOR4d quat, q;
  VECTOR3d r = {x - centroX, y - centroY, z - centroZ};

  gcgTrackball(quat, x1, y1, x2, y2);    // Calcula rotacao desejada
  gcgMULTQUATERNION(quat, quat, orientation);  // Alinha c/ sistema da camera+rotacao
  gcgCONJUGATEQUATERNION(q, orientation);
  gcgMULTQUATERNION(quat, q, quat);           // Desalinha o sistema da camera

  gcgAPPLYQUATERNIONVECTOR3(r, quat, r);      // Rotaciona o vetor r
  setPosition(r[0] + centroX, r[1] + centroY, r[2] + centroZ);
}


// Gira livremente a bola em graus.
void gcgFRUSTUM::rotateOrbitSpherical(double centroX, double centroY, double centroZ, double dlat, double dlong) { // Gira livremente a bola em graus.
  VECTOR4d quat, rot;
  VECTOR3d eixo, r = {x - centroX, y - centroY, z - centroZ};

  dlat  = DEG2RAD(dlat);
  dlong = DEG2RAD(dlong);

  // Gira em X
  gcgSETVECTOR3(eixo, 1, 0, 0);
  gcgAXISTOQUATERNION(rot, -dlat, eixo);    // Positivo sobe: mao direita

  // Gira em Y
  gcgSETVECTOR3(eixo, 0, 1, 0);
  gcgAXISTOQUATERNION(quat, dlong, eixo);
  gcgMULTQUATERNION(rot, quat, rot);

  gcgMULTQUATERNION(rot, rot, orientation);  // Alinha c/ sistema da camera+rotacao
  gcgCONJUGATEQUATERNION(quat, orientation);
  gcgMULTQUATERNION(rot, quat, rot);           // Desalinha o sistema da camera

  gcgAPPLYQUATERNIONVECTOR3(r, rot, r);       // Rotaciona o vetor r
  setPosition(r[0] + centroX, r[1] + centroY, r[2] + centroZ);
}


// Gira livremente a bola em graus.
void gcgFRUSTUM::rotateOrbitEuler(double centroX, double centroY, double centroZ, double pitch, double yaw, double roll) { // Gira livremente a bola em graus.
  VECTOR4d rot, quat;
  VECTOR3d r = {x - centroX, y - centroY, z - centroZ};

  gcgEULERTOQUATERNION(rot, pitch, yaw, roll);

  gcgMULTQUATERNION(rot, rot, orientation);  // Alinha c/ sistema da camera+rotacao
  gcgCONJUGATEQUATERNION(quat, orientation);
  gcgMULTQUATERNION(rot, quat, rot);           // Desalinha o sistema da camera

  gcgAPPLYQUATERNIONVECTOR3(r, rot, r);       // Rotaciona o vetor r
  setPosition(r[0] + centroX, r[1] + centroY, r[2] + centroZ);
}



// Rotacao do sistema de observacao em torno de um eixo que passa por O e tem direcao dir
void gcgFRUSTUM::rotateAxis(double ang, double ox, double oy, double oz, double dirX, double dirY, double dirZ) {
  VECTOR3d eixo, r = {x - ox, y - oy, z - oz};
  VECTOR4d quat;

  ang = DEG2RAD(ang);

  gcgSETVECTOR3(eixo, dirX, dirY, dirZ);
  gcgNORMALIZEVECTOR3(eixo, eixo);
  gcgAXISTOQUATERNION(quat, ang, eixo);

  gcgAPPLYQUATERNIONVECTOR3(r, quat, r);       // Rotaciona o vetor r
  setPosition(r[0] + ox, r[1] + oy, r[2] + oz);
}



// Configura a projecao perspectiva
// Calcula a matriz de projecao perspectiva diretamente e recalcula os dados do FRUSTUM.
void gcgFRUSTUM::setPerspective(float fovh, float aspect, float nearplane, float farplane) {
  // Calcula os outros planos
  double top    = nearplane *  tan(DEG2RAD(fovh * 0.5));
  double bottom = -top;
  double right  = aspect * top;
  double left   = -right;

  // Monta a matriz de perspectiva OPENGL
  proj_matrix[0] = (2.0 * nearplane) / (right - left);
  proj_matrix[1] = 0;
  proj_matrix[2] = 0;
  proj_matrix[3] = 0;

  proj_matrix[4] = 0;
  proj_matrix[5] = (2.0 * nearplane) / (top - bottom);
  proj_matrix[6] = 0;
  proj_matrix[7] = 0;

  proj_matrix[8] = (right + left) / (right - left);
  proj_matrix[9] = (top + bottom) / (top - bottom);
  proj_matrix[10]= -(farplane + nearplane) / (farplane - nearplane);
  proj_matrix[11]= -1.0;

  proj_matrix[12] = 0;
  proj_matrix[13] = 0;
  proj_matrix[14] = -(2.0 * farplane * nearplane) / (farplane - nearplane);
  proj_matrix[15] = 0;

  computeParameters();
}


// Configura a projecao ortográfica
// Calcula a matriz de projecao ortográfica diretamente e recalcula os dados do FRUSTUM.
void gcgFRUSTUM::setOrthographic(float left, float right, float top, float bottom, float nearplane, float farplane) {
  // Monta a matriz ortográfica OPENGL
  proj_matrix[0] = 2.0 / (right - left);
  proj_matrix[1] = 0;
  proj_matrix[2] = 0;
  proj_matrix[3] = 0;

  proj_matrix[4] = 0;
  proj_matrix[5] = 2.0 / (top - bottom);
  proj_matrix[6] = 0;
  proj_matrix[7] = 0;

  proj_matrix[8] = 0;
  proj_matrix[9] = 0;
  proj_matrix[10]= -2.0 / (farplane - nearplane);
  proj_matrix[11]= 0;

  proj_matrix[12] = -(right + left) / (right - left);
  proj_matrix[13] = -(top + bottom) / (top - bottom);
  proj_matrix[14] = -(farplane + nearplane) / (farplane - nearplane);
  proj_matrix[15] = 1;

  computeParameters();
}


////////////////////////////////////////////////////////////////////////////////////
// Determina se um ponto esta´ inserido no frustum.
////////////////////////////////////////////////////////////////////////////////////
int gcgFRUSTUM::isPointVisible(float x, float y, float z ) {
	//   Na equacao de um plano (A*x + B*y + C*z + D = 0), [A B C] e´ a normal do plano
	// e D e´ a distancia do plano `a origem. Se um ponto (x y z) esta´ sobre o plano,
	// a equacao e´ verdadeira (=0). Se o resultado e´ negativo, o ponto esta´ atras do
	// plano. Se e´ positivo, esta na frente do plano.
	//   Um ponto esta dentro do frustum, se ele esta´ na frente (eq >= 0) de todos seus
	// planos.

	// Testa se o ponto esta´ em frente ao plano anterior
	if(nearA * x + nearB * y + nearC * z < -nearD) return GCG_FRUSTUM_OUTSIDE;

	// Testa se o ponto esta´ em frente ao plano posterior
	if(farA * x + farB * y + farC * z  < -farD) return GCG_FRUSTUM_OUTSIDE;

	// Testa se o ponto esta´ em frente ao plano direito
	if(rightA * x + rightB * y + rightC * z < -rightD) return GCG_FRUSTUM_OUTSIDE;

	// Testa se o ponto esta´ em frente ao plano esquerdo
	if(leftA * x + leftB * y + leftC * z < -leftD) return GCG_FRUSTUM_OUTSIDE;

	// Testa se o ponto esta´ em frente ao plano inferior
	if(bottomA * x + bottomB * y + bottomC * z < -bottomD) return GCG_FRUSTUM_OUTSIDE;

	// Testa se o ponto esta´ em frente ao plano superior.
	if(topA * x + topB * y + topC * z < -topD) return GCG_FRUSTUM_OUTSIDE;

	// O ponto esta atras de todos os planos
	return GCG_FRUSTUM_INSIDE;
}


////////////////////////////////////////////////////////////////////////////////////
// Determina a visibilidade de uma esfera no frustum (TOTAL, PARCIAL ou NAOVISIVEL)
////////////////////////////////////////////////////////////////////////////////////
int gcgFRUSTUM::isSphereVisible(float x, float y, float z, float raio) {
	// O centro da esfera pode estar fora do frustum mas a esfera nao. Assim.
	// deve-se levar o raio em consideracao. Assim, se uma equacao produziu 3, o centro
	// esta´ a uma distancia -3 fora do frustum. Mas se o raio for 5, a esfera ainda
	// esta´ dentro do frustum. Uma esfera esta fora do plano se sua distancia
	// e´ maior que o raio.
	int	result = GCG_FRUSTUM_INSIDE;

	// Testa a esfera em relacao ao plano proximo
	float distancia = nearA * x + nearB * y + nearC * z + nearD;
	if(distancia < -raio) return GCG_FRUSTUM_OUTSIDE;
	else if(distancia <= raio) result = GCG_FRUSTUM_PARTIAL;

	// Testa a esfera em relacao ao plano direito
	distancia = rightA * x + rightB * y + rightC * z + rightD;
	if(distancia < -raio) return GCG_FRUSTUM_OUTSIDE;
	else if(distancia <= raio) result = GCG_FRUSTUM_PARTIAL;

	// Testa a esfera em relacao ao plano esquerdo
	distancia = leftA * x + leftB * y + leftC * z + leftD;
	if(distancia < -raio) return GCG_FRUSTUM_OUTSIDE;
	else if(distancia <= raio) result = GCG_FRUSTUM_PARTIAL;

	// Testa a esfera em relacao ao plano inferior
	distancia = bottomA * x + bottomB * y + bottomC * z + bottomD;
	if(distancia < -raio) return GCG_FRUSTUM_OUTSIDE;
	else if(distancia <= raio) result = GCG_FRUSTUM_PARTIAL;

	// Testa a esfera em relacao ao plano superior
	distancia = topA * x + topB * y + topC * z + topD;
	if(distancia < -raio) return GCG_FRUSTUM_OUTSIDE;
	else if(distancia <= raio) result = GCG_FRUSTUM_PARTIAL;

  // Testa a esfera em relacao ao plano posterior (Menor probabilidade)
  distancia = farA * x + farB * y + farC * z + farD;
	if(distancia < -raio) return GCG_FRUSTUM_OUTSIDE;
	else if(distancia <= raio) result = GCG_FRUSTUM_PARTIAL;


	// A esfera esta´ pelo menos parcialmente inserida no frustum
	return result;
}

////////////////////////////////////////////////////////////////////////////////////
// Determina a visibilidade de uma caixa alinhada com os eixos AABB.
////////////////////////////////////////////////////////////////////////////////////
int gcgFRUSTUM::isBoxVisible(float xmin, float ymin, float zmin, float xmax, float ymax, float zmax) {
  // Usa o método de vértices negativos e positivos
  register float   xp, yp, zp;   // Coordenadas do vertice positivo
  register float   xn, yn, zn;   // Coordenadas do vertice negativo
  float   distancia;

  ///////////////////////////////////////////////////////////////////////
  // Testa todos os vertices POSITIVOS (que devem estar dentro do plano)

  // Testa a caixa em relacao ao PLANO PROXIMO
  // Verifica a normal do plano e ajusta o vertice positivo (que deve estar dentro do plano)
  xp = (nearA >= 0) ? xmax : xmin;
  yp = (nearB >= 0) ? ymax : ymin;
  zp = (nearC >= 0) ? zmax : zmin;
  distancia = nearA * xp + nearB * yp + nearC * zp + nearD;
	if(distancia < 0) return GCG_FRUSTUM_OUTSIDE; // Neste caso está totalmente fora do plano

  // Testa a caixa em relacao ao PLANO DIREITO
  // Verifica a normal do plano e ajusta o vertice positivo (que deve estar dentro do plano)
  xp = (rightA >= 0) ? xmax : xmin;
  yp = (rightB >= 0) ? ymax : ymin;
  zp = (rightC >= 0) ? zmax : zmin;
  distancia = rightA * xp + rightB * yp + rightC * zp + rightD;
	if(distancia < 0) return GCG_FRUSTUM_OUTSIDE; // Neste caso está totalmente fora do plano

  // Testa a caixa em relacao ao PLANO ESQUERDO
  // Verifica a normal do plano e ajusta o vertice positivo (que deve estar dentro do plano)
  xp = (leftA >= 0) ? xmax : xmin;
  yp = (leftB >= 0) ? ymax : ymin;
  zp = (leftC >= 0) ? zmax : zmin;
  distancia = leftA * xp + leftB * yp + leftC * zp + leftD;
	if(distancia < 0) return GCG_FRUSTUM_OUTSIDE; // Neste caso está totalmente fora do plano

  // Testa a caixa em relacao ao PLANO INFERIOR
  // Verifica a normal do plano e ajusta o vertice positivo (que deve estar dentro do plano)
  xp = (bottomA >= 0) ? xmax : xmin;
  yp = (bottomB >= 0) ? ymax : ymin;
  zp = (bottomC >= 0) ? zmax : zmin;
  distancia = bottomA * xp + bottomB * yp + bottomC * zp + bottomD;
	if(distancia < 0) return GCG_FRUSTUM_OUTSIDE; // Neste caso está totalmente fora do plano

  // Testa a caixa em relacao ao PLANO SUPERIOR
  // Verifica a normal do plano e ajusta o vertice positivo (que deve estar dentro do plano)
  xp = (topA >= 0) ? xmax : xmin;
  yp = (topB >= 0) ? ymax : ymin;
  zp = (topC >= 0) ? zmax : zmin;
  distancia = topA * xp + topB * yp + topC * zp + topD;
	if(distancia < 0) return GCG_FRUSTUM_OUTSIDE; // Neste caso está totalmente fora do plano


  // Testa a caixa em relacao ao PLANO DISTANTE
  // Verifica a normal do plano e ajusta o vertice positivo (que deve estar dentro do plano)
  xp = (farA >= 0) ? xmax : xmin;
  yp = (farB >= 0) ? ymax : ymin;
  zp = (farC >= 0) ? zmax : zmin;
  distancia = farA * xp + farB * yp + farC * zp + farD;
	if(distancia < 0) return GCG_FRUSTUM_OUTSIDE; // Neste caso está totalmente fora do plano


  // Neste ponto sabemos que a caixa esta parcialmente ou completamento dentro do FRUSTUM

  ///////////////////////////////////////////////////////////////////////
  // Testa todos os vertices NEGATIVOS (que devem estar fora do plano)

  // Testa a caixa em relacao ao PLANO PROXIMO
  // Verifica a normal do plano e ajusta o vertice negativo (que deve estar fora do plano)
  xn = (nearA >= 0) ? xmin : xmax;
  yn = (nearB >= 0) ? ymin : ymax;
  zn = (nearC >= 0) ? zmin : zmax;
  distancia = nearA * xn + nearB * yn + nearC * zn + nearD;
	if(distancia < 0) return GCG_FRUSTUM_PARTIAL; // Neste caso sabemos que é ao menos PARCIAL

  // Testa a caixa em relacao ao PLANO DIREITO
  // Verifica a normal do plano e ajusta o vertice negativo (que deve estar fora do plano)
  xn = (rightA >= 0) ? xmin : xmax;
  yn = (rightB >= 0) ? ymin : ymax;
  zn = (rightC >= 0) ? zmin : zmax;
  distancia = rightA * xn + rightB * yn + rightC * zn + rightD;
	if(distancia < 0) return GCG_FRUSTUM_PARTIAL; // Neste caso sabemos que é ao menos PARCIAL

  // Testa a caixa em relacao ao PLANO ESQUERDO
  // Verifica a normal do plano e ajusta o vertice negativo (que deve estar fora do plano)
  xn = (leftA >= 0) ? xmin : xmax;
  yn = (leftB >= 0) ? ymin : ymax;
  zn = (leftC >= 0) ? zmin : zmax;
  distancia = leftA * xn + leftB * yn + leftC * zn + leftD;
	if(distancia < 0) return GCG_FRUSTUM_PARTIAL; // Neste caso sabemos que é ao menos PARCIAL

  // Testa a caixa em relacao ao PLANO INFERIOR
  // Verifica a normal do plano e ajusta o vertice negativo (que deve estar fora do plano)
  xn = (bottomA >= 0) ? xmin : xmax;
  yn = (bottomB >= 0) ? ymin : ymax;
  zn = (bottomC >= 0) ? zmin : zmax;
  distancia = bottomA * xn + bottomB * yn + bottomC * zn + bottomD;
	if(distancia < 0) return GCG_FRUSTUM_PARTIAL; // Neste caso sabemos que é ao menos PARCIAL

  // Testa a caixa em relacao ao PLANO SUPERIOR
  // Verifica a normal do plano e ajusta o vertice negativo (que deve estar fora do plano)
  xn = (topA >= 0) ? xmin : xmax;
  yn = (topB >= 0) ? ymin : ymax;
  zn = (topC >= 0) ? zmin : zmax;
  distancia = topA * xn + topB * yn + topC * zn + topD;
	if(distancia < 0) return GCG_FRUSTUM_PARTIAL; // Neste caso sabemos que é ao menos PARCIAL

  // Testa a caixa em relacao ao PLANO DISTANTE
  // Verifica a normal do plano e ajusta o vertice negativo (que deve estar fora do plano)
  xn = (farA >= 0) ? xmin : xmax;
  yn = (farB >= 0) ? ymin : ymax;
  zn = (farC >= 0) ? zmin : zmax;
  distancia = farA * xn + farB * yn + farC * zn + farD;
	if(distancia < 0) return GCG_FRUSTUM_PARTIAL; // Neste caso sabemos que é ao menos PARCIAL

	// A caixa esta´ no frustum totalmente
	return GCG_FRUSTUM_INSIDE;
}

// Exporta as transformacoes para o OpenGL
void gcgFRUSTUM::exportOpenGL() {
  glMatrixMode(GL_PROJECTION);

//  glLoadMatrixf(proj_matrix);
//  glMatrixMode(GL_MODELVIEW);
//  glLoadMatrixf(view_matrix);

  glLoadMatrixd(proj_matrix);
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixd(view_matrix);
}


// Calcula os vetores de visualizacao a partir do quaternion
// Calcula os planos do frustum
void gcgFRUSTUM::computeParameters() {
  MATRIX4d   m, rot, trans;
  MATRIX4d   transM;	// Guarda a transformacao completa TRANSPOSTA(projecao * modelovisao)
	register double A, B, C, D, norma;


  // Extrai a orientacao da camera do quaternion
  gcgNORMALIZEQUATERNION(orientation, orientation);    // to avoid round errors
  gcgQUATERNIONTOMATRIX4(m, orientation); // Padrao OPENGL

  gcgSETVECTOR3(right, (float) m[0], (float) m[4], (float) m[8]);
  gcgSETVECTOR3(up,    (float) m[1], (float) m[5], (float) m[9]);
  gcgSETVECTOR3(view,  (float) m[2], (float) m[6], (float) m[10]);

  // Trying to minimize error propagation!
  gcgNORMALIZEVECTOR3(view,  view);
  gcgNORMALIZEVECTOR3(up,    up);
  gcgNORMALIZEVECTOR3(right, right);
  m[0] = right[0]; m[4] = right[1]; m[8]  = right[2];
  m[1] = up[0];    m[5] = up[1];    m[9]  = up[2];
  m[2] = view[0];  m[6] = view[1];  m[10] = view[2];

  gcgTRANSPOSEMATRIX4(rot, m);

  // Monta a matriz de translacao (TRANSPOSTA DO PADRAO OPENGL)
  gcgIDENTITYMATRIX4(trans);
  trans[ 3] = -x; trans[ 7] = -y; trans[11] = -z;

  // Calcula a matriz de visualizacao
  gcgMULTMATRIX4(m, rot, trans);
  gcgTRANSPOSEMATRIX4(view_matrix, m); // Padrao OpenGL

	// Multiplica as transformacoes de projecao e modelo_visao que estao transpostas
	// (padrao OPENGL). De acordo com a propriedade de matrizes (A . B)t = Bt . At,
	// a transposta da transformacao M = (projecao . modelo_visao) e´ obtido pela
	// multiplicacao de TRANSPOSTA(M) = (TRANSPOSTA(modelo_visao) . TRANSPOSTA(projecao)).

	gcgMULTMATRIX4(transM, view_matrix, proj_matrix);

	//////////////////////////////////////////////////////////////////////////
	//	  Por default, uma camera o OPENGL e´ direcionada para o z negativo com
	// o lado superior da cabeca voltada para o eixo y positivo. Assim, inicialmente
	// o vetores normais (coordenadas homogeneas) aos planos do volume sao:
	//		- plano superior  = [ 0 -1  0  1 ]
	//		- plano inferior  = [ 0  1  0  1 ]
	//		- plano posterior = [ 0  0  1  1 ]
	//		- plano anterior  = [ 0  0 -1  1 ]
	//		- plano direito   = [-1  0  0  1 ]
	//		- plano esquerdo  = [ 1  0  0  1 ]
	//
	//	  Esses vetores estao invertidos para que a tranformacao de projecao calcule
	// corretamente os respectivos vetores normais dos planos que apontam para dentro
	// do frustum;
	//    Os planos do frustum sao obtidos pela tranformacao dos vetores normais
	// aos planos. Com isso obtem-se a equacao dos planos Ax + By + Xz + D = 0, onde
	// [A B C] e´ o vetor normal ao plano e D e´ a sua distancia `a origem.

	///////////////////////////////////////////////////////////////
	// Obtem a equacao do plano superior do volume = M * [0 -1 0 1]
	///////////////////////////////////////////////////////////////
	A = transM[ 3] - transM[ 1];
	B = transM[ 7] - transM[ 5];
	C = transM[11] - transM[ 9];
	D = transM[15] - transM[13];

	// Normaliza o plano superior dividindo a sua equacao pela norma do vetor [A B C]
	norma = sqrt(A * A + B * B + C * C);
	topA = (float) (A / norma);		topB = (float) (B / norma);
	topC = (float) (C / norma);		topD = (float) (D / norma);

	///////////////////////////////////////////////////////////////
	// Obtem a equacao do plano inferior do volume = M * [0 1 0 1]
	///////////////////////////////////////////////////////////////
	A = transM[ 3] + transM[ 1];
	B = transM[ 7] + transM[ 5];
	C = transM[11] + transM[ 9];
	D = transM[15] + transM[13];

	// Normaliza o plano inferior dividindo a sua equacao pela norma do vetor [A B C]
	norma = sqrt(A * A + B * B + C * C);
	bottomA = (float) (A / norma);		bottomB = (float) (B / norma);
	bottomC = (float) (C / norma);		bottomD = (float) (D / norma);

	///////////////////////////////////////////////////////////////
	// Obtem a equacao do plano posterior do volume = M * [0 0 1 1]
	///////////////////////////////////////////////////////////////
	A = transM[ 3] + transM[ 2];
	B = transM[ 7] + transM[ 6];
	C = transM[11] + transM[10];
	D = transM[15] + transM[14];

	// Normaliza o plano posterior dividindo a sua equacao pela norma do vetor [A B C]
	norma = sqrt(A * A + B * B + C * C);
	farA = (float) (A / norma);	farB = (float) (B / norma);
	farC = (float) (C / norma);	farD = (float) (D / norma);

	///////////////////////////////////////////////////////////////
	// Obtem a equacao do plano anterior do volume = M * [0 0 -1 1]
	///////////////////////////////////////////////////////////////
	A = transM[ 3] - transM[ 2];
	B = transM[ 7] - transM[ 6];
	C = transM[11] - transM[10];
	D = transM[15] - transM[14];

	// Normaliza o plano anterior dividindo a sua equacao pela norma do vetor [A B C]
	norma = sqrt(A * A + B * B + C * C);
	nearA = (float) (A / norma);
  nearB = (float) (B / norma);
	nearC = (float) (C / norma);
  nearD = (float) (D / norma);


	///////////////////////////////////////////////////////////////
	// Obtem a equacao do plano direito do volume = M * [-1 0 0 1]
	///////////////////////////////////////////////////////////////
	A = transM[ 3] - transM[ 0];
	B = transM[ 7] - transM[ 4];
	C = transM[11] - transM[ 8];
	D = transM[15] - transM[12];

	// Normaliza o plano direito dividindo a sua equacao pela norma do vetor [A B C]
	norma = sqrt(A * A + B * B + C * C);
	rightA = (float) (A / norma);		rightB = (float) (B / norma);
	rightC = (float) (C / norma);		rightD = (float) (D / norma);

	///////////////////////////////////////////////////////////////
	// Obtem a equacao do plano esquerdo do volume = M * [1 0 0 1]
	///////////////////////////////////////////////////////////////
	A = transM[ 3] + transM[ 0];
	B = transM[ 7] + transM[ 4];
	C = transM[11] + transM[ 8];
	D = transM[15] + transM[12];

	// Normaliza o plano esquerdo dividindo a sua equacao pela norma do vetor [A B C]
	norma = sqrt(A * A + B * B + C * C);
	leftA = (float) (A / norma);		leftB = (float) (B / norma);
	leftC = (float) (C / norma);		leftD = (float) (D / norma);
}

// Gets the view matrix
void gcgFRUSTUM::getViewMatrix(MATRIX4 matrot) {
  gcgCOPYMATRIX4(matrot, view_matrix);
}

// Gets the view matrix
void gcgFRUSTUM::getViewMatrix(MATRIX4d matrot) {
  gcgCOPYMATRIX4(matrot, view_matrix);
}

// Gets the projection matrix
void gcgFRUSTUM::getProjectionMatrix(MATRIX4 matproj) {
  gcgCOPYMATRIX4(matproj, proj_matrix);
}

// Gets the projection matrix
void gcgFRUSTUM::getProjectionMatrix(MATRIX4d matproj) {
  gcgCOPYMATRIX4(matproj, proj_matrix);
}
