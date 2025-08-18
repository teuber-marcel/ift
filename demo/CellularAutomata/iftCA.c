#include "ift.h"
#include "include/ca.h"

#define DEBUG 0
#define SAVE_FREQ 10
#define BORDER_DIST 1
#define MAX_OBJ_AREA 9000
#define MIN_OBJ_AREA 1000

void FilterSaliencyEgg(iftImage *saliency) {
  iftImage *aux1 = NULL;
  aux1 = iftSelectEggCompFromSaliencyMap(
    saliency, BORDER_DIST, MIN_OBJ_AREA, MAX_OBJ_AREA
  );

  #pragma omp parallel for
  for (size_t p=0; p < saliency->n; p++) {
    if (aux1->val[p] == 0) {
      saliency->val[p] = 0;
    }
  }

  if (saliency != aux1) {
    iftDestroyImage(&aux1);
  }
}

iftCAModel *InitializeCA(
  char *input_image_path, char *saliency_path, float neighborhood_size,
  int mode, int is_brain
) {
  iftAdjRel *A;
  iftCAModel *ca_model = calloc(1, sizeof(iftCAModel));

  // Loads intensity image
  iftImage *orig = NULL;
  iftImage *saliency = NULL;
  // If orig image greater then saliency, and mode 0, subsample orig image
  if (mode == 0) {
    printf("[INFO] Subsampling Orig Image\n");
    iftImage *tmp = iftReadImageByExt(input_image_path);
    saliency = iftReadImageByExt(saliency_path);
    if (
      tmp->xsize == saliency->xsize && tmp->ysize == saliency->ysize
      && tmp->zsize == saliency->zsize
    ) {
      orig = tmp;
    } else {
      float sx = (float)saliency->xsize / (float)tmp->xsize;
      float sy = (float)saliency->ysize / (float)tmp->ysize;
      float sz = (float)saliency->zsize / (float)tmp->zsize;
      if (sz == 1) {
        orig = iftInterp2D(tmp, sx, sy);
      } else {
        orig = iftInterp(tmp, sx, sy, sz);
      }
      iftDestroyImage(&tmp);
    }
  }

  else if (mode == 1) {
    printf("[INFO] Oversampling Saliency Image\n");
    orig = iftReadImageByExt(input_image_path);
    iftImage *tmp = iftReadImageByExt(saliency_path);
    if (
      tmp->xsize == orig->xsize && tmp->ysize == orig->ysize
      && tmp->zsize == orig->zsize
    ) {
      saliency = tmp;
    } else {
      float sx = (float)orig->xsize / (float)tmp->xsize;
      float sy = (float)orig->ysize / (float)tmp->ysize;
      float sz = (float)orig->zsize / (float)tmp->zsize;
      if (sz == 1) {
        saliency = iftInterp2D(tmp, sx, sy);
      } else {
        saliency = iftInterp(tmp, sx, sy, sz);
      }
      iftDestroyImage(&tmp);
    }
  }

  // Reads saliency image, which will be used to initialize foreground seeds
  ca_model->saliency = saliency;
  ca_model->fg_strength = iftCreateFImage(orig->xsize, orig->ysize, orig->zsize);
  ca_model->bg_strength = iftCreateFImage(orig->xsize, orig->ysize, orig->zsize);
  ca_model->label = iftCreateImage(orig->xsize, orig->ysize, orig->zsize);

  if(iftIs3DImage(orig) || is_brain) {
    ca_model->i_max = 65535;
    if (iftIs3DImage(orig)) {
      ca_model->neighborhood = iftSpheric(neighborhood_size);
    } else {
      ca_model->neighborhood = iftRectangular(neighborhood_size, neighborhood_size);
    }
    printf("N Neighbours is %d\n", ca_model->neighborhood->n);

  }
  else {
    ca_model->i_max = 255;
    A = iftCircular(10);
    ca_model->neighborhood = iftRectangular(neighborhood_size, neighborhood_size);
  }

  iftImage *aux = NULL;
  // Brain initialization
  if (is_brain) {
    ca_model->orig = iftImageToMImage(orig, GRAY_CSPACE);
    ca_model->brain_mask = iftCreateImageFromImage(orig);
    #pragma omp parallel for
    for (size_t p = 0; p < ca_model->orig->n; p++) {
      ca_model->orig->val[p][0] = ca_model->orig->val[p][0] / (float) ca_model->i_max;
      ca_model->fg_strength->val[p] = (float)ca_model->saliency->val[p] / 255;
      ca_model->brain_mask->val[p] = (orig->val[p] != 0);
      ca_model->bg_strength->val[p] = (orig->val[p] == 0);
      ca_model->label->val[p] = (ca_model->saliency->val[p] != 0);
    }
  }
  else {
    iftImage *dilated_s = iftDilate(ca_model->saliency, A, NULL);
    aux = iftComplement(dilated_s);
    iftDestroyImage(&dilated_s);
    ca_model->orig = iftImageToMImage(orig, LABNorm2_CSPACE);
    #pragma omp parallel for
    for (size_t p = 0; p < ca_model->orig->n; p++) {
      ca_model->fg_strength->val[p] = (float)ca_model->saliency->val[p] / ca_model->i_max;
      ca_model->bg_strength->val[p] = (float)aux->val[p]  / ca_model->i_max;
      ca_model->label->val[p] = (ca_model->saliency->val[p] != 0);
    }
    iftDestroyAdjRel(&A);
  }
  printf("[INFO] Salience max value is: %d\n", iftMaximumValue(ca_model->saliency));
  printf("[INFO] ca_model->i_max is: %d\n", ca_model->i_max);
  printf("[INFO] Orig max value is: %f\n", iftMMaximumValue(ca_model->orig, 0));
  printf("[INFO] Fg strength max value is: %f\n", iftFMaximumValue(ca_model->fg_strength));
  printf("[INFO] Bg strength max value is: %f\n", iftFMaximumValue(ca_model->bg_strength));
  ca_model->fg_dist = IFT_INFINITY_DBL;
  ca_model->bg_dist = IFT_INFINITY_DBL;

  // Releases resources
  iftDestroyImage(&orig);
  iftDestroyImage(&aux);

  return ca_model;
}

