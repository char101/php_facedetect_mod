/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Robert Eisele <robert@xarg.org>                              |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "php.h"
#include "ext/standard/info.h"
#include "php_facedetect.h"

#include <opencv/cv.h>
#include <opencv/highgui.h>

/* {{{ facedetect_functions[]
 *
 * Every user visible function must have an entry in facedetect_functions[].
 */
static function_entry facedetect_functions[] = {
    PHP_FE(face_detect, NULL)
    PHP_FE(face_count, NULL)
    {NULL, NULL, NULL}
};
/* }}} */

/* {{{ facedetect_module_entry
 */
zend_module_entry facedetect_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_FACEDETECT_EXTNAME,
    facedetect_functions,
    NULL,
    NULL,
    NULL,
    NULL,
    PHP_MINFO(facedetect),
#if ZEND_MODULE_API_NO >= 20010901
    PHP_FACEDETECT_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_FACEDETECT
ZEND_GET_MODULE(facedetect)
#endif

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINFO_FUNCTION(facedetect)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "facedetect support", "enabled");
	php_info_print_table_row(2, "facedetect version", PHP_FACEDETECT_VERSION);
	php_info_print_table_row(2, "OpenCV version", CV_VERSION);
	php_info_print_table_end();
}
/* }}} */

static void php_facedetect(INTERNAL_FUNCTION_PARAMETERS, int return_type)
{
	char *file = 0, *casc = 0;
	long flen = 0, clen = 0;

	zval *array = 0;

	CvHaarClassifierCascade* cascade = 0;
	IplImage *img = 0;
	IplImage *temp = 0;
	//IplImage *dummy = 0;
	IplImage *smallimg = 0;
	CvMemStorage *storage = 0;
	CvSeq *faces = 0;
	CvRect *rect = 0;
	double scale = 1.0;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &file, &flen, &casc, &clen) == FAILURE) {
		RETURN_NULL();
	}

	if (!flen || !clen) {
		RETURN_FALSE;
	}

	//php_printf(file);
	//php_printf(casc);

	// BUGFIX
	//dummy = cvCreateImage(cvSize(1, 1), IPL_DEPTH_8U, 1);
    //cvErode(dummy, dummy, 0, 1);
    //cvReleaseImage(&dummy);

	img = cvLoadImage(file, 1);
	if (!img) {
		RETURN_FALSE;
	}

	cascade = (CvHaarClassifierCascade*)cvLoad(casc, 0, 0, 0);
	if (!cascade) {
		RETURN_FALSE;
	}

	storage = cvCreateMemStorage(0);
	if (!storage) {
		RETURN_FALSE;
	}

	if (img->width > 600) {
		scale = 1.0 * img->width / 600;
	}

	temp = cvCreateImage(cvSize(img->width, img->height), 8, 1);
	cvCvtColor(img, temp, CV_BGR2GRAY);
	cvEqualizeHist(temp, temp);
	
	if (scale > 1.0) {
		smallimg = cvCreateImage(cvSize(cvRound(img->width/scale), cvRound(img->height/scale)), 8, 1);
		cvResize(temp, smallimg, CV_INTER_LINEAR);
		cvReleaseImage(&temp);
		temp = smallimg;
	}
	
	faces = cvHaarDetectObjects(
			temp, // grayscale image
			cascade, 
			storage, 
			1.1, // scale factor
			2, 
			CV_HAAR_DO_CANNY_PRUNING | CV_HAAR_FIND_BIGGEST_OBJECT | CV_HAAR_SCALE_IMAGE, 
			cvSize(50, 50), // minimum size of a face
			cvSize(0, 0)
	);

	cvClearMemStorage(storage);

	if (return_type) {
		array_init(return_value);

		if (faces && faces->total > 0) {
			int i;
			for(i = 0; i < faces->total; ++i) {
				MAKE_STD_ZVAL(array);
				array_init(array);

				rect = (CvRect *)cvGetSeqElem(faces, i);

				add_assoc_long(array, "x", cvRound(scale * rect->x));
				add_assoc_long(array, "y", cvRound(scale * rect->y));
				add_assoc_long(array, "w", cvRound(scale * rect->width));
				add_assoc_long(array, "h", cvRound(scale * rect->height));

				add_next_index_zval(return_value, array);
			}
		}
	} else {
		RETVAL_LONG(faces ? faces->total : 0);
	}

	if (faces) {
		cvClearSeq(faces);
	}

	cvReleaseMemStorage(&storage);
	cvReleaseImage(&temp);
	cvReleaseImage(&img);
}

/* {{{ proto array face_detect(string picture_path, string cascade_file)
   Finds coordinates of faces (or in gernal "objects") on the given picture */
PHP_FUNCTION(face_detect)
{
	php_facedetect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */

/* {{{ proto int face_count(string picture_path, string cascade_file)
   Finds number of faces (or in gernal "objects") on the given picture*/
PHP_FUNCTION(face_count)
{
	php_facedetect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */

