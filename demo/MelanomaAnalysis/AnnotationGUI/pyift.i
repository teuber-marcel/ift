// pyift.i: declaration of ift structs and functions for wrapping.
// Read the comments carefully before modifying.

%module pyift
%{
  #include "ift.h"
%}

#ifndef PYIFT_DEBUG
//#define PYIFT_DEBUG
#endif

// Structs:
// Declare the structs as "typedef struct{...} name;", (not "typedef struct struct_name{...} name;")
// Otherwise, the correct destructor won't be called by swig.
// To extend a struct (define methods for its corresponding python object) use the directive %extend following the examples bellow.
// You should also override the destructor (~structName) to free dynamically allocated memory
// Note: among other things, you can't index an array inside a struct from python. You can work around this by defining struct methods, as bellow.
// Swig reference: http://www.swig.org/Doc1.3/Python.html.
// Python special method names (useful for overriding): http://docs.python.org/3.3/reference/datamodel.html
// Creating python objects: http://www.swig.org/Doc1.3/Python.html#Python_nn57

//Struct iftColor

typedef struct {
  int val[3];
} iftColor;

%extend iftColor{
	void __setitem__(int i, int value){
		if(i >= 0 && i < 3){
			($self)->val[i] = value;
		}
	}

	void set_values(PyObject *tuple){
		iftColor *color = ($self);

		color->val[0] = (int)PyInt_AsLong(PyTuple_GetItem(tuple,0));
		color->val[1] = (int)PyInt_AsLong(PyTuple_GetItem(tuple,1));
		color->val[2] = (int)PyInt_AsLong(PyTuple_GetItem(tuple,2));
	}

	PyObject* get_values(){
		iftColor *color = ($self);
		PyObject *tuple = PyTuple_New(3);

		PyTuple_SetItem(tuple,0,PyInt_FromLong(color->val[0]));
		PyTuple_SetItem(tuple,1,PyInt_FromLong(color->val[1]));
		PyTuple_SetItem(tuple,2,PyInt_FromLong(color->val[2]));

		return tuple;
	}
};

typedef struct{
  int    *val;
  ushort *Cb,*Cr;
  int xsize,ysize,zsize;
  float dx,dy,dz;
  int *tby, *tbz;        // speed-up voxel access tables
  int maxval, minval, n; // minimum and maximum values, and number of voxels
} iftImage;

//Struct iftImage

%extend iftImage{
	PyObject* __getitem__(int pixel){
		iftImage *image = ($self);
		PyObject *color = PyTuple_New(3);

		PyTuple_SetItem(color, 0, PyInt_FromLong(image->val[pixel]));

		if(iftIsColorImage(image)){
			PyTuple_SetItem(color, 1, PyInt_FromLong((long)image->Cb[pixel]));
			PyTuple_SetItem(color, 2, PyInt_FromLong((long)image->Cr[pixel]));
		}else{
			PyTuple_SetItem(color, 1, PyInt_FromLong(0));
			PyTuple_SetItem(color, 2, PyInt_FromLong(0));
		}

		return color;
	}

	void __setitem__(int pixel, PyObject* color){
		iftImage *image = ($self);

		image->val[pixel] = (int)PyInt_AsLong(PyTuple_GetItem(color,0));
		if(iftIsColorImage(image)){
			image->Cb[pixel] = (ushort)PyInt_AsLong(PyTuple_GetItem(color,1));
			image->Cr[pixel] = (ushort)PyInt_AsLong(PyTuple_GetItem(color,2));
		}
	}

	int get_voxel_index(iftVoxel* voxel){
		iftImage *s = ($self);
		iftVoxel v = *voxel;
		return iftGetVoxelIndex(s,v);
	}

	iftVoxel get_voxel(int p){
		iftVoxel v = iftGetVoxelCoord(($self),p);
		return v;
	}
	
	void add_const(int c){
		int p;
		for(p = 0; p < ($self)->n; p++)
			($self)->val[p]++;
	}
	
	~iftImage(){
		iftImage *ptr = ($self);
		iftDestroyImage(&ptr);

		#ifdef PYIFT_DEBUG
			printf("iftImage deleted\n");
		#endif
	}
}

//Struct iftAdjRel

typedef struct{
  int *dx,*dy,*dz; /* displacements to achieve the n adjacent voxels. */
  int  n; /* number of adjacent voxels. */
} iftAdjRel;

