/*=============================================================================
*                                                                       
*                Copyright (C) 2008 Hantro Products Oy     		 
* File Name: 	rm_user.h  
* User for : 	rm demux mux, 
* Creat By : 	                                                                   
* CrateDate: 	2008/05/28
* Last modify :      
*
* relase history:
* 		version 0.1 release by caijin
=============================================================================*/

#ifndef RM_USER_H
#define RM_USER_H

/*=============================================================================
	from here add for 830 media player control(memory and frame counter control)
=============================================================================*/
#define RM_AUDIO_MAX_BUFFER_SIZE 		(1024*32)			// at least can load 64 frames aac in buffer((max 1024 bytes/f)),  also at least load 2048 frames amr in buffer 	
#define RM_VIDEO_MAX_BUFFER_SIZE 		(1024*100)		// at least can load 5 frames 320X240 video in buffer(max 20k/f)

#define RM_AUDIO_MAX_FRAME_READ 		32				//max read 240 frames amr / 128 frames aac audio once time( 48000k aac means read data 128/48 s)			
#define RM_AUDIO_MIN_FRAME_READ 		24				//mix read 120 frames amr / 64 frames aac audio once time( 48000k aac means read data 64/48 s)		

#define RM_VIDEO_MAX_FRAME_READ 		2				//max read 30 frames video once time( 30fps means read data 1000 ms)		
#define RM_VIDEO_MIN_FRAME_READ 		1				//min read 10 frames video once time( 30fps means read data 300 ms)			


#define RM_DEPACK_CALLBACK  0	// call back
#define RM_DEPACK_SENDFIFO  1	// send to fifo
#define RM_DEPACK_PACKET	0	// packet by packet
#define RM_DEPACK_FRAME		2	// frame by frame

#define RM_DEPACK_MODE	RM_DEPACK_SENDFIFO|RM_DEPACK_FRAME

#define RM_DEPACK_1BIT_MASK	0x1
#define RM_DEPACK_2BIT_MASK	0x3
#define RM_DEPACK_3BIT_MASK	0x7
#define RM_DEPACK_4BIT_MASK	0xF
#define RM_DEPACK_CALLBACK_OR_FIFO_MASK_SHIFT 0
#define RM_DEPACK_PACKET_OR_FRAME_MASK_SHIFT  1

#define RM_DEPACK_IS_CALLBACK(mode) (((~mode)>>(RM_DEPACK_CALLBACK_OR_FIFO_MASK_SHIFT))&(RM_DEPACK_1BIT_MASK))
#define RM_DEPACK_IS_FIFO(mode) (((mode)>>(RM_DEPACK_CALLBACK_OR_FIFO_MASK_SHIFT))&(RM_DEPACK_1BIT_MASK))
#define RM_DEPACK_IS_PACKET_BY_PACKET(mode) (((~mode)>>(RM_DEPACK_PACKET_OR_FRAME_MASK_SHIFT))&(RM_DEPACK_1BIT_MASK))
#define RM_DEPACK_IS_FRAME_BY_FRAME(mode) (((mode)>>(RM_DEPACK_PACKET_OR_FRAME_MASK_SHIFT))&(RM_DEPACK_1BIT_MASK))

#endif