void DestroyCAModel(iftCAModel **ca_model) {
  if ((*ca_model) != NULL) {
    iftDestroyMImage(&(*ca_model)->orig);
    iftDestroyImage(&(*ca_model)->saliency);
    iftDestroyImage(&(*ca_model)->label);
    iftDestroyFImage(&(*ca_model)->fg_strength);
    iftDestroyFImage(&(*ca_model)->bg_strength);
    iftDestroyAdjRel(&(*ca_model)->neighborhood);
    if (((*ca_model)->brain_mask) != NULL) {
      iftDestroyImage(&(*ca_model)->brain_mask);
    }
    iftFree(*ca_model);
  }
}

float similarity_func(
  iftMImage *orig, iftImage *label, int p, int q, int label_evolving, int gray
)  {
  float g;
  if (!gray) {
    // Pixel p and q at band 0 (Using only L for parasites, update it to use LAB TO, adaptively? Parameter?)
    float y_p[3] = {orig->val[p][0], orig->val[p][1], orig->val[p][2]};
    float y_q[3] = {orig->val[q][0], orig->val[q][1], orig->val[q][2]};
    float lab_dist = iftEuclideanDistance(y_p, y_q, 3);

    // Constants for propagation inside label
    const float THRESHOLD = 0.20;
    const float BETA_STRONG = 0.6;
    const float BETA_NORMAL = 1.0;
    float beta;
    if (
      (lab_dist < THRESHOLD) && (label->val[q] == label_evolving) && (label_evolving == 1)
    ) {
      beta = BETA_STRONG;
    }
    else {
      beta = BETA_NORMAL;
    }
    // float g = expf(-beta *  (fabs(y_p - y_q)));
    g =  expf(-beta *  lab_dist);
  } else {
    float y_p = orig->val[p][0];
    float y_q = orig->val[q][0];
    float beta;
    float dist = fabs(y_p - y_q);

    if (
      (y_p > y_q) && (label->val[q] == label_evolving) && (label_evolving == 1)
    ) {
      beta = 0.6;
    } else {
      beta = 1;
    }

    g = expf(-beta * dist);
  }

    return g;
}