%extend iftAdjRel{
	PyObject* __getitem__(int i){
		PyObject *displacement = PyTuple_New(3);

		PyTuple_SetItem(displacement,0, PyInt_FromLong(($self)->dx[i]));
		PyTuple_SetItem(displacement,1, PyInt_FromLong(($self)->dy[i]));
		PyTuple_SetItem(displacement,2, PyInt_FromLong(($self)->dz[i]));

		return displacement;
	}

	~iftAdjRel(){
		iftAdjRel *ptr = ($self);
		iftDestroyAdjRel(&ptr);

		#ifdef PYIFT_DEBUG
			printf("iftAdjRel deleted\n");
		#endif
	}
}

//Struct iftDataSet

typedef struct {
  iftSample *sample;   // sample
  int        nfeats;   // number of features
  int        nsamples; // number of samples
  int        nlabels;  // number of clusters
  int        nclasses; // number of classes
  int        ntrainsamples; // number of training samples
  iftArcWeightFun iftArcWeight; // distance function
  float     *alpha;   // coefficients used for arc weight computation
  void      *ref_data; // it migh be an image, for voxel datasets, a text file with image filenames, for image datasets, a region graph, for supervoxel datasets, etc.
  iftFeatSpaceParam fsp; // parameters of the feature scape transformation
} iftDataSet;

%extend iftDataSet{
	void set_alpha(PyObject* list){
		iftDataSet *dataset = ($self);
		int n_features = PyList_Size(list);

		int i;
		for(i = 0; i < n_features; i++){
			dataset->alpha[i] = (float)PyFloat_AsDouble(PyList_GetItem(list,i));
		}

		#ifdef PYIFT_DEBUG
			printf("iftDataSet alpha: %f, %f, %f\n",dataset->alpha[0],dataset->alpha[1],dataset->alpha[2]);
		#endif
	}
	
	void set_sample_features(int sample, PyObject* list){
		iftDataSet *dataset = ($self);
		int nfeats = PyList_Size(list);
		
		int i;
		for(i = 0; i < nfeats; i++){
			dataset->sample[sample].feat[i] = (float)PyFloat_AsDouble(PyList_GetItem(list,i));
		}
	}

	int get_sample_label(int sample_index){
		return ($self)->sample[sample_index].label;
	}

	int get_sample_id(int sample_index){
		return ($self)->sample[sample_index].id;
	}

	void set_status_all(int status){
		iftSetStatus(($self),status);
	}

	void set_training_samples(iftSet* S){
		iftDataSet *Z = ($self);

		iftSet *s = S;
		while (s != NULL){
			int sample_id = s->elem;
			Z->sample[sample_id].status = TRAIN;
			Z->sample[sample_id].truelabel = Z->sample[sample_id].label;

			s = s->next;
		}
	}

	void set_class_all(int cl){
		iftDataSet* dataset = ($self);
		int i;
		for (i = 0; i < dataset->nsamples; i++)
			dataset->sample[i].truelabel = cl;
	}

	void set_class(int sample_id, int cl){
		iftDataSet* dataset = ($self);

		dataset->sample[sample_id].truelabel = cl;
	}

	~iftDataSet(){
		iftDataSet *ptr = ($self);
		iftDestroyDataSet(&ptr);

		#ifdef PYIFT_DEBUG
			printf("iftDataSet deleted\n");
		#endif
	}
}

//Struct iftLabeledSet

typedef struct {
  int elem;
  int label;
  struct ift_labeledset *next;
} iftLabeledSet;

%extend iftLabeledSet{
	~iftLabeledSet(){
		iftLabeledSet *ptr = ($self);
		iftDestroyLabeledSet(&ptr);

		#ifdef PYIFT_DEBUG
			printf("iftLabeledSet deleted\n");
		#endif
	}

	iftLabeledSet* __add__(iftLabeledSet* lset){
		iftLabeledSet *copy = iftCopyOrderedLabeledSet(($self));
		iftConcatLabeledSet(&copy,&lset);
		return copy;
	}


	iftLabeledSet* __sub__(iftLabeledSet* lset){
		iftLabeledSet *copy = iftCopyOrderedLabeledSet(($self));
		iftRemoveSubsetLabeledSet(&copy,&lset);
		return copy;
	}

	PyObject* to_dict(){
		iftLabeledSet* s = ($self);

		PyObject *dict = PyDict_New();
		while(s != NULL){
			PyDict_SetItem(dict, PyInt_FromLong(s->elem), PyInt_FromLong(s->label));
			s = s->next;
		}

		return dict;
	}

	void print_seeds(){
		iftLabeledSet *ptr = ($self);

		while(ptr != NULL){
			printf("%d,%d\n",ptr->elem,ptr->label);
			ptr = ptr->next;
		}
	}
}

