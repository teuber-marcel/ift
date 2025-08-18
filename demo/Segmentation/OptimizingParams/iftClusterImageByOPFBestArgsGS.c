#include "ift.h"


double find_best_args(iftDict* problem, iftDict* params) {

  char *orig_img_path=iftGetStrValFromDict("orig_img_path",problem);
  char *gt_labels_path=iftGetStrValFromDict("gt_labels_path",problem);
  char *orig_img_ext= iftGetStrValFromDict("orig_img_ext", problem);

  iftDir *image_files = iftLoadFilesFromDirBySuffix(orig_img_path, orig_img_ext);
  char *gt_img_ext="";
  if (!strcmp(orig_img_ext,"png") || !strcmp(orig_img_ext,"PNG") || !strcmp(orig_img_ext,"ppm") || !strcmp(orig_img_ext,"PPM") || !strcmp(orig_img_ext,"pgm") || !strcmp(orig_img_ext,"PGM"))
    gt_img_ext="pgm";
  else
  if (!strcmp(orig_img_ext,"scn"))
    gt_img_ext="scn";
  else
      iftError("Invalid extension for orig image directory", "find_best_args");
  iftDir *gt_label_files = iftLoadFilesFromDirBySuffix(gt_labels_path, gt_img_ext);

  int file_count=iftGetLongValFromDict("count_train_img",problem);
  int limit_nb_superp=iftGetLongValFromDict("limit_nb_superp",problem);
  int limit_secs=iftGetLongValFromDict("limit_secs",problem);
  int type_measure = iftGetLongValFromDict("type_measure", problem);

  float measure_acc =0.0;
  int superpixels_acc=0;

  iftImage        *img=NULL,*label=NULL,*aux=NULL,*border=NULL,*gt_label=NULL,*gt_border=NULL;
  iftDataSet      *Z=NULL;
  iftKnnGraph     *graph=NULL;
  timer           *t1=NULL,*t2=NULL;
  iftAdjRel *A;
  iftMImage *mimg,*eimg;

  float train_perc_arg=iftGetDblValFromDict("train_perc_arg",problem);
  float kmax_arg=iftGetDblValFromDict("kmax_arg",params);
  int area_arg=(int)iftGetLongValFromDict("area_arg",problem);
  bool exited=false;

  for (int i=0;i<file_count;i++){

    img=iftReadImageByExt(image_files->files[i]->path);

    /* convert the image to multi-image*/
    if (!iftIsColorImage(img)) {
      mimg = iftImageToMImage(img, GRAYNorm_CSPACE);
      if (img->zsize > 1)
        A = iftSpheric(1.0);
      else
        A = iftCircular(sqrtf(2.0));
      eimg=iftExtendMImageByAdjacencyAndVoxelCoord(mimg,A,1);
      iftDestroyMImage(&mimg);
      iftDestroyAdjRel(&A);
      mimg = eimg;
    }
    else{
      mimg = iftImageToMImage(img, LABNorm2_CSPACE);
      eimg = iftExtendMImageByVoxelCoord(mimg,1);
      iftDestroyMImage(&mimg);
      mimg = eimg;
    }

    Z= iftMImageToDataSet(mimg);

    t1=iftTic();
//    iftSelectUnsupTrainSamples(Z,train_perc_arg,100);
    iftImage *mask1 = iftSelectImageDomain(mimg->xsize,mimg->ysize,mimg->zsize);
    iftImage *mask_sampling = iftGridSampling(mimg, mask1, train_perc_arg);

    int train_samples_nb=0;
    for (int s=0;s<Z->nsamples;s++){
      int voxel=Z->sample[s].id;
      if (mask_sampling->val[voxel]){
        Z->sample[s].status=IFT_TRAIN;
        train_samples_nb++;
      }
    }
    Z->ntrainsamples=train_samples_nb;
    iftDestroyImage(&mask1);
    iftDestroyImage(&mask_sampling);

    int kmax = (int) iftMax((kmax_arg * Z->ntrainsamples), 1);

    graph = iftCreateKnnGraph(Z,kmax);
    iftUnsupTrain(graph,iftNormalizedCut);
    iftUnsupClassify(graph,Z);

    t2= iftToc();
    float time = iftCompTime(t1, t2) / 1000;
    /* if the method execution is more than 1 minute discard this configuration of parameters*/
    if (time > (float)limit_secs){
      measure_acc =IFT_INFINITY_FLT_NEG;
      exited=true;
      break;
    }

    label = iftDataSetClusterInformationToLabelImage(Z, false);

    /* do smoothing in the label image*/
    aux = iftSmoothRegionsByDiffusion(label,img,0.5,5);
    iftDestroyImage(&label);
    label=aux;

    /* eliminate short superpixels*/
    aux = iftSelectAndPropagateRegionsAboveArea(label,area_arg);
    iftDestroyImage(&label);
    label=aux;

    int nb_sup=iftMaximumValue(label);
    if (nb_sup > 2*limit_nb_superp){
      measure_acc =IFT_INFINITY_FLT_NEG;
      exited=true;
      break;
    }
    superpixels_acc+=nb_sup;

    gt_label=iftReadImageByExt(gt_label_files->files[i]->path);
    aux=iftRelabelGrayScaleImage(gt_label,0);
    iftDestroyImage(&gt_label);
    gt_label=aux;

    if (type_measure){
      /*computing under segmentation error*/
      /*here we are minimizing not maximizing*/
      measure_acc -= iftUnderSegmentation(gt_label,label);
    }
    else{
      /*compute boundary recall*/
      border  = iftBorderImage(label,0);
      gt_border=iftBorderImage(gt_label,0);
      measure_acc += iftBoundaryRecall(gt_border, border, 2.0);

      iftDestroyImage(&border);
      iftDestroyImage(&gt_border);
    }

    iftDestroyImage(&img);
    iftDestroyMImage(&mimg);
    iftDestroyDataSet(&Z);
    iftDestroyImage(&label);

    iftDestroyImage(&gt_label);
  }

  if (!exited){
    float mean_sup=superpixels_acc/file_count;
    if (mean_sup > (float)limit_nb_superp)
      measure_acc =IFT_INFINITY_FLT_NEG;
    else
      measure_acc /=file_count;
  }

  iftDestroyDir(&image_files);
  iftDestroyDir(&gt_label_files);

  return measure_acc;

}

