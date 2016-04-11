/*************************************************************************************
    GCG - GROUP FOR COMPUTER GRAPHICS, IMAGE AND VISION
        Universidade Federal de Juiz de Fora
        Instituto de Ciências Exatas
        Departamento de Ciência da Computação

   MATRIXCROSS.CPP: cross type functions.

   Marcelo Bernardes Vieira
**************************************************************************************/

#if defined(NUMBERTYPE) && defined(CROSSTYPE)

bool gcgMATRIX<NUMBERTYPE>::createSimilar(gcgMATRIX<CROSSTYPE> *source) {
  // Check source.
  if(source == NULL) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_ALGEBRA, GCG_BADPARAMETERS), "createSimilar(): source is NULL. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
  }
  if(source->data == NULL || source->rows == 0 || source->columns == 0) {
    gcgReport(GCG_REPORT_CODE(GCG_WARNING, GCG_DOMAIN_ALGEBRA, GCG_INVALIDOBJECT), "createSimilar(): invalid source dimensions. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
    return false;
  }

  // Structural incompatibility
  if(this->data == NULL || source->rows != this->rows || source->columns != this->columns) {
    this->destroyMatrix(); // Release previous resources

    // Duplicate memory for signal data
    this->data = new NUMBERTYPE [source->rows * source->columns];
    if(this->data == NULL) {
      gcgReport(GCG_REPORT_CODE(GCG_ERROR, GCG_DOMAIN_MEMORY, GCG_ALLOCATIONERROR), "createSimilar(): matrix data allocation error. (%s:%d)", basename((char*)(char*)__FILE__), __LINE__);
      return false;
    }

    // Copy main parameters
    this->rows = source->rows;
    this->columns = source->columns;
    this->isstaticdata = false;
  }

	// Ok, it's a similar matrix
	return true;
}


#endif // #defined(NUMBERTYPE) && defined(CROSSTYPE)