//Struct iftCplGraph

typedef struct {
  iftCplNode *node;          // node
  float      *pathval;       // path value for each node
  int        *ordered_nodes; // list of nodes ordered by path value
  int         nnodes;        // number of nodes
  iftFHeap   *Q;             // priority queue
  iftDataSet *Z;             // Each graph node is a training sample in Z
} iftCplGraph;

%extend iftCplGraph{
	~iftCplGraph(){
		iftCplGraph *ptr = ($self);
		iftDestroyCplGraph(&ptr);

		#ifdef PYIFT_DEBUG
			printf("iftCplGraph deleted\n");
		#endif
	}
}

//Struct iftVoxel

typedef struct {
  int x,y,z;
} iftVoxel;

//Struct iftRegionGraphNode

typedef struct{
	iftSet* adjacent;
} iftRegionGraphNode;

//Struct iftRegionGraph

typedef struct{
	iftRegionGraphNode* node;
	iftDataSet *dataset;
	int nnodes;
	float *pathval;
	iftFHeap *heap;
} iftRegionGraph;

%extend iftRegionGraph{
	~iftRegionGraph(){
		iftRegionGraph *ptr = ($self);

		iftDestroyRegionGraph(&ptr);
		#ifdef PYIFT_DEBUG
			printf("iftRegionGraph deleted\n");
		#endif
	}
};

typedef struct {
  char *val;
  int   n, nbytes;
} iftBMap;

%extend iftBMap{
	~iftBMap(){
		iftBMap *ptr = ($self);

		iftDestroyBMap(&ptr);
		#ifdef PYIFT_DEBUG
			printf("iftBMap deleted\n");
		#endif
	}
};


//Struct iftKnnGraph
typedef struct {
  iftKnnNode *node;          // node
  float      *pathval;       // path value for each node
  int        *ordered_nodes; // list of nodes ordered by path value
  int         nnodes;        // number of nodes
  float      *maxarcw;       // maximum arc weight for each value of k
  int         kmax;          // maximum number of arcs
  int         k;             // best number of arcs
  iftFHeap   *Q;             // priority queue
  iftDataSet *Z;             // Each graph node is a training sample in Z
} iftKnnGraph;

%extend iftKnnGraph{
	iftSet* get_roots(){
		iftKnnGraph *graph = ($self);

		iftSet* roots = 0;

		int i;
		for(i = 0; i < graph->nnodes;i++){
			if( i == graph->node[i].root ){
				//Misleading: sample_id is actually the index of the sample in the array
				int sample_id = graph->node[i].sample;
				iftInsertSet(&roots,sample_id);
			}
		}

		return roots;
	}

	~iftKnnGraph(){
		iftKnnGraph *ptr = ($self);

		iftDestroyKnnGraph(&ptr);
		#ifdef PYIFT_DEBUG
			printf("iftKnnGraph deleted\n");
		#endif
	}
};

//Struct iftMST
typedef struct {
  iftMSTNode *node;     // node
  int         nnodes;   // number of nodes
  float       maxarcw;  // maximum arc weight in the tree
  float       minarcw;  // minimum arc weight in the tree
  iftDataSet *Z;        // Each graph node is a training sample in Z
} iftMST;

%extend iftMST{
	~iftMST(){
		iftMST *ptr = ($self);

		iftDestroyMST(&ptr);
		#ifdef PYIFT_DEBUG
			printf("iftMST deleted\n");
		#endif
	}
};

typedef struct {
  int elem;
  struct ift_set *next;
} iftSet;

%extend iftSet{
	PyObject* to_list(){
		iftSet* s = ($self);

		PyObject *list = PyList_New(iftSetSize(s));
		int i = 0;
		while(s != NULL){
			PyList_SetItem(list, i, PyInt_FromLong(s->elem));
			s = s->next;
			i++;
		}

		return list;
	}

	~iftSet(){
		iftSet *ptr = ($self);

		iftDestroySet(&ptr);
		#ifdef PYIFT_DEBUG
			printf("iftSet deleted\n");
		#endif
	}
};


typedef struct{
	struct iftRegionHierarchyNode *left;
	struct iftRegionHierarchyNode *right;

	int merging_time; //Indicates when the region was merged
	int region; //If this node is a leaf contains the region id. Otherwise contains NIL

	int xmax, ymax, zmax, xmin, ymin, zmin; //Bounding cube coordinates
} iftRegionHierarchyNode;

