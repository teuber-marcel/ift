%module pyift
%{
  #include "ift.h"
  #include "iftImage.h"
%}

%{

PyObject* get_superpixel_centers(char *image_path, int vol_threshold){
	float spatial_radius = 1.5;
	
	iftImage *image = iftReadImageP6(image_path);
	
	iftAdjRel* adj_relation = iftCircular(spatial_radius);
	iftImage* basins = iftImageBasins(image,adj_relation);
    iftImage* marker = iftVolumeClose(basins,vol_threshold);
	
    iftImage* label_image = iftWaterGray(basins,marker,adj_relation);
	
	PyObject *coordinates =  PyList_New(0);
	
	iftLabeledSet* geo_centers =  iftGeometricCenters(label_image);
	iftLabeledSet* g = geo_centers;
	while(g != NULL){
		int p = g->elem;
		
		int x = iftGetXCoord(image, p);
		int y = iftGetYCoord(image, p);
		
		PyObject *xy = PyTuple_New(2);
		PyTuple_SetItem(xy, 0, PyInt_FromLong(x));
		PyTuple_SetItem(xy, 1, PyInt_FromLong(y));
		
		PyList_Append(coordinates, xy);
		
		g = g->next;
	}
	
	iftDestroyAdjRel(&adj_relation);
	iftDestroyImage(&image);
	iftDestroyImage(&basins);
	iftDestroyImage(&marker);
	iftDestroyImage(&label_image);
	iftDestroyLabeledSet(&geo_centers);
	
	return coordinates;
}

PyObject* get_windows_from_label_image(iftImage* label_image){
	//List of windows (tuples): (xmax,ymax,xmin,ymin)
	int nregions = iftMaximumValue(label_image);
	
	PyObject *windows = PyList_New(nregions);
	
	int *xmaxs = iftAllocIntArray(nregions);
	int *ymaxs = iftAllocIntArray(nregions);
	
	int *xmins = iftAllocIntArray(nregions);
	int *ymins = iftAllocIntArray(nregions);
	int r;
	for(r = 0; r < nregions; r++){
		xmins[r] = label_image->xsize -1;
		ymins[r] = label_image->ysize -1;
	}
	
	int p;
	for(p = 0; p < label_image->n; p++){
		int region = label_image->val[p] - 1;
		
		iftVoxel v = iftGetVoxelCoord(label_image,p);
		
		if(v.x < xmins[region])
			xmins[region] = v.x;
		if(v.x > xmaxs[region])
			xmaxs[region] = v.x;
		
		if(v.y < ymins[region])
			ymins[region] = v.y;
		if(v.y > ymaxs[region])
			ymaxs[region] = v.y;
	}
	
	for(r = 0; r < nregions; r++){
		PyObject *rect = PyTuple_New(4);
		
		PyTuple_SetItem(rect, 0, PyInt_FromLong(xmaxs[r]));
		PyTuple_SetItem(rect, 1, PyInt_FromLong(ymaxs[r]));
		PyTuple_SetItem(rect, 2, PyInt_FromLong(xmins[r]));
		PyTuple_SetItem(rect, 3, PyInt_FromLong(ymins[r]));
		
		PyList_SetItem(windows, r, rect);
	}
	
	free(xmaxs);
	free(ymaxs);
	free(xmins);
	free(ymins);
	
	return windows;
}

PyObject* get_superpixel_windows(char *image_path, int vol_threshold){
	float spatial_radius = 1.5;
	
	iftImage *image = iftReadImageP6(image_path);
	
	iftAdjRel* adj_relation = iftCircular(spatial_radius);
	iftImage* basins = iftImageBasins(image,adj_relation);
    iftImage* marker = iftVolumeClose(basins,vol_threshold);
	
    iftImage* label_image = iftWaterGray(basins,marker,adj_relation);

	PyObject *windows = get_windows_from_label_image(label_image);
	
	iftDestroyImage(&image);
	iftDestroyAdjRel(&adj_relation);
	iftDestroyImage(&basins);
	iftDestroyImage(&marker);
	iftDestroyImage(&label_image);
	
	return windows;
}


PyObject* get_hybrid_superpixel_windows_basic(char* image_path, int volume_threshold, PyObject* alpha){
		float spatial_radius = 1.5;
	
	iftImage *image = iftReadImageP6(image_path);
	
	iftAdjRel* adj_relation = iftCircular(spatial_radius);
	iftImage* basins = iftImageBasins(image,adj_relation);
    iftImage* marker = iftVolumeClose(basins,volume_threshold);
	
    iftImage* label_image = iftWaterGray(basins,marker,adj_relation);

	iftDataSet* dataset = iftSupervoxelsToMeanStdSizeDataset(image, label_image);
	iftAdjRel *adj2 = iftCircular(1.0);
	
	float alpha_array[10];
	int i;
	for(i = 0; i < 10; i++)
    	alpha_array[i] = (float)PyFloat_AsDouble(PyList_GetItem(alpha, i));

	iftRegionHierarchy *rh = iftCreateRegionHierarchy(label_image, dataset, adj2, iftDistMeanStdSizeSupervoxel, iftMergeMeanStdSizeSupervoxel, alpha_array);

    iftRegionHierarchyNode **nodes = iftGetRegionHierarchyNodes(rh);
    int nregions = rh->nleaves + rh->root->merging_time;
	
	//List of windows (tuples): (xmax,ymax,xmin,ymin)
	PyObject *windows = PyList_New(nregions);
	
	int r;
	for(r = 0; r < nregions; r++){
		PyObject *rect = PyTuple_New(4);
		
		PyTuple_SetItem(rect, 0, PyInt_FromLong(nodes[r]->xmax));
		PyTuple_SetItem(rect, 1, PyInt_FromLong(nodes[r]->ymax));
		PyTuple_SetItem(rect, 2, PyInt_FromLong(nodes[r]->xmin));
		PyTuple_SetItem(rect, 3, PyInt_FromLong(nodes[r]->ymin));
		
		PyList_SetItem(windows, r, rect);
	}

    iftDestroyImage(&image);
    iftDestroyAdjRel(&adj_relation);
    iftDestroyImage(&basins);
    iftDestroyImage(&marker);
    iftDestroyImage(&label_image);
    iftDestroyDataSet(&dataset);
    iftDestroyAdjRel(&adj2);
    free(nodes);
    iftDestroyRegionHierarchy(&rh);
	
	return windows;
}

PyObject* get_hybrid_superpixel_windows_full(char* image_path, int volume_threshold, PyObject* alpha, int nbins){
	float spatial_radius = 1.5;
	
	iftImage *image = iftReadImageP6(image_path);
	
	iftAdjRel* adj_relation = iftCircular(spatial_radius);
	iftImage* basins = iftImageBasins(image,adj_relation);
    iftImage* marker = iftVolumeClose(basins,volume_threshold);
	
    iftImage* label_image = iftWaterGray(basins,marker,adj_relation);

	iftDataSet* dataset = iftSupervoxelsToSelectiveSearchDataset(image, label_image, nbins);
	iftAdjRel *adj2 = iftCircular(1.0);
	
	float alpha_array[10];
	int i;
	for(i = 0; i < 10; i++)
    	alpha_array[i] = (float)PyFloat_AsDouble(PyList_GetItem(alpha, i));

	iftRegionHierarchy *rh = iftCreateRegionHierarchy(label_image, dataset, adj2, iftDistSelectiveSearchSupervoxel, iftMergeSelectiveSearchSupervoxel, alpha_array);

    iftRegionHierarchyNode **nodes = iftGetRegionHierarchyNodes(rh);
    int nregions = rh->nleaves + rh->root->merging_time;
	
	//List of windows (tuples): (xmax,ymax,xmin,ymin)
	PyObject *windows = PyList_New(nregions);
	
	int r;
	for(r = 0; r < nregions; r++){
		PyObject *rect = PyTuple_New(4);
		
		PyTuple_SetItem(rect, 0, PyInt_FromLong(nodes[r]->xmax));
		PyTuple_SetItem(rect, 1, PyInt_FromLong(nodes[r]->ymax));
		PyTuple_SetItem(rect, 2, PyInt_FromLong(nodes[r]->xmin));
		PyTuple_SetItem(rect, 3, PyInt_FromLong(nodes[r]->ymin));
		
		PyList_SetItem(windows, r, rect);
	}

    iftDestroyImage(&image);
    iftDestroyAdjRel(&adj_relation);
    iftDestroyImage(&basins);
    iftDestroyImage(&marker);
    iftDestroyImage(&label_image);
    iftDestroyDataSet(&dataset);
    iftDestroyAdjRel(&adj2);
    free(nodes);
    iftDestroyRegionHierarchy(&rh);
	
	return windows;
}

PyObject* iftPyExtractDeepFeatures(char *image_path, iftConvNetwork *net){
	iftImage *image;
	if(net->input_nbands == 1)
		image = iftReadImageP5(image_path);
	else
		image = iftReadImageP6(image_path);
		
	iftMImage* mimg  = iftImageToMImage(image,RGB_CSPACE);
	iftMImage* result = iftApplyConvNetwork(mimg,net);
	iftFeatures *feats = iftMImageToFeatures(result);
	
	PyObject *pylist =  PyList_New(feats->n);
	
	int i;
	for(i = 0; i < feats->n; i++)
		PyList_SetItem(pylist, i, PyFloat_FromDouble(feats->val[i]) );
		
	free(feats->val);
	free(feats);
	
	iftDestroyImage(&image);
	iftDestroyMImage(&mimg);
	iftDestroyMImage(&result);
	
	return pylist;
}

PyObject* iftPyExtractBICFeatures(char *image_path, int bins_per_band){
	iftImage *image = iftReadImageP6(image_path);
	
	iftFeatures *feats = iftExtractBIC(image, bins_per_band);
	
	PyObject *pylist =  PyList_New(feats->n);
	
	int i;
	for(i = 0; i < feats->n; i++)
		PyList_SetItem(pylist, i, PyFloat_FromDouble(feats->val[i]) );
		
	free(feats->val);
	free(feats);
	
	iftDestroyImage(&image);
	
	return pylist;
}

void iftPyDestroyConvNetwork(iftConvNetwork *net){
	iftDestroyConvNetwork(&net);
	free(net);
}

void              iftSaveConvNetwork(iftConvNetwork *convnet, char *filename){
	convnet->with_weights = 1;
	iftWriteConvNetwork(convnet,filename);
}

%}

iftConvNetwork   *iftReadConvNetwork(char *filename);
void              iftSaveConvNetwork(iftConvNetwork *convnet, char *filename);
void iftPyDestroyConvNetwork(iftConvNetwork *net);

PyObject* iftPyExtractDeepFeatures(char *image_path, iftConvNetwork *net);
PyObject* iftPyExtractBICFeatures(char *image_path, int bins_per_band);

PyObject* get_windows_from_label_image(iftImage* label_image); 

PyObject* get_superpixel_centers(char *image_path, int vol_threshold);
PyObject* get_superpixel_windows(char *image_path, int vol_threshold);
PyObject* get_hybrid_superpixel_windows_basic(char* image_path, int volume_threshold, PyObject* alpha);
PyObject* get_hybrid_superpixel_windows_full(char* image_path, int volume_threshold, PyObject* alpha, int nbins);