double evolve_state(
  int *epoch, iftMImage *orig, iftFImage **strength, iftImage **label,
  iftAdjRel *adj_rel, int label_evolving, int is_brain
) {
  iftFImage *new_strength = iftCreateFImageFromFImage(*strength);
  iftImage *new_label = iftCreateImageFromImage(*label);

  // Copies old strength
  #pragma omp parallel for
  for (size_t p = 0; p < (*strength)->n; p++) {
    float q_max = (*strength)->val[p];
    iftVoxel u = iftFGetVoxelCoord((*strength), p);
    // Verifies adjacency (Neighboors cells tries to attack current cell)
    for (size_t adj = 1; adj < adj_rel->n; adj++) {
      iftVoxel v = iftGetAdjacentVoxel(adj_rel, u, adj);
      // If neighboor cell is in image domain, then it attacks the current cell
      if (iftValidVoxel((*strength), v)) {
        int q = iftFGetVoxelIndex((*strength), v);
        float q_aux = similarity_func(orig, *label, p, q, label_evolving, is_brain)
                       * (*strength)->val[q];
        // If q_aux is greater than q_max, the current cell is conquered
        if (q_aux > q_max) {
          new_label->val[p] = (*label)->val[q];
          q_max = q_aux;
        }
      }
    }
    new_strength->val[p] = q_max;
  }

  // Computes diff
  double dist = 0;
  #pragma omp parallel for reduction(+:dist)
  for (size_t p = 0; p < (*strength)->n; p++) {
    dist += sqrtf( powf( (new_strength->val[p] - (*strength)->val[p]), 2 ) );
  }
  dist = dist / (*strength)->n;
  (*epoch)++;

  // Destroy old strengh and updates it as the currently evolved strength
  iftDestroyFImage(strength);
  iftDestroyImage(label);
  (*strength) = new_strength;
  (*label) = new_label;

  return dist;
}

void evolve_ca_model(iftCAModel *ca_model, double dist, int is_brain) {
  // Evolves simultaneously, to forster competition between cells
  timer *tstart, *tend;
  int epoch = 0;

  if (DEBUG) {
    iftImage *l_debug = NULL;
    // SAVE FG
    iftImage *fg_debug = iftFImageToImage(ca_model->fg_strength, 255);
    iftWriteImageByExt(fg_debug, "debug/fg_debug.png");
    iftDestroyImage(&fg_debug);

    // Save BG
    iftImage *bg_debug = iftFImageToImage(ca_model->bg_strength, 255);
    iftWriteImageByExt(bg_debug, "debug/bg_debug.png");
    iftDestroyImage(&bg_debug);

    // Save Label
    l_debug = iftCreateImageFromImage(ca_model->label);
    for (size_t p = 0; p < ca_model->label->n; p++) {
      l_debug->val[p] = ca_model->label->val[p] * 255;
    }
    iftWriteImageByExt(l_debug, "debug/label_debug.png");
    iftDestroyImage(&l_debug);
  }
  tstart = iftTic();
  // [TODO]
  // Improve labels competition
  while (ca_model->fg_dist > dist) {
    ca_model->fg_dist = evolve_state(
      &epoch, ca_model->orig, &(ca_model->fg_strength), &(ca_model->label),
      ca_model->neighborhood, 1, is_brain
    );


    if (DEBUG) {
      char debug_path[512];
      iftImage *l_debug = NULL;
      l_debug = iftCreateImageFromImage(ca_model->label);
      sprintf(debug_path, "debug/epoch_%d_label_debug.png", epoch);
      for (size_t p = 0; p < ca_model->label->n; p++) {
        l_debug->val[p] = ca_model->label->val[p] * 255;
      };
      iftWriteImageByExt(l_debug, debug_path);
      iftDestroyImage(&l_debug);
    }
    fflush(stdout);
    printf("\r[INFO] Evolving fg strenght epoch %d dist %lf", epoch, ca_model->fg_dist);
  }
  tend = iftToc();
  printf("\n[INFO] Evolution took %s\n", iftFormattedTime(iftCompTime(tstart, tend)));
  epoch = 0;
  tstart = iftTic();
  while (ca_model->bg_dist > dist)
  {
    ca_model->bg_dist = evolve_state(
      &epoch, ca_model->orig, &(ca_model->bg_strength), &(ca_model->label),
      ca_model->neighborhood, 0, is_brain
    );
    fflush(stdout);
    printf("\r[INFO] Evolving bg strenght epoch %d dist %lf", epoch, ca_model->bg_dist);
  }

  tend = iftToc();
  printf("\n[INFO] Evolution took %s\n\n", iftFormattedTime(iftCompTime(tstart, tend)));
}