typedef struct{
	iftRegionHierarchyNode *root; //Root of the hierarchy (node with the highest merging_time)
	int nleaves; //Number of leaves in the tree (regions)

	iftSet** pixels_by_node; //List of pixels by (unmerged) region. Speeds up prunning.
	iftImage* label_image; //Label image representing the initial regions
} iftRegionHierarchy;

%extend iftRegionHierarchy{
	~iftRegionHierarchy(){
		iftRegionHierarchy *ptr = ($self);

		iftDestroyRegionHierarchy(&ptr);
		#ifdef PYIFT_DEBUG
			printf("iftDestroyRegionHierarchy deleted\n");
		#endif
	}
};


// Functions:
// If your function returns a dynamically allocated object that should be deleted when its respective python object
// is deleted, insert the directive %newobject functionName above it.

int iftXSize(iftImage *img);
int iftYSize(iftImage *img);
int iftZSize(iftImage *img);

%newobject iftCreateBMap;
iftBMap  *iftCreateBMap(int n);

void      iftDestroyBMap(iftBMap **b);

iftImage *iftNormalize(iftImage *img, float minval, float maxval);

%newobject iftCreateImage;
iftImage *iftCreateImage(int xsize,int ysize,int zsize);

%newobject iftNormalize;
iftImage *iftNormalize(iftImage *img, float minval, float maxval);

%newobject iftSupervoxelsToDataSet;
iftDataSet *iftSupervoxelsToDataSet(iftImage *img, iftImage *label);

%newobject iftSupervoxelsToMeanSizeDataSet;
iftDataSet* iftSupervoxelsToMeanSizeDataSet(iftImage* image, iftImage* label_image, char colorspace);

%newobject iftImageToDataSet;
iftDataSet *iftImageToDataSet(iftImage *img);

%newobject iftReadImageP5;
iftImage *iftReadImageP5(char *filename);
void      iftWriteImageP5(iftImage *img, char *filename);

%newobject iftReadImageP6;
iftImage *iftReadImageP6(char *filename);
void      iftWriteImageP6(iftImage *img, char *filename);

%newobject iftReadImageP2;
iftImage *iftReadImageP2(char *filename);
void      iftWriteImageP2(iftImage *img, char *filename);


%newobject iftWindowAndLevel;
iftImage *iftWindowAndLevel(iftImage *img, int width, int level, int maxval);

%newobject iftRotateImage2D;
iftImage *iftRotateImage2D(iftImage *img, float theta);

%newobject iftInterp2D;
iftImage *iftInterp2D(iftImage *img, float sx, float sy);

%newobject iftScaleImage2D;
iftImage *iftScaleImage2D(iftImage *img, float sx, float sy);

%newobject iftCircular;
iftAdjRel *iftCircular(float r);
%newobject iftImageBasins;
iftImage *iftImageBasins(iftImage *img, iftAdjRel *A);
%newobject iftVolumeClose;
iftImage *iftVolumeClose(iftImage *img, int volume_thres);
%newobject iftFastAreaClose;
iftImage *iftFastAreaClose(iftImage *img, int area_thres);
%newobject iftWaterGray;
iftImage  *iftWaterGray(iftImage *basins, iftImage *marker, iftAdjRel *A);

%newobject iftCopyImage;
iftImage  *iftCopyImage(iftImage *img);

%newobject iftRGBtoYCbCr;
iftColor iftRGBtoYCbCr(iftColor cin);

%newobject iftYCbCrtoRGB;
iftColor iftYCbCrtoRGB(iftColor cin);

%newobject iftCreateCplGraph;
iftCplGraph *iftCreateCplGraph(iftDataSet *Z);

void         iftSupTrain(iftCplGraph *graph);
int          iftClassify(iftCplGraph *graph, iftDataSet *Z);

void      iftDrawBorders(iftImage *img, iftImage *label, iftAdjRel *A, iftColor YCbCr, iftAdjRel *B);

void iftSetTrainingSupervoxelsFromSeeds(iftDataSet *dataset, iftImage *label_image, iftLabeledSet* seed);

char      iftValidVoxel(iftImage *img, iftVoxel v);

%newobject iftReadSeeds2D;
iftLabeledSet *iftReadSeeds2D(char *filename, iftImage *img);
void iftWriteSeeds2D(char* filename, iftLabeledSet* seed, iftImage* image);

