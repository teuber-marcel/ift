/*****************************************************************************\
* RunSICLECOMP.c
*
* AUTHOR  : Felipe Belem
* DATE    : 2023-01-09
* LICENSE : MIT License
* EMAIL   : felipe.belem@ic.unicamp.br
\*****************************************************************************/
#include "ift.h"
#include "iftArgs.h"
#include "ift/segm/iftSICLE.h"

/* PROTOTYPES ****************************************************************/
void readImgInputs
(iftArgs *args, iftImage **img, iftImage **mask, iftImage **objsm, 
	const char **path, bool *is_video);

void readSICLEArgs
(iftArgs *args, iftSICLEArgs **sargs);

void usage();

char *remove_ext (const char* myStr, char extSep, char pathSep);

void iftWriteVolumeAsSingleVideoFolder
(  iftImage *video, const char *path);

/* MAIN **********************************************************************/
int main(int argc, char   *argv[])
{
	//-----------------------------------------------------------------------//
	bool has_req, has_help;
	iftArgs *args;

	args = iftCreateArgs(argc, argv);

	has_req = iftExistArg(args, "img")  && iftExistArg(args, "out");
	has_help = iftExistArg(args, "help");

	if(has_req == false || has_help == true)
	{ usage(); iftDestroyArgs(&args); return EXIT_FAILURE; }
	//-----------------------------------------------------------------------//
	bool multiscale, is_video;
	const char* OUT;
	iftSICLEArgs *sargs;
	iftSICLE *sicle;
	iftImage *img, *objsm, *mask;

	multiscale = iftExistArg(args, "multiscale");
	readImgInputs(args, &img, &mask, &objsm, &OUT, &is_video);
	readSICLEArgs(args, &sargs);
	iftDestroyArgs(&args);
	
	sicle = iftCreateSICLE(img, objsm, mask);
	iftDestroyImage(&img);
	if(objsm != NULL) { iftDestroyImage(&objsm); }
	if(mask != NULL) { iftDestroyImage(&mask); }

	iftVerifySICLEArgs(sicle, sargs);
	if(multiscale == false)
	{
		iftImage *segm;

		segm = iftRunSICLE(sicle, sargs);
		if(is_video == false)
		{ iftWriteImageByExt(segm, OUT); }
		else
		{ iftWriteVolumeAsSingleVideoFolder(segm, OUT); }
		iftDestroyImage(&segm);
	}
	else
	{
		const char *EXT;
		int num_scales;
		char *basename;
		iftImage **multisegm;

	  EXT = iftFileExt(OUT);
	  basename = remove_ext(OUT,'.','/');
		multisegm = iftRunMultiscaleSICLE(sicle, sargs, &num_scales);
		for(int i = 0; i < num_scales; ++i)
		{
			if(is_video == false)
			{ iftWriteImageByExt(multisegm[i], "%s_%d%s", basename, i+1, EXT); }
			else
			{ 
				char tmp[IFT_STR_DEFAULT_SIZE];

				sprintf(tmp, "%s_%d/%s",basename,i+1,EXT);
				iftWriteVolumeAsSingleVideoFolder(multisegm[i], tmp); 
			}
			iftDestroyImage(&(multisegm[i]));
		}
		free(multisegm);
	}
	iftDestroySICLE(&sicle);
	iftDestroySICLEArgs(&sargs);


	return EXIT_SUCCESS;
}
/* METHODS********************************************************************/
void readImgInputs
(iftArgs *args, iftImage **img, iftImage **mask, iftImage **objsm, 
	const char **path, bool *is_video)
{
	const char *VAL;
	if(iftHasArgVal(args, "img") == true)
	{
		VAL = iftGetArg(args,"img");
		if(iftIsImageFile(VAL) == true) 
		{ (*img) = iftReadImageByExt(VAL); (*is_video) = false; }
		else if(iftDirExists(VAL) == true)
		{ (*img) = iftReadImageFolderAsVolume(VAL); (*is_video) = true;}
		else { iftError("Unknown image/video format", __func__); } 
	}
	else { iftError("No image path was given", __func__); }

	if(iftHasArgVal(args, "out") == true)
	{ (*path) = iftGetArg(args, "out"); }
	else { iftError("No output path was given", __func__); }

	if(iftExistArg(args, "mask") == false) { (*mask) = NULL; }
	else if(iftHasArgVal(args, "mask") == true)
	{
		VAL = iftGetArg(args,"mask");
		if(iftIsImageFile(VAL) == true) 
		{ (*mask) = iftReadImageByExt(VAL); }
		else if(iftDirExists(VAL) == true)
		{ (*mask) = iftReadImageFolderAsVolume(VAL); }
		else { iftError("Unknown image/video format", __func__); }

		iftVerifyImageDomains((*img), (*mask), __func__);
 	}
	else { iftError("No mask path was given", __func__); }

	if(iftExistArg(args, "objsm") == false) { (*objsm) = NULL; }
	else if(iftHasArgVal(args, "objsm") == true)
	{
		VAL = iftGetArg(args,"objsm");
		if(iftIsImageFile(VAL) == true) 
		{ (*objsm) = iftReadImageByExt(VAL); }
		else if(iftDirExists(VAL) == true)
		{ (*objsm) = iftReadImageFolderAsVolume(VAL); } 
		else { iftError("Unknown image/video format", __func__); }

		iftVerifyImageDomains((*img), (*objsm), __func__);
	}
	else { iftError("No saliency map path was given", __func__); }
}

