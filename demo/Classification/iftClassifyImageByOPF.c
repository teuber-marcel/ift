#include "ift.h"

/************************** HEADERS **************************/
iftDict *iftGetArgs(int argc, const char *argv[]);
void iftGetRequiredArgs(  iftDict *args, char **img_path, char **clf_path, char **out_img_path);
void iftValidateInputs(const char *img_path, const char *clf_path, const char *out_img_path);
int iftGetImageMaxRange(  iftDict *args,   iftImage *img);
/*************************************************************/

iftMImage *iftExtractGrayFeatures(iftImage *img)
{
    iftMImage *mimg = iftCreateMImage(img->xsize,img->ysize,img->zsize,2);
    iftAdjRel *A;

    if (iftIs3DImage(img)){
        A  = iftSpheric(1.0);
    } else {
        A = iftCircular(1.0);
    }

#pragma omp parallel shared(img,mimg,A)
    for (int p=0; p < img->n; p++) {
        iftVoxel u = iftGetVoxelCoord(img,p);
        mimg->val[p][0] = img->val[p];
        mimg->val[p][1] = 0.0;
	int n = 0;
        for (int i=1; i < A->n; i++){
            iftVoxel v = iftGetAdjacentVoxel(A,u,i);
            if (iftValidVoxel(img,v)){
                int q = iftGetVoxelIndex(img,v);
		mimg->val[p][1] += fabs(img->val[q]-img->val[p]);
		n++;
            }
        }
	mimg->val[p][1] /= n;
    }

    iftDestroyAdjRel(&A);
    return(mimg);
}

int main(int argc, const char *argv[]) {
  iftImage *img = NULL, *label = NULL, *smoothed = NULL;
  iftMImage *mimg = NULL;
  iftCplGraph *graph = NULL;
  iftLabeledSet *training_set = NULL;
  iftDataSet *Z = NULL;
  size_t mem_start, mem_end;
  timer *t1, *t2;

  mem_start = iftMemoryUsed();

  iftDict *args = iftGetArgs(argc, argv);

  // mandatory args
  char *img_path        = NULL;
  char *clf_path        = NULL;
  char *out_img_path    = NULL;
  // optional args
  bool save_obj_maps = iftDictContainKey("--save-object-maps", args, NULL);
  double smooth_factor = 0.0;

  if(iftDictContainKey("--smooth-factor", args, NULL))
    smooth_factor = iftGetDblValFromDict("--smooth-factor", args);
  printf("- Smooth factor: %lf\n", smooth_factor);


  iftGetRequiredArgs(args, &img_path, &clf_path, &out_img_path);

  t1 = iftTic();

  img = iftReadImageByExt(img_path);

  if (smooth_factor > 0.0) {
    /* Smoothing image */
    iftKernel *K = NULL;
    if(iftIs3DImage(img))
      K = iftGaussianKernel(1.5, smooth_factor*iftMaximumValue(img));
    else
      K = iftGaussianKernel2D(1.5, smooth_factor*iftMaximumValue(img));

    smoothed = iftFastLinearFilter(img, K, 0);

    iftDestroyKernel(&K);
    iftDestroyImage(&img);

    img = smoothed;
  }

  if (!iftIsColorImage(img)) {
    // mimg = iftExtractGrayFeatures(img);
    mimg = iftImageToMImage(img, GRAY_CSPACE);
  } else {
    mimg = iftImageToMImage(img, LAB_CSPACE);
  }

  
  graph = iftReadCplGraph(clf_path);
  
  Z   = iftMImageToDataSet(mimg, NULL,0);

  iftSetStatus(Z, IFT_TEST);
  iftClassifyWithCertaintyValues(graph, Z);
  
  puts("- Saving Resulting Labeled Image");
  label  = iftDataSetToLabelImage(Z,NULL,true,IFT_LABEL);

  iftImage* norm = iftNormalize(label, 0, 255);
  iftWriteImageByExt(norm, out_img_path);
  iftDestroyImage(&norm);

  if (save_obj_maps) {
    puts("- Saving Resulting Object Maps");
    char *base        = iftBasename(out_img_path);
    const char *ext   = iftFileExt(out_img_path);
    int img_max_range = iftGetImageMaxRange(args, img);
    printf("  - Image Range: [0, %d]\n", img_max_range);

    // for each class except the background (class 1 in Z)
    for (int lb = 2; lb <= graph->Z->nclasses; lb++) {
      char obj_map_path[512];
      sprintf(obj_map_path, "%s_obj_map%d%s", base, lb - 1, ext);
      printf("  - %s\n", obj_map_path);

      iftImage *obj_map = iftDataSetObjectMap(Z, NULL, img_max_range, lb);
      iftWriteImageByExt(obj_map, obj_map_path);
      iftDestroyImage(&obj_map);
    }
    iftFree(base);
  }

  iftDestroyLabeledSet(&training_set);
  iftDestroyDistanceTable(&iftDist);
  iftDestroyImage(&img);
  iftDestroyMImage(&mimg);
  iftDestroyImage(&label);
  //  iftDestroyImage(&objmap);
  iftDestroyDataSet(&Z);
  iftDestroyCplGraph(&graph);
  iftDestroyDict(&args);
  iftFree(img_path);
  iftFree(clf_path);
  iftFree(out_img_path);

  mem_end = iftMemoryUsed();
  iftVerifyMemory(mem_start,mem_end);

  t2 = iftToc();
  puts(iftFormattedTime(iftCompTime(t1,t2)));


    return(0);
}
/************************** SOURCES **************************/
iftDict *iftGetArgs(int argc, const char *argv[]) {
  char program_description[2048] = \
            "Classifies an Image (all voxels) using an OPF Classifier.\n" \
            "The Image is converted to a Dataset where each sample is a voxel, and its feature vector " \
            "is only the voxel's brightness value.\n" \
            "The program might also compute the object map for each object's label based on the weights " \
            "of the dataset's samples." \
            "PS: The OPF Classifier is trained by the program demo/Classification/iftTrainImageClassifierByOPF.c";

  iftCmdLineOpt cmd_line_opts[] = {
          {.short_name = "-i", .long_name = "--input-test-img", .has_arg=true, .arg_type=IFT_STR_TYPE,
                  .required=true, .help="Input Test Image to be classified."},
          {.short_name = "-c", .long_name = "--classifier", .has_arg=true, .arg_type=IFT_STR_TYPE,
                  .required=true, .help="Classifier's pathname (*.zip)."},
          {.short_name = "-o", .long_name = "--output-image", .has_arg=true, .arg_type=IFT_STR_TYPE,
                  .required=true, .help="Output Labeled Image's Pathname."},
          {.short_name = "-f", .long_name = "--smooth-factor", .has_arg=true, .arg_type=IFT_DBL_TYPE,
                  .required=false, .help="Factor used to smooth the image (e.g., 0.3)."},
          {.short_name = "-s", .long_name = "--save-object-maps", .has_arg=false,
                  .required=false, .help="Saves the resulting object maps for each class with the same " \
                                     "Output Labeled Image Basename."},
          {.short_name = "-b", .long_name = "--img-depth", .has_arg=true, .arg_type=IFT_LONG_TYPE,
                  .required=false, .help="Input Image Depth in bits (8, 12, 16, ...)\n" \
                                    "Default: it tries to find the image depth automatically"}
  };
  int n_opts = sizeof(cmd_line_opts) / sizeof (iftCmdLineOpt);

  // Parser Setup
  iftCmdLineParser *parser = iftCreateCmdLineParser(program_description, n_opts, cmd_line_opts);
  iftDict *args            = iftParseCmdLine(argc, argv, parser); // getting the passed options/arguments
  iftDestroyCmdLineParser(&parser);

  return args;
}