%newobject iftSeedImageFromLabeledSet;
iftImage* iftSeedImageFromLabeledSet(iftLabeledSet* labeled_set, iftImage *image);
%newobject iftLabeledSetFromSeedImage;
iftLabeledSet *iftLabeledSetFromSeedImage(iftImage* seed_image);

%newobject iftRegionGraphFromLabelImage;
iftRegionGraph* iftRegionGraphFromLabelImage(iftImage* label_image, iftDataSet* dataset, iftAdjRel* adjacency);
void iftDestroyRegionGraph(iftRegionGraph** rg);
void iftSuperpixelClassification(iftRegionGraph *graph, iftImage *label_image, iftLabeledSet *pixel_seeds);
void iftSuperpixelClassificationGeodesic(iftRegionGraph *graph, iftImage *label_image, iftLabeledSet *pixel_seeds, float beta);

%newobject iftEnhanceEdges;
iftImage      *iftEnhanceEdges(iftImage *img, iftAdjRel *A, iftLabeledSet *seed, float alpha);

%newobject iftRelaxedWatershed;
iftImage  *iftRelaxedWatershed(iftImage *basins, iftAdjRel *A, iftLabeledSet *seed, int num_smooth_iterations, float smooth_factor);

%newobject iftWatershed;
iftImage *iftWatershed(iftImage *basins, iftAdjRel *A, iftLabeledSet *seed);

%newobject iftWatershedOnPixelDist;
iftImage  *iftWatershedOnPixelDist(iftDataSet *dataset, iftAdjRel *A,iftLabeledSet *seed);

%newobject iftGeodesicMarkersForSegmentation;
iftLabeledSet *iftGeodesicMarkersForSegmentation(iftImage *gt_image, iftImage *classification_image);

%newobject iftBorderMarkersForSuperpixelSegmentation;
iftLabeledSet* iftBorderMarkersForSuperpixelSegmentation(iftImage* label_image,iftImage* gt_image, iftDataSet* dataset);

%newobject iftBorderMarkersForPixelSegmentation;
iftLabeledSet *iftBorderMarkersForPixelSegmentation(iftImage *grad_image, iftImage *gt_image, float border_distance);

%newobject iftGetSeeds;
iftLabeledSet* iftGetSeeds(iftLabeledSet* lset, int nelem, int label);

%newobject iftGetMisclassifiedSeeds;
iftLabeledSet* iftGetMisclassifiedSeeds(iftLabeledSet* S, int nelem, int label, iftImage* gt_image, iftImage* cl_image);

%newobject iftCopyLabeledSet;
iftLabeledSet* iftCopyLabeledSet(iftLabeledSet *s);

%newobject iftCopyOrderedLabeledSet;
iftLabeledSet* iftCopyOrderedLabeledSet(iftLabeledSet *s);

%newobject iftGeodesicCenters;
iftLabeledSet* iftGeodesicCenters(iftImage* label_image);

int       iftMaximumValue(iftImage *img);
int       iftMinimumValue(iftImage *img);

void      iftSetImage(iftImage *img, int value);

typedef struct ift_error_classification {
	int tp;
	int fp;
	int fn;
	int tn;
} iftErrorClassification;

void        iftSetStatus(iftDataSet *Z,uchar status);

%newobject iftConvertColorSpace;
iftImage *iftConvertColorSpace(iftImage* image, char origin_cspace, char dest_cspace);

%newobject iftCreateRefinedLabelImage;
iftImage* iftCreateRefinedLabelImage(iftImage* image, iftLabeledSet* seed, float spatial_radius, int volume_threshold, int steps, float vol_ratio);

%newobject iftSupervoxelsToHistogramDataSet;
iftDataSet *iftSupervoxelsToHistogramDataSet(iftImage *img, iftImage *label, int nbins, int bpp);

%newobject iftSuperpixelLabelImageFromDataset;
iftImage* iftSuperpixelLabelImageFromDataset(iftImage *label_image, iftDataSet *dataset);

%newobject iftMask;
iftImage* iftMask(iftImage *image, iftImage* segmentation);

float iftPrecisionGivenErrors(iftErrorClassification* error);
float iftRecallGivenErrors(iftErrorClassification* error);
float iftFScoreGivenErrors(iftErrorClassification* error);
float iftAccuracyGivenErrors(iftErrorClassification* error);

iftErrorClassification iftSegmentationErrors(iftImage* gt_image, iftImage* cl_image);

int iftMarkersFromMisclassifiedSeeds(iftImage* seed_image, iftLabeledSet* all_seeds, iftBMap* used_seeds, int nseeds, int number_of_objects, iftImage* gt_image, iftImage* cl_image, int dist_border, int max_marker_radius, int min_marker_radius);