void readSICLEArgs
(iftArgs *args, iftSICLEArgs **sargs)
{
	(*sargs) = iftCreateSICLEArgs();

	(*sargs)->use_diag = true;
	(*sargs)->use_dift = true;
	(*sargs)->n0 = 3000;
	(*sargs)->max_iters = 7;
	(*sargs)->irreg = 0.12;
	(*sargs)->adhr = 12;
	(*sargs)->connopt = IFT_SICLE_CONN_FSUM;
	(*sargs)->critopt = IFT_SICLE_CRIT_MAXSC;

	if(iftExistArg(args, "objsm") == true)
	{
		(*sargs)->alpha = 2.0;
		(*sargs)->penopt = IFT_SICLE_PEN_BOBS;
	}
	else
	{
		(*sargs)->alpha = 0.0;
		(*sargs)->penopt = IFT_SICLE_PEN_NONE;
	}

	if(iftExistArg(args, "nf") == true)
	{ 
		if(iftHasArgVal(args, "nf") == true) 
		{ (*sargs)->nf = atoi(iftGetArg(args, "nf")); }
		else { iftError("No final quantity of superspels was given", __func__); }
	}
	
	if(iftExistArg(args, "ni") == true)
	{
	    if(iftHasArgVal(args, "ni") == true)
	    {
	      const char *VAL;
	      char *tmp, *tok;
	      int i;
	      iftSet *vals;

	      vals = NULL;
	      VAL = iftGetArg(args, "ni");
	      tmp = iftCopyString(VAL);
	      tok = strtok(tmp, ",");

	      i = 0;
	      while(tok != NULL)
	      {
	        iftInsertSet(&vals, atoi(tok));
	        tok = strtok(NULL, ",");
	        ++i;
	     	}
	      free(tmp);
	      if(vals == NULL) { iftError("No list of Ni values was provided", __func__); }

	      (*sargs)->user_ni = iftCreateIntArray(i);
	      while(vals != NULL)
	      { (*sargs)->user_ni->val[--i] = iftRemoveSet(&vals);}
	    }
	    else { iftError("No list of Ni values was provided", __func__); }
	} 
}

void usage()
{
	  int SKIP_IND = 15; // For indentation purposes
	printf("\nMandatory parameters:\n");
	printf("%-*s %s\n", SKIP_IND, "--img",
		"Input image");
	printf("%-*s %s\n", SKIP_IND, "--out",
		"Output label image");

	printf("\nOptional files:\n");
	printf("%-*s %s\n", SKIP_IND, "--mask",
		"Mask image indicating the region of interest.");
	printf("%-*s %s\n", SKIP_IND, "--objsm",
		"Grayscale object saliency map.");

	printf("\nOptional general parameters:\n");
	printf("%-*s %s\n", SKIP_IND, "--multiscale",
		"Generates a multiscale segmentation.");
	printf("%-*s %s\n", SKIP_IND, "--nf",
		"Desired final number of superpixels. Default: 200");
	printf("%-*s %s\n", SKIP_IND, "--ni",
		"Comma-separated list of intermediary seed quantity.");
	printf("%-*s %s\n", SKIP_IND, "--help",
		"Prints this message");

	printf("\n");
}

// from: https://stackoverflow.com/questions/2736753/how-to-remove-extension-from-file-name
char *remove_ext (const char* myStr, char extSep, char pathSep) {
    char *retStr, *lastExt, *lastPath;

    // Error checks and allocate string.

    if (myStr == NULL) return NULL;
    if ((retStr = malloc (strlen (myStr) + 1)) == NULL) return NULL;

    // Make a copy and find the relevant characters.

    strcpy (retStr, myStr);
    lastExt = strrchr (retStr, extSep);
    lastPath = (pathSep == 0) ? NULL : strrchr (retStr, pathSep);

    // If it has an extension separator.

    if (lastExt != NULL) {
        // and it's to the right of the path separator.

        if (lastPath != NULL) {
            if (lastPath < lastExt) {
                // then remove it.

                *lastExt = '\0';
            }
        } else {
            // Has extension separator with no path separator.

            *lastExt = '\0';
        }
    }

    // Return the modified string.

    return retStr;
}

void iftWriteVolumeAsSingleVideoFolder
(  iftImage *video, const char *path)
{
    int i;
    iftImage *frame = NULL;
    const char *EXT = iftFileExt(path);
    char *base = iftRemoveSuffix(path, EXT);

    for(i = 0; i < video->zsize; i++)
    {
        char mod_path[IFT_STR_DEFAULT_SIZE];

        sprintf(mod_path, "%s%0*d%s", base, IFT_VIDEO_FOLDER_FRAME_NZEROES, 
                                       i, EXT);

        frame = iftGetXYSlice(video, i);
        iftWriteImageByExt(frame, mod_path);

        iftDestroyImage(&frame);
    }
    free(base);
}