#define EPS 10e-9
iftFImage *GetProbMap(iftCAModel *ca_model) {
  iftFImage *prob_map = iftCreateFImageFromFImage(ca_model->fg_strength);

  #pragma omp parallel for
  for (size_t p = 0; p < ca_model->fg_strength->n; p++) {
    prob_map->val[p] = log(ca_model->bg_strength->val[p] + EPS) /
        (
          log(ca_model->bg_strength->val[p] + EPS)
          + log(ca_model->fg_strength->val[p] + EPS)
        );
  }

  return prob_map;
}

iftImage *ExtractParasites(iftImage *img) {
  iftImage *odomes     = iftOpenDomes(img,NULL,NULL);
  iftImage *resid      = iftSub(img,odomes);
  iftAdjRel *A         = iftCircular(1.0);
  iftImage *open       = iftOpen(resid,A,NULL);

  iftImage *parasite;
  int otsu_value = iftOtsu(open);
  if (otsu_value == 0) {
    parasite = iftCreateImageFromImage(img);
  } else {
    parasite = iftThreshold(open, otsu_value, IFT_INFINITY_INT, 255);
  }

  iftDestroyImage(&odomes);
  iftDestroyImage(&resid);
  iftDestroyImage(&open);
  iftDestroyAdjRel(&A);

  return parasite;
}


iftImage *ExtractTumor(iftImage *img, iftImage *brain_mask) {
  // Pre-processing steps
  iftImage *odomes = iftOpenDomes(img, NULL, NULL);
  iftImage *resid = iftSub(img, odomes);
  iftAdjRel *A = iftCircular(1.0);
  iftImage *open = iftOpen(resid, A, NULL);

  // Compute histogram of the pre-processed image within the brain mask
  int nbins = iftMaximumValue(open) + 1;
  int *hist = iftAllocIntArray(nbins);
  int npixels = 0;

  // Fill histogram for pixels within brain mask
  for (int p = 0; p < open->n; p++) {
    if (brain_mask->val[p] > 0) {
      hist[open->val[p]]++;
      npixels++;
    }
  }

  // Find the peak of the highest intensities (ignore background)
  int peak_intensity = 0;
  int max_freq = 0;
  int start_search = nbins * 0.7; // Start searching from top 30% intensities

  for (int i = start_search; i < nbins; i++) {
    if (hist[i] > max_freq) {
      max_freq = hist[i];
      peak_intensity = i;
    }
  }

  // Calculate mean and standard deviation around the peak
  double sum = 0.0, sum_sq = 0.0;
  int count = 0;
  int window = nbins * 0.2; // 10% window around peak

  for (int i = peak_intensity - window; i <= peak_intensity + window; i++) {
    if (i >= 0 && i < nbins) {
      sum += i * hist[i];
      sum_sq += i * i * hist[i];
      count += hist[i];
    }
  }

  double mean = (count > 0) ? sum / count : 0;
  double variance = (count > 0) ? (sum_sq / count) - (mean * mean) : 0;
  double std_dev = sqrt(variance);

  printf("Peak intensity: %d\n", peak_intensity);
  printf("Mean around peak: %.2f\n", mean);
  printf("Standard deviation: %.2f\n", std_dev);

  // Threshold based on mean - k*std_dev (k is a parameter to adjust)
  double k = 3.0; // Adjust this parameter based on validation
  int threshold = (int)(mean - k * std_dev);

  // Ensure threshold is within valid range
  threshold = iftMax(threshold, 1);
  printf("Final threshold: %d\n", threshold);

  // Apply threshold
  iftImage *tumor = iftThreshold(open, threshold, IFT_INFINITY_INT, 255);
  iftFree(hist);
  iftDestroyImage(&odomes);
  iftDestroyImage(&resid);
  iftDestroyImage(&open);
  iftDestroyAdjRel(&A);

  return tumor;
}