void iftWriteSeedsOnImage(iftImage* image, iftLabeledSet* seed);

int iftUnsupClassify(iftKnnGraph *graph, iftDataSet *Z);

//We need to wrap this because of the function pointer
%{
	iftKnnGraph *iftUnsupLearn3(iftDataSet *Z, float train_perc, float kmax_perc, int niterations){
		iftSelectUnsupTrainSamples(Z,train_perc);
		return iftUnsupLearn(Z,kmax_perc,iftNormalizedCut,niterations);
	}
%}

%newobject iftUnsupLearn3;
iftKnnGraph *iftUnsupLearn3(iftDataSet *Z, float train_perc, float kmax_perc, int niterations);

%newobject iftGraphBoundaryReduction;
iftDataSet *iftGraphBoundaryReduction(iftKnnGraph *graph, iftDataSet *Z);

%newobject iftCreateMST;
iftMST *iftCreateMST(iftDataSet *Z);

%newobject iftSelectSamplesForUserLabelingMST;
iftSet *iftSelectSamplesForUserLabelingMST(iftMST *mstree, int n);

void    iftNormalizeSampleWeightMST(iftMST *mstree);

void    iftSortNodesByWeightMST(iftMST *mstree, int order);

void        iftWriteOPFDataSet(iftDataSet *Z, char *filename);

float iftBoundaryRecall(iftImage *gt_image, iftImage *label_image, iftAdjRel *A);
float iftBoundaryRecallFromBImage(iftImage *border_image, iftImage *label_image, iftAdjRel *A);
float iftUnderSegmentation(iftImage *gt_image, iftImage *label_image, float tol_per);
float iftUnderSegmentationSLIC2(iftImage *gt_image, iftImage *label_image, float tol_per);
float iftUnderSegmentationTurbopixel(iftImage *gt_image, iftImage *label_image);
float iftUnderSegmentationMin(iftImage *gt_image, iftImage *label_image);

%newobject iftSupervoxelsToSelectiveSearchDataset;
iftDataSet* iftSupervoxelsToSelectiveSearchDataset(iftImage *image, iftImage* label_image, int bins_per_band, char colorspace);

%newobject iftFlattenRegionHierarchy;
iftImage* iftFlattenRegionHierarchy(iftRegionHierarchy* rh, int nregions);

%newobject iftEliminateRegionsByArea;
iftImage* iftEliminateRegionsByArea(iftDataSet* dataset, iftAdjRel *adj, int threshold);

%{
	iftRegionHierarchy* iftCreateRegionHierarchySelectiveSearch(iftDataSet *dataset, iftAdjRel *adj){
		return iftCreateRegionHierarchy(dataset, adj, iftMergeSelectiveSearchSupervoxel);
	}
	
	iftRegionHierarchy* iftCreateRegionHierarchyMeanSize(iftDataSet *dataset, iftAdjRel *adj){
		return iftCreateRegionHierarchy(dataset, adj, iftMergeMeanSizeSupervoxel);
	}
%}


%newobject iftCreateRegionHierarchySelectiveSearch;
iftRegionHierarchy* iftCreateRegionHierarchySelectiveSearch(iftDataSet *dataset, iftAdjRel *adj);

%newobject iftCreateRegionHierarchyMeanSize;
iftRegionHierarchy* iftCreateRegionHierarchyMeanSize(iftDataSet *dataset, iftAdjRel *adj);

// Struct iftBand
typedef struct {
  float *val;
} iftBand;

// Struct iftKernel
typedef struct {
  iftAdjRel *A;
  float     *weight;
} iftKernel;

%extend iftKernel{
	~iftKernel(){
		iftKernel *ptr = ($self);
		iftDestroyKernel(&ptr);

		#ifdef PYIFT_DEBUG
			printf("iftKernel deleted\n");
		#endif
	}
}

// Struct iftMKernel
typedef struct {
  iftAdjRel *A;
  iftBand   *weight;
  int        nbands;
} iftMKernel;

%extend iftMKernel{
	~iftMKernel(){
		iftMKernel *ptr = ($self);
		iftDestroyMKernel(&ptr);

		#ifdef PYIFT_DEBUG
			printf("iftMKernel deleted\n");
		#endif
	}
}

// Struct iftMMKernel
typedef struct {
  iftAdjRel *A;
  iftBand  **weight; /* one kernel per row and one band per column */
  int        nkernels;
  int        nbands;
} iftMMKernel;

