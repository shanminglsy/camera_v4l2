#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "h264encoder.h"

void compress_begin(Encoder *en, int width, int height)
{
	en->param = (x264_param_t *) malloc(sizeof(x264_param_t));
	en->picture = (x264_picture_t *) malloc(sizeof(x264_picture_t));
	x264_param_default(en->param); //set default param
/*
	//en->param->rc.i_rc_method = X264_RC_CQP;
	// en->param->i_log_level = X264_LOG_NONE;

	 en->param->i_threads  = X264_SYNC_LOOKAHEAD_AUTO;

	en->param->i_width = width; //set frame width
	en->param->i_height = height; //set frame height

	en->param->i_frame_total = 0;

	 en->param->i_keyint_max = 10;
	en->param->rc.i_lookahead = 0; 
	   en->param->i_bframe = 5; 

	  en->param->b_open_gop = 0;
	  en->param->i_bframe_pyramid = 0;
	   en->param->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;

	en->param->rc.i_bitrate = 1024 * 10;//rate 为10 kbps
	en->param->i_fps_num = 5; 
	en->param->i_fps_den = 1; 
	x264_param_apply_profile(en->param, x264_profile_names[4]); 

	if ((en->handle = x264_encoder_open(en->param)) == 0) {
		printf("handle is zero");
		return;
	}
  */
	/* Create a new pic */
//==================================================================================
	x264_param_default_preset(en->param, "fast" , "zerolatency" );
 	en->param->i_width = width; 
 	en->param->i_height = height; 
 	en->param->b_repeat_headers = 1;           
 	en->param->b_cabac = 1;          
	en-> param->i_threads = 1;            
	en-> param->i_fps_num = 30;          
	en-> param->i_fps_den = 1;
	en-> param->i_keyint_max =10; 

	en-> param->rc.f_rf_constant = 20; 
	en-> param->rc.f_rf_constant_max = 45; 
	en-> param->rc.i_rc_method = X264_RC_CRF ;// X264_RC_ABR;
	 //param.rc.f_rate_tolerance=0.1;
	en-> param->rc.i_vbv_max_bitrate=1024*10*1.2;

 	en-> param->rc.i_bitrate = 1024*10;//(int)m_bitRate/1000; 


	 x264_param_apply_profile(en->param, "baseline");
	 en->param->i_level_idc=30;

	 en->param->i_log_level = X264_LOG_NONE;

 	if(( en->handle  = x264_encoder_open(en->param)) == NULL) 
	{	
		return;
	}

//===================================================================================
	x264_picture_alloc(en->picture, X264_CSP_I420, en->param->i_width,en->param->i_height);
	en->picture->img.i_csp = X264_CSP_I420;
	en->picture->img.i_plane = 3;
}

//====================================================================

int compress_frame(Encoder *en, int type, uint8_t *in, uint8_t *out) 
{

	x264_picture_t pic_out;
	int nNal = -1;
	int result = 0;
	int i = 0;
	uint8_t *p_out = out;




    

	char *y = en->picture->img.plane[0];
	char *u = en->picture->img.plane[1];
	char *v = en->picture->img.plane[2];
     
	int is_y = 1, is_u = 1;
	int y_index = 0, u_index = 0, v_index = 0;

	int yuv422_length = 2 * en->param->i_width * en->param->i_height;

	for (i = 0; i < yuv422_length; ++i) {
		if (is_y) {
			*(y + y_index) = *(in + i);
			++y_index;
			is_y = 0;
		} else {
			if (is_u) {
				*(u + u_index) = *(in + i);
				++u_index;
				is_u = 0;
			} else {
				*(v + v_index) = *(in + i);
				++v_index;
				is_u = 1;
			}
			is_y = 1;
		}
	}


	switch (type) {
	case 0:
		en->picture->i_type = X264_TYPE_P;
		break;
	case 1:
		en->picture->i_type = X264_TYPE_IDR;
		break;
	case 2:
		en->picture->i_type = X264_TYPE_I;
		break;
	default:
		en->picture->i_type = X264_TYPE_AUTO;
		break;
	}
     //   en->picture->i_pts = pts++;                                           //add 
        printf("here_1\n");
	if (x264_encoder_encode(en->handle, &(en->nal), &nNal, en->picture,
			&pic_out) < 0) {
		printf("here_2\n");
		return -1;
	}
       en->picture->i_pts++;
	for (i = 0; i < nNal; i++) {
		memcpy(p_out, en->nal[i].p_payload, en->nal[i].i_payload);
		p_out += en->nal[i].i_payload;
		result += en->nal[i].i_payload;
	}

	return result;
}

//===================================================================
void compress_end(Encoder *en)
{
	if (en->picture) {
		x264_picture_clean(en->picture);
		free(en->picture);
		en->picture = 0;
	}
	if (en->param) {
		free(en->param);
		en->param = 0;
	}
	if (en->handle) {
		x264_encoder_close(en->handle);
	}
	free(en);
}