int main(int argc, char **argv) {
  timer *tstart;
  int memory_start, memory_end;
  memory_start = iftMemoryUsed();
  tstart = iftTic();

  if (argc != 6) {
    iftError(
      "Usage: iftCA <P1> <P2> <P3> <P4> <P5>\n"
      "P1: Saliency Folder.\n"
      "P2: Orig image folder.\n"
      "P3: Features Folder.\n"
      "P4: Output folder.\n"
      "P5: Parasites/Brain (0, 1)\n",
      "main"
    );
  }

  iftFileSet *fs = iftLoadFileSetFromDirBySuffix(argv[1], ".png", 1);
  char orig_path[512];
  char output_path[512];
  char ext[8];
  int is_brain = atoi(argv[5]); // 1 for brain
  sprintf(ext, "%s", iftFileExt(fs->files[0]->path));

  for (size_t i=0; i < fs->n; i++) {
    char *img_basename = iftFilename(fs->files[i]->path, ".png");
    printf("Processing Image: %s\n", img_basename);
    sprintf(orig_path, "%s/%s.png", argv[2], img_basename);

    // 1. Initializes CA Data
    iftCAModel *ca_model = InitializeCA(
      orig_path, fs->files[i]->path, 3, 1, is_brain
    );

    iftImage *fg_init = iftFImageToImage(ca_model->fg_strength, 255);
    sprintf(output_path, "%s/%s_%s%s", argv[4], img_basename, "fg_init", ext);
    iftWriteImageByExt(fg_init, output_path);
    iftImage *bg_init = iftFImageToImage(ca_model->bg_strength, 255);
    sprintf(output_path, "%s/%s_%s%s", argv[4], img_basename, "bg_init", ext);
    iftWriteImageByExt(bg_init, output_path);
    iftDestroyImage(&fg_init);
    iftDestroyImage(&bg_init);

    // 2. Evolves CA Data until target convergence (Verify algorithm in paper and implement here)
    evolve_ca_model(ca_model, 1e-12, is_brain);

    // 3. Gets Probability Map from foreground and background strength
    iftFImage *prob_map = GetProbMap(ca_model);
    iftImage *output_prob_map = iftFImageToImage(prob_map, 255);
    iftImage *output_fg = iftFImageToImage(ca_model->fg_strength, ca_model->i_max);
    iftImage *output_bg = iftFImageToImage(ca_model->bg_strength, ca_model->i_max);

    // 4. Saves foreground, background, and prob_tumor.
    sprintf(output_path, "%s/%s_%s%s", argv[4], img_basename, "prob_map", ext);
    iftWriteImageByExt(output_prob_map, output_path);
    sprintf(output_path, "%s/%s_%s%s", argv[4], img_basename, "fg", ext);
    iftWriteImageByExt(output_fg, output_path);
    sprintf(output_path, "%s/%s_%s%s", argv[4], img_basename, "bg", ext);
    iftWriteImageByExt(output_bg, output_path);
    sprintf(output_path, "%s/%s_%s%s", argv[4], img_basename, "label", ext);
    #pragma omp parallel for
    for (size_t p=0; p < ca_model->label->n; p++) {
      ca_model->label->val[p] = ca_model->label->val[p] * 255;
    }
    iftWriteImageByExt(ca_model->label, output_path);

    // 5. Computes final saliency
    iftImage *output_saliency = NULL;
    if (!is_brain) {
      output_saliency = ExtractParasites(output_prob_map);
    } else {
      output_saliency = ExtractTumor(output_prob_map, ca_model->brain_mask);
    }
    sprintf(output_path, "%s/%s_%s%s", argv[4], img_basename, "sal", ext);
    iftWriteImageByExt(output_saliency, output_path);
    // [EXPS] Current version does not use features yet
    // Releases allocated resource
    iftDestroyFImage(&prob_map);
    iftDestroyImage(&output_saliency);
    iftDestroyImage(&output_prob_map);
    iftDestroyImage(&output_fg);
    iftDestroyImage(&output_bg);
    iftFree(img_basename);
    DestroyCAModel(&ca_model);
  }

  // Releases allocated resource
  iftDestroyFileSet(&fs);

  puts("\nDone ...");
  puts(iftFormattedTime(iftCompTime(tstart, iftToc())));
  memory_end = iftMemoryUsed();
  iftVerifyMemory(memory_start, memory_end);

  return 0;
}