%extend iftMMKernel{
	~iftMMKernel(){
		iftMMKernel *ptr = ($self);
		iftDestroyMMKernel(&ptr);

		#ifdef PYIFT_DEBUG
			printf("iftMMKernel deleted\n");
		#endif
	}
}

%newobject iftRectangular;
iftAdjRel *iftRectangular(int xsize, int ysize);


%{
	iftImage* iftPixelGradSegmentationSobel(iftImage* image, iftLabeledSet *seed, int num_iter, char* dataset_path, char* outputwg, iftImage* border_image, char* outputbas){
		
		iftRandomSeed(IFT_RANDOM_SEED);
		iftAdjRel *A = iftCircular(1.5);
		//iftLabeledSet *S=iftReadSeeds2D(outputwg,image);
		iftImage *basins = iftSobelGradientMagnitude(image);
		iftWriteImageP2(basins,outputbas);
	        iftImage *label_image  = iftRelaxedWatershed(basins,A,seed,5,0.5);
		//iftImage *label_image  = iftRelaxedWatershed(basins,A,S,5,0.5);
		//label_image = iftWatershed(basins,A,seed)
		//iftDestroyLabeledSet(&S);
		iftDestroyAdjRel(&A);
		iftDestroyImage(&basins);
		return label_image;
	}
%}

%newobject iftPixelGradSegmentationSobel;
iftImage* iftPixelGradSegmentationSobel(iftImage* image, iftLabeledSet *seed, int num_iter, char* dataset_path, char* outputwg, iftImage* border_image, char* outputbas);

%{
	iftImage* iftPixelGradSegmentationEnhancement(iftImage* image, iftLabeledSet *seed, float spatial_radius, float volume_threshold, int num_iter, char* dataset_path, char* outputwg, iftImage* border_image, char* outputbas){
		
		iftRandomSeed(IFT_RANDOM_SEED);
		iftAdjRel *A = iftCircular(1.5);
		//iftLabeledSet *S=iftReadSeeds2D(outputwg,image);
	        iftImage *basins = iftEnhanceEdges(image,A,seed,0.5);
		iftWriteImageP2(basins,outputbas);
	        iftImage *label_image  = iftRelaxedWatershed(basins,A,seed,5,0.5);
		//iftImage *label_image  = iftRelaxedWatershed(basins,A,S,5,0.5);
		//label_image = iftWatershed(basins,A,seed)
		//iftDestroyLabeledSet(&S);
		iftDestroyAdjRel(&A);
		iftDestroyImage(&basins);
		return label_image;
	}
%}

%newobject iftPixelGradSegmentationEnhancement;
iftImage* iftPixelGradSegmentationEnhancement(iftImage* image, iftLabeledSet *seed, float spatial_radius, float volume_threshold, int num_iter, char* dataset_path, char* outputwg, iftImage* border_image, char* outputbas);


%{
	iftImage* iftMergeWatergray(iftImage* image, float volume_threshold, int nregions,char* outputwg){

		iftAdjRel *adj1 = iftCircular(sqrtf(2.0));
		iftAdjRel *adj2 = iftCircular(1.0);
		
		iftImage *basins = iftImageBasins(image,adj1);
		iftImage *marker = iftVolumeClose(basins, volume_threshold*2);

		iftImage *wg_label_image = iftWaterGray(basins,marker,adj1);

		//iftDataSet* dataset = iftSupervoxelsToMeanStdSizeDataset(image, wg_label_image);
		iftDataSet* dataset = iftSupervoxelsToSelectiveSearchDataset(image, wg_label_image, 25, YCbCr_CSPACE);

		/*
		float alpha[10];
		alpha[0] = 0.2;
		alpha[1] = 1.0;
		alpha[2] = 1.0;
		alpha[3] = 0.2;
		alpha[4] = 1.0;
		alpha[5] = 1.0;
		alpha[6] = 0.0;
		alpha[7] = 0.0;
		alpha[8] = 0.0;
		alpha[9] = 0.0;
		*/

		//iftRegionHierarchy *rh = iftCreateRegionHierarchy(wg_label_image, dataset, adj2, iftDistMeanStdSizeSupervoxel, iftMergeMeanStdSizeSupervoxel, alpha);
		//iftRegionHierarchy *rh = iftCreateRegionHierarchy(wg_label_image, dataset, adj2, iftDistSelectiveSearchSupervoxel, iftMergeSelectiveSearchSupervoxel, alpha);
		iftRegionHierarchy *rh = iftCreateRegionHierarchy(dataset, adj2, iftMergeSelectiveSearchSupervoxel);
		iftImage *label_image = iftFlattenRegionHierarchy(rh, nregions);


		iftImage *label = iftCopyImage(label_image);		
		iftImage *clon = iftCopyImage(image);
		iftAdjRel *adj3 = iftCircular(0.0);
		iftColor RGB, YCbCr;
		RGB.val[0] = 0;
		RGB.val[1] = 0;
		RGB.val[2] = 255;
		YCbCr      = iftRGBtoYCbCr(RGB);
		iftDrawBorders(clon,label,adj1,YCbCr,adj3);
		iftWriteImageP6(clon, outputwg);
		iftDestroyImage(&clon);


		iftDestroyImage(&wg_label_image);
		iftDestroyAdjRel(&adj1);
		iftDestroyAdjRel(&adj2);
		iftDestroyAdjRel(&adj3);
		iftDestroyImage(&basins);
		iftDestroyImage(&marker);
		iftDestroyDataSet(&dataset);
		iftDestroyRegionHierarchy(&rh);

		return label_image;
	}
%}