int main(int argc, char *argv[])
{
  iftRandomSeed(IFT_RANDOM_SEED);

  if (argc != 13)
    iftError("Usage: iftClusterImageByOPFBestArgsGS <dir_orig_img> <dir_label_img> <count_train_img> <limit_nb_superp> <limit_secs> <optimizing measure 0(BR)/1(UE)> <count_train_samples> <orig_img_ext (ppm:png,...)> <init_kmax1> <final_kmax1> <step_kmax1> <init_area>","main");

  iftDict* params = iftCreateDict();
  iftDict* problem = iftCreateDict();

  iftInsertIntoDict("kmax_arg", iftRange(atof(argv[9]), atof(argv[10]), atof(argv[11])),params);

  iftInsertIntoDict("train_perc_arg", atof(argv[7]),problem);
  iftInsertIntoDict("orig_img_ext",argv[8],problem);
  iftInsertIntoDict("orig_img_path", argv[1],problem);
  iftInsertIntoDict("gt_labels_path", argv[2],problem);
  iftInsertIntoDict("count_train_img", atoi(argv[3]),problem);
  iftInsertIntoDict("limit_nb_superp", atoi(argv[4]),problem);
  iftInsertIntoDict("limit_secs", atoi(argv[5]),problem);
  iftInsertIntoDict("type_measure", atoi(argv[6]),problem);
  iftInsertIntoDict("area_arg", atoi(argv[12]),problem);

  iftDict *result =iftGridSearch(params, find_best_args, problem);

  printf("Best parameters for %s with %d images an limit number superpixels %d and %d seconds with measure-> %s\n",argv[1],atoi(argv[3]),atoi(argv[4]),atoi(argv[5]),atoi(argv[6])?"ue":"br");

  printf("best_train_perc_arg -> %.4f\n",(float)iftGetDblValFromDict("train_perc_arg",problem));
  printf("best_kmax_arg -> %.4f\n",(float)iftGetDblValFromDict("kmax_arg",result));
  printf("best_area_arg -> %d\n",(int)iftGetLongValFromDict("area_arg",problem));
  float res=(float)iftGetDblValFromDict("best_func_val",result);
  printf("best_objetive_func -> %.4f\n",atoi(argv[6])?-res:res);

  iftDestroyDict(&params);
  iftDestroyDict(&problem);
  iftDestroyDict(&result);

  return(0);
}