void iftGetRequiredArgs(  iftDict *args, char **img_path, char **clf_path, char **out_img_path) {
  *img_path     = iftGetStrValFromDict("--input-test-img", args);
  *clf_path     = iftGetStrValFromDict("--classifier", args);
  *out_img_path = iftGetStrValFromDict("--output-image", args);

  iftValidateInputs(*img_path, *clf_path, *out_img_path);

  puts("-----------------------");
  printf("- Input Image: \"%s\"\n", *img_path);
  printf("- Classifier: \"%s\"\n", *clf_path);
  printf("- Output Labeled Image: \"%s\"\n", *out_img_path);
  puts("-----------------------");
}


void iftValidateInputs(const char *img_path, const char *clf_path, const char *out_img_path) {
  if (!iftIsImageFile(img_path))
    iftError("Invalid Input Image: \"%s\"", "iftValidateInputs", img_path);

  if (iftFileExists(clf_path)) {
    if (!iftEndsWith(clf_path, ".zip")) {
      iftError("Invalid Extension for the Classifier's File: \"%s\"... Use *.zip",
               "iftValidateInputs", clf_path);
    }
  }
  else iftError("Classifier's File \"%s\" does not exist", "iftValidateInputs", clf_path);

  if (!iftIsImagePathnameValid(out_img_path)) {
    iftError("Invalid Output Image Pathname: \"%s\"", "iftValidateInputs", out_img_path);
  }

  char *parent_dir = iftParentDir(out_img_path);
  if (!iftDirExists(parent_dir)) {
    iftMakeDir(parent_dir);
  }
  iftFree(parent_dir);
}


int iftGetImageMaxRange(  iftDict *args,   iftImage *img) {
  int img_max_range;

  if (iftDictContainKey("--img-depth", args, NULL)) {
    int img_depth = iftGetLongValFromDict("--img-depth", args);
    if (img_depth <= 0)
      iftError("Invalid Image Depth: %d... Try > 0", "iftGetOptionalArgs", img_depth);

    img_max_range = (1 << img_depth) - 1; // (2^img_depth) - 1
  }
  else img_max_range = iftNormalizationValue(iftMaximumValue(img));

  return img_max_range;
}
/*************************************************************/