%newobject iftMergeWatergray;
iftImage* iftMergeWatergray(iftImage* image, float volume_threshold, int nregions,char* outputwg);


%newobject paint_seeds_on_image;
iftImage *paint_seeds_on_image(iftImage* image, iftLabeledSet* seeds);

%newobject paint_segmentation_on_image;
iftImage *paint_segmentation_on_image(iftImage* image, iftImage* segmentation);

%{
	iftImage *paint_seeds_on_image(iftImage* image, iftLabeledSet* seeds){
		iftColor red;
	    red.val[0] = 255; red.val[1] = 0; red.val[2] = 0;
	    red = iftRGBtoYCbCr(red);
	    
	    iftColor yellow;
	    red.val[0] = 255; red.val[1] = 0; red.val[2] = 255;
	    yellow = iftRGBtoYCbCr(yellow);
	    
	    iftImage *painted = iftCopyImage(image);
	    
	    iftLabeledSet *s = seeds;
	    while(s){
	    	int p = s->elem;
	    	int label = s->label;
	    	
	    	if(label == 0){
	    		painted->val[p] = red.val[0];
	    		painted->Cb[p] = red.val[1];
	    		painted->Cr[p] = red.val[2];
	    	}else{
	    		painted->val[p] = yellow.val[0];
	    		painted->Cb[p] = yellow.val[1];
	    		painted->Cr[p] = yellow.val[2];
	    	} 
	    
	    	s = s->next;
	    }
	    
	    return painted;
	}
	
	iftImage *paint_segmentation_on_image(iftImage* image, iftImage* segmentation){
		iftAdjRel *adj2 = iftCircular(1.0);
		iftAdjRel *adj3 = iftCircular(1.0);
	
		iftColor rgb_color;
	    rgb_color.val[0] = 255; rgb_color.val[1] = 0; rgb_color.val[2] = 0;
	    rgb_color = iftRGBtoYCbCr(rgb_color);
	
	    iftImage *painted = iftCopyImage(image);
	    iftDrawBorders(painted, segmentation, adj2, rgb_color, adj3);
	    
	    iftDestroyAdjRel(&adj2);
	    iftDestroyAdjRel(&adj3);
	    
	    return painted;
	}
%}

%newobject relabel_color_image;
iftImage* relabel_color_image(iftImage *image);

%{

iftImage* relabel_color_image(iftImage *image){
	iftImage* map = iftCreateImage(image->xsize, image->ysize, image->zsize);

	int p;
	for(p = 0; p < map->n; p++){
		map->val[p] = image->val[p] + image->Cb[p]*255 + image->Cr[p]*255*255 + 1;
	}
	
	iftAdjRel *adj = iftCircular(1.0); 
	iftImage *relabeled = iftRelabelRegions(map, adj);
	
	iftDestroyImage(&map);
	
	return relabeled;
}

%}

%newobject iftOr;
iftImage* iftOr(iftImage *img1, iftImage *img2);

%newobject iftRelabelRegions;
iftImage* iftRelabelRegions(iftImage* labelled, iftAdjRel* adj_rel);


void setIftImage(iftImage* image, PyObject* array);
%{
	void setIftImage(iftImage* image, PyObject* array){
		int p;
		for(p = 0; p < image->n; p++){
			image->val[p] = (int)PyInt_AsLong(PyList_GetItem(array,p));
		}
	}
%